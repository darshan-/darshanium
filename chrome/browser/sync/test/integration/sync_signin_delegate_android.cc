// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/sync_signin_delegate_android.h"

#include "chrome/browser/sync/test/integration/sync_test_utils_android.h"

void SyncSigninDelegateAndroid::SigninFake(Profile* profile,
                                           const std::string& username) {
  sync_test_utils_android::SetUpTestAccountAndSignIn();
}

bool SyncSigninDelegateAndroid::SigninUI(Profile* profile,
                                         const std::string& username,
                                         const std::string& password) {
  return false;
}

bool SyncSigninDelegateAndroid::ConfirmSigninUI(Profile* profile) {
  return true;
}
