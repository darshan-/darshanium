// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;

import androidx.fragment.app.Fragment;

import org.junit.Assert;

/**
 * Activity test rule that launch {@link SettingsActivity} in tests.
 *
 * Noting that the activity is not starting after the test rule created. The user have to call
 * {@link #startSettingsActivity()} explicitly to launch the settings activity.
 *
 * @param <T> Fragment that will be attached to the SettingsActivity.
 */
public class SettingsActivityTestRule<T extends Fragment>
        extends ActivityTestRule<SettingsActivity> {
    private final Class<T> mFragmentClass;

    /**
     * Create the settings activity test rule with an specific fragment class.
     * @param fragmentClass Fragment that will be attached after the activity starts.
     */
    public SettingsActivityTestRule(Class<T> fragmentClass) {
        this(fragmentClass, false);
    }

    /**
     * Create the settings activity test rule with an specific fragment class.
     * @param fragmentClass Fragment that will be attached after the activity starts.
     * @param initialTouchMode Whether in touch mode after the activity starts.
     */
    public SettingsActivityTestRule(Class<T> fragmentClass, boolean initialTouchMode) {
        super(SettingsActivity.class, initialTouchMode, false);
        mFragmentClass = fragmentClass;
    }

    /**
     * Launches the settings activity with the specified fragment.
     * @return The activity that just started.
     */
    public SettingsActivity startSettingsActivity() {
        return startSettingsActivity(null);
    }

    /**
     * Launches the settings activity with the specified fragment and arguments.
     * @param fragmentArgs A bundle of additional fragment arguments.
     * @return The activity that just started.
     */
    public SettingsActivity startSettingsActivity(Bundle fragmentArgs) {
        Context context = InstrumentationRegistry.getTargetContext();
        SettingsLauncher settingsLauncher = new SettingsLauncherImpl();
        Intent intent = settingsLauncher.createSettingsActivityIntent(
                context, mFragmentClass.getName(), fragmentArgs);
        SettingsActivity activity = super.launchActivity(intent);
        Assert.assertNotNull(activity);

        return activity;
    }

    /**
     * @return The fragment attached to the SettingsActivity.
     */
    public T getFragment() {
        Assert.assertNotNull("#getFragment is called before activity launch.", getActivity());

        Fragment fragment = getActivity().getMainFragment();
        Assert.assertNotNull(fragment);
        return (T) fragment;
    }
}
