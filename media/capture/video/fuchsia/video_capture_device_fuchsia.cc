// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/fuchsia/video_capture_device_fuchsia.h"

#include <zircon/status.h>

#include "base/fuchsia/fuchsia_logging.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "media/base/video_types.h"
#include "third_party/libyuv/include/libyuv/convert.h"
#include "ui/gfx/buffer_format_util.h"

namespace media {

namespace {

size_t RoundUp(size_t value, size_t alignment) {
  return ((value + alignment - 1) / alignment) * alignment;
}

void CopyAndConvertFrame(
    base::span<const uint8_t> src_span,
    fuchsia::sysmem::PixelFormatType src_pixel_format,
    size_t src_stride_y,
    size_t src_coded_height,
    std::unique_ptr<VideoCaptureBufferHandle> output_handle,
    gfx::Size output_size) {
  const uint8_t* src_y = src_span.data();
  size_t src_y_plane_size = src_stride_y * src_coded_height;

  // Calculate offsets and strides for the output buffer.
  uint8_t* dst_y = output_handle->data();
  int dst_stride_y = output_size.width();
  size_t dst_y_plane_size = output_size.width() * output_size.height();
  uint8_t* dst_u = dst_y + dst_y_plane_size;
  int dst_stride_u = output_size.width() / 2;
  uint8_t* dst_v = dst_u + dst_y_plane_size / 4;
  int dst_stride_v = output_size.width() / 2;

  // Check that the output fits in the buffer.
  const uint8_t* dst_end = dst_v + dst_y_plane_size / 4;
  CHECK_LE(dst_end, output_handle->data() + output_handle->mapped_size());

  switch (src_pixel_format) {
    case fuchsia::sysmem::PixelFormatType::YV12:
    case fuchsia::sysmem::PixelFormatType::I420: {
      const uint8_t* src_u = src_y + src_y_plane_size;
      int src_stride_u = src_stride_y / 2;
      size_t src_u_plane_size = src_stride_u * src_coded_height / 2;
      const uint8_t* src_v = src_u + src_u_plane_size;
      int src_stride_v = src_stride_y / 2;

      if (src_pixel_format == fuchsia::sysmem::PixelFormatType::YV12) {
        // Swap U and V planes to account for different plane order in YV12.
        std::swap(src_u, src_v);
      }

      size_t src_v_plane_size = src_stride_v * src_coded_height / 2;
      const uint8_t* src_end = src_v + src_v_plane_size;
      CHECK_LE(src_end, src_span.data() + src_span.size());

      libyuv::I420Copy(src_y, src_stride_y, src_u, src_stride_u, src_v,
                       src_stride_v, dst_y, dst_stride_y, dst_u, dst_stride_u,
                       dst_v, dst_stride_v, output_size.width(),
                       output_size.height());
      break;
    }

    case fuchsia::sysmem::PixelFormatType::NV12: {
      const uint8_t* src_uv = src_y + src_stride_y * src_coded_height;
      int src_stride_uv = src_stride_y;

      int src_uv_plane_size = src_stride_uv * src_coded_height / 2;
      const uint8_t* src_end = src_uv + src_uv_plane_size;
      CHECK_LE(src_end, src_span.data() + src_span.size());

      libyuv::NV12ToI420(src_y, src_stride_y, src_uv, src_stride_uv, dst_y,
                         dst_stride_y, dst_u, dst_stride_u, dst_v, dst_stride_v,
                         output_size.width(), output_size.height());

      break;
    }

    default:
      NOTREACHED();
  }
}

}  // namespace

// static
VideoPixelFormat VideoCaptureDeviceFuchsia::GetConvertedPixelFormat(
    fuchsia::sysmem::PixelFormatType format) {
  switch (format) {
    case fuchsia::sysmem::PixelFormatType::I420:
    case fuchsia::sysmem::PixelFormatType::YV12:
    case fuchsia::sysmem::PixelFormatType::NV12:
      // Convert all YUV formats to I420 since consumers currently don't support
      // NV12 or YV12.
      return PIXEL_FORMAT_I420;

    default:
      LOG(ERROR) << "Camera uses unsupported pixel format "
                 << static_cast<int>(format);
      return PIXEL_FORMAT_UNKNOWN;
  }
}

bool VideoCaptureDeviceFuchsia::IsSupportedPixelFormat(
    fuchsia::sysmem::PixelFormatType format) {
  return GetConvertedPixelFormat(format) != PIXEL_FORMAT_UNKNOWN;
}

VideoCaptureDeviceFuchsia::VideoCaptureDeviceFuchsia(
    fidl::InterfaceHandle<fuchsia::camera3::Device> device) {
  device_.Bind(std::move(device));
  device_.set_error_handler(
      fit::bind_member(this, &VideoCaptureDeviceFuchsia::OnDeviceError));
}

VideoCaptureDeviceFuchsia::~VideoCaptureDeviceFuchsia() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void VideoCaptureDeviceFuchsia::AllocateAndStart(
    const VideoCaptureParams& params,
    std::unique_ptr<Client> client) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(params.requested_format.pixel_format, PIXEL_FORMAT_I420);
  DCHECK(!client_);
  DCHECK(!stream_);

