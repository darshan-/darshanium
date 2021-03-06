// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_QUERY_TILES_ANDROID_TILE_CONVERSION_BRIDGE_H_
#define COMPONENTS_QUERY_TILES_ANDROID_TILE_CONVERSION_BRIDGE_H_

#include <vector>

#include "base/android/jni_android.h"
#include "components/query_tiles/tile.h"

using base::android::ScopedJavaLocalRef;

namespace upboarding {

// Helper class providing tile conversion utility methods between C++ and Java.
class TileConversionBridge {
 public:
  static ScopedJavaLocalRef<jobject> CreateJavaTiles(
      JNIEnv* env,
      const std::vector<Tile>& tiles);
};

}  // namespace upboarding

#endif  // COMPONENTS_QUERY_TILES_ANDROID_TILE_CONVERSION_BRIDGE_H_
