// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.customtabs.content.CustomTabActivityNavigationController;
import org.chromium.chrome.browser.customtabs.features.toolbar.CustomTabToolbarCoordinator;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.ui.RootUiCoordinator;

/**
 * A {@link RootUiCoordinator} variant that controls UI for {@link BaseCustomTabActivity}.
 */
public class BaseCustomTabRootUiCoordinator extends RootUiCoordinator {
    private final CustomTabToolbarCoordinator mToolbarCoordinator;
    private final CustomTabActivityNavigationController mNavigationController;

    public BaseCustomTabRootUiCoordinator(ChromeActivity activity,
            ObservableSupplier<ShareDelegate> shareDelegateSupplier,
            CustomTabToolbarCoordinator customTabToolbarCoordinator,
            CustomTabActivityNavigationController customTabNavigationController,
            ActivityTabProvider tabProvider, ObservableSupplier<Profile> profileSupplier,
            ObservableSupplier<BookmarkBridge> bookmarkBridgeSupplier) {
        super(activity, null, shareDelegateSupplier, tabProvider, profileSupplier,
                bookmarkBridgeSupplier);

        mToolbarCoordinator = customTabToolbarCoordinator;
        mNavigationController = customTabNavigationController;
    }

    @Override
    protected void initializeToolbar() {
        super.initializeToolbar();

        mToolbarCoordinator.onToolbarInitialized(mToolbarManager);
        mNavigationController.onToolbarInitialized(mToolbarManager);
    }
}