  client_ = std::move(client);

  if (!device_) {
    OnError(FROM_HERE, VideoCaptureError::kFuchsiaCameraDeviceDisconnected,
            "fuchsia.camera3.Device disconnected");
    return;
  }

  start_time_ = base::TimeTicks::Now();
  frames_received_ = 0;

  // TODO(crbug.com/1075839) Select stream_id based on requested resolution.
  device_->ConnectToStream(/*stream_id=*/0, stream_.NewRequest());
  stream_.set_error_handler(
      fit::bind_member(this, &VideoCaptureDeviceFuchsia::OnStreamError));

  WatchResolution();

  // Call SetBufferCollection() with a new buffer collection token to indicate
  // that we are interested in buffer collection negotiation. The collection
  // token will be returned back from WatchBufferCollection(). After that it
  // will be initialized in InitializeBufferCollection().
  stream_->SetBufferCollection(sysmem_allocator_.CreateNewToken());
  WatchBufferCollection();
}

void VideoCaptureDeviceFuchsia::StopAndDeAllocate() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DisconnectStream();
  client_.reset();
}

void VideoCaptureDeviceFuchsia::OnDeviceError(zx_status_t status) {
  OnError(FROM_HERE, VideoCaptureError::kFuchsiaCameraDeviceDisconnected,
          base::StringPrintf("fuchsia.camera3.Device disconnected: %s (%d)",
                             zx_status_get_string(status), status));
}

void VideoCaptureDeviceFuchsia::OnStreamError(zx_status_t status) {
  OnError(FROM_HERE, VideoCaptureError::kFuchsiaCameraStreamDisconnected,
          base::StringPrintf("fuchsia.camera3.Stream disconnected: %s (%d)",
                             zx_status_get_string(status), status));
}

void VideoCaptureDeviceFuchsia::DisconnectStream() {
  stream_.Unbind();
  buffer_collection_creator_.reset();
  buffer_collection_.reset();
  buffer_reader_.reset();
  frame_size_.reset();
}

void VideoCaptureDeviceFuchsia::OnError(base::Location location,
                                        VideoCaptureError error,
                                        const std::string& reason) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  DisconnectStream();

  if (client_) {
    client_->OnError(error, location, reason);
  }
}

void VideoCaptureDeviceFuchsia::WatchResolution() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  stream_->WatchResolution(fit::bind_member(
      this, &VideoCaptureDeviceFuchsia::OnWatchResolutionResult));
}

void VideoCaptureDeviceFuchsia::OnWatchResolutionResult(
    fuchsia::math::Size frame_size) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  frame_size_ = gfx::Size(frame_size.width, frame_size.height);

  WatchResolution();
}

void VideoCaptureDeviceFuchsia::WatchBufferCollection() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  stream_->WatchBufferCollection(
      [this](fidl::InterfaceHandle<fuchsia::sysmem::BufferCollectionToken>
                 token_handle) {
        InitializeBufferCollection(std::move(token_handle));
        WatchBufferCollection();
      });
}

void VideoCaptureDeviceFuchsia::InitializeBufferCollection(
    fidl::InterfaceHandle<fuchsia::sysmem::BufferCollectionToken>
        token_handle) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Drop old buffers.
  buffer_collection_.reset();
  buffer_reader_.reset();

  // Initialize the new collection.
  fuchsia::sysmem::BufferCollectionTokenPtr token;
  token.Bind(std::move(token_handle));
  buffer_collection_creator_ =
      sysmem_allocator_.MakeBufferPoolCreatorFromToken(std::move(token));

  // Request just one buffer in collection constraints: each frame is copied as
  // soon as it's received.
  const size_t kMaxUsedOutputFrames = 1;
  fuchsia::sysmem::BufferCollectionConstraints constraints =
      SysmemBufferReader::GetRecommendedConstraints(kMaxUsedOutputFrames);
  buffer_collection_creator_->Create(
      std::move(constraints),
      base::BindOnce(&VideoCaptureDeviceFuchsia::OnBufferCollectionCreated,
                     base::Unretained(this)));
}

void VideoCaptureDeviceFuchsia::OnBufferCollectionCreated(
    std::unique_ptr<SysmemBufferPool> collection) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  buffer_collection_ = std::move(collection);
  buffer_collection_->CreateReader(
      base::BindOnce(&VideoCaptureDeviceFuchsia::OnBufferReaderCreated,
                     base::Unretained(this)));
}

