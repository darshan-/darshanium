// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/query_tiles/android/tile_conversion_bridge.h"

#include <memory>
#include <string>

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "components/query_tiles/jni_headers/TileConversionBridge_jni.h"

namespace upboarding {

using base::android::ConvertUTF8ToJavaString;
using base::android::ToJavaArrayOfStrings;

ScopedJavaLocalRef<jobject> CreateJavaTileAndMaybeAddToList(
    JNIEnv* env,
    ScopedJavaLocalRef<jobject> jlist,
    const Tile& tile) {
  ScopedJavaLocalRef<jobject> jchildren =
      Java_TileConversionBridge_createList(env);

  for (const auto& subtile : tile.sub_tiles)
    CreateJavaTileAndMaybeAddToList(env, jchildren, *subtile.get());

  std::vector<std::string> urls;
  for (const ImageMetadata& image : tile.image_metadatas)
    urls.push_back(image.url.spec());

  return Java_TileConversionBridge_createTileAndMaybeAddToList(
      env, jlist, ConvertUTF8ToJavaString(env, tile.id),
      ConvertUTF8ToJavaString(env, tile.display_text),
      ConvertUTF8ToJavaString(env, tile.accessibility_text),
      ConvertUTF8ToJavaString(env, tile.query_text),
      ToJavaArrayOfStrings(env, urls), jchildren);
}

ScopedJavaLocalRef<jobject> TileConversionBridge::CreateJavaTiles(
    JNIEnv* env,
    const std::vector<Tile>& tiles) {
  ScopedJavaLocalRef<jobject> jlist = Java_TileConversionBridge_createList(env);

  for (const auto& tile : tiles)
    CreateJavaTileAndMaybeAddToList(env, jlist, tile);

  return jlist;
}

}  // namespace upboarding
