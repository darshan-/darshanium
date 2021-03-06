// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import androidx.annotation.Nullable;

import org.chromium.chrome.browser.omnibox.LocationBar.OmniboxFocusReason;
import org.chromium.chrome.browser.omnibox.UrlFocusChangeListener;
import org.chromium.chrome.browser.omnibox.voice.VoiceRecognitionHandler;
import org.chromium.chrome.browser.ui.native_page.NativePage;

/**
 * Handles user interaction with the fakebox (the URL bar in the NTP and tasks surface).
 */
public interface FakeboxDelegate {
    /**
     * Signal a {@link UrlBar} focus change request.
     * @param shouldBeFocused Whether the focus should be requested or cleared. True requests
     *         focus
     *        and False clears focus.
     * @param pastedText The given pasted text when focus, which could be null.
     * @param reason The given reason.
     */
    void setUrlBarFocus(
            boolean shouldBeFocused, @Nullable String pastedText, @OmniboxFocusReason int reason);

    /**
     * Performs a search query on the current {@link Tab}.  This calls {@link
     * TemplateUrlService#getUrlForSearchQuery(String)} to get a url based on {@code query} and
     * loads that url in the current {@link Tab}.
     * @param query The {@link String} that represents the text query that should be searched for.
     */
    void performSearchQuery(String query);

    /**
     * @return Whether the URL bar is currently focused.
     */
    boolean isUrlBarFocused();

    /**
     * @return whether the provided native page is the one currently displayed to the user.
     */
    boolean isCurrentPage(NativePage nativePage);

    /**
     * Get the {@link VoiceRecognitionHandler}.
     * @return the {@link VoiceRecognitionHandler}
     */
    VoiceRecognitionHandler getVoiceRecognitionHandler();

    /**
     * Adds a URL focus change listener that will be notified when the URL gains or loses focus.
     * @param listener The listener to be registered.
     */
    default void addUrlFocusChangeListener(UrlFocusChangeListener listener) {}

    /**
     * Removes a URL focus change listener that was previously added.
     * @param listener The listener to be removed.
     */
    default void removeUrlFocusChangeListener(UrlFocusChangeListener listener) {}
}