void VideoCaptureDeviceFuchsia::OnBufferReaderCreated(
    std::unique_ptr<SysmemBufferReader> reader) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  buffer_reader_ = std::move(reader);
  if (!buffer_reader_->buffer_settings().has_image_format_constraints) {
    OnError(FROM_HERE, VideoCaptureError::kFuchsiaSysmemDidNotSetImageFormat,
            "Sysmem created buffer without image format constraints");
    return;
  }

  auto pixel_format = buffer_reader_->buffer_settings()
                          .image_format_constraints.pixel_format.type;
  if (!IsSupportedPixelFormat(pixel_format)) {
    OnError(FROM_HERE, VideoCaptureError::kFuchsiaUnsupportedPixelFormat,
            base::StringPrintf("Unsupported video frame format: %d",
                               static_cast<int>(pixel_format)));
    return;
  }

  if (!started_) {
    started_ = true;
    client_->OnStarted();
    ReceiveNextFrame();
  }
}

void VideoCaptureDeviceFuchsia::ReceiveNextFrame() {
  stream_->GetNextFrame([this](fuchsia::camera3::FrameInfo frame_info) {
    ProcessNewFrame(std::move(frame_info));
    ReceiveNextFrame();
  });
}

void VideoCaptureDeviceFuchsia::ProcessNewFrame(
    fuchsia::camera3::FrameInfo frame_info) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(client_);

  if (!buffer_reader_) {
    DLOG(WARNING) << "Dropping frame received before sysmem collection has "
                     "been initialized.";
    return;
  }

  size_t index = frame_info.buffer_index;
  if (index >= buffer_reader_->num_buffers()) {
    OnError(FROM_HERE, VideoCaptureError::kFuchsiaSysmemInvalidBufferIndex,
            base::StringPrintf("Received frame with invalid buffer_index=%zu",
                               index));
    return;
  }

  const fuchsia::sysmem::ImageFormatConstraints& sysmem_buffer_format =
      buffer_reader_->buffer_settings().image_format_constraints;

  // Calculate coded frame dimensions for the buffer collection based on the
  // sysmem collection constraints. This logic should match
  // LogicalBufferCollection::Allocate() in sysmem.
  size_t src_coded_width =
      RoundUp(std::max(sysmem_buffer_format.min_coded_width,
                       sysmem_buffer_format.required_max_coded_width),
              sysmem_buffer_format.coded_width_divisor);
  size_t src_coded_height =
      RoundUp(std::max(sysmem_buffer_format.min_coded_height,
                       sysmem_buffer_format.required_max_coded_height),
              sysmem_buffer_format.coded_height_divisor);
  size_t src_stride = RoundUp(
      std::max(static_cast<size_t>(sysmem_buffer_format.min_bytes_per_row),
               src_coded_width),
      sysmem_buffer_format.bytes_per_row_divisor);
  gfx::Size visible_size =
      frame_size_.value_or(gfx::Size(src_coded_width, src_coded_height));
  gfx::Size output_size((visible_size.width() + 1) & ~1,
                        (visible_size.height() + 1) & ~1);

  base::TimeTicks reference_time =
      base::TimeTicks::FromZxTime(frame_info.timestamp);
  base::TimeDelta timestamp =
      std::max(reference_time - start_time_, base::TimeDelta());

  float frame_rate =
      (timestamp > base::TimeDelta())
          ? static_cast<float>(frames_received_) / timestamp.InSecondsF()
          : 0.0;
  VideoCaptureFormat capture_format(output_size, frame_rate, PIXEL_FORMAT_I420);
  capture_format.pixel_format = PIXEL_FORMAT_I420;

  Client::Buffer buffer;
  Client::ReserveResult result = client_->ReserveOutputBuffer(
      capture_format.frame_size, capture_format.pixel_format,
      /*frame_feedback_id=*/0, &buffer);
  if (result != Client::ReserveResult::kSucceeded) {
    DLOG(WARNING) << "Failed to allocate output buffer for a video frame";
    return;
  }

  auto src_span = buffer_reader_->GetMappingForBuffer(index);
  if (src_span.empty()) {
    OnError(FROM_HERE, VideoCaptureError::kFuchsiaFailedToMapSysmemBuffer,
            "Failed to map buffers allocated by sysmem");
    return;
  }

  // For all supported formats (I420, NV12 and YV12) the U and V channels are
  // subsampled at 2x in both directions, so together they occupy half of the
  // space needed for the Y plane and the total buffer size is 3/2 of the Y
  // plane size.
  size_t src_buffer_size = src_coded_height * src_stride * 3 / 2;
  if (src_span.size() < src_buffer_size) {
    OnError(FROM_HERE, VideoCaptureError::kFuchsiaSysmemInvalidBufferSize,
            "Sysmem allocated buffer that's smaller than expected");
    return;
  }

  auto src_pixel_format = buffer_reader_->buffer_settings()
                              .image_format_constraints.pixel_format.type;
  CopyAndConvertFrame(src_span, src_pixel_format, src_stride, src_coded_height,
                      buffer.handle_provider->GetHandleForInProcessAccess(),
                      output_size);

  client_->OnIncomingCapturedBufferExt(
      std::move(buffer), capture_format, gfx::ColorSpace(), reference_time,
      timestamp, gfx::Rect(visible_size), VideoFrameMetadata());

  // Frame buffer is returned to the device by dropping the |frame_info|.
}

}  // namespace media