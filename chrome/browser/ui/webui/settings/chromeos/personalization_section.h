// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_PERSONALIZATION_SECTION_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_PERSONALIZATION_SECTION_H_

#include "chrome/browser/ui/webui/settings/chromeos/os_settings_section.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace content {
class WebUIDataSource;
}  // namespace content

namespace chromeos {
namespace settings {

// Provides UI strings and search tags for Personalization settings. Search tags
// are only added when not in guest mode, and Ambient mode settings are added
// depending on whether the feature is allowed and enabled.
class PersonalizationSection : public OsSettingsSection {
 public:
  PersonalizationSection(Profile* profile,
                         Delegate* per_page_delegate,
                         PrefService* pref_service);
  ~PersonalizationSection() override;

 private:
  // OsSettingsSection:
  void AddLoadTimeData(content::WebUIDataSource* html_source) override;
  void AddHandlers(content::WebUI* web_ui) override;

  // ash::AmbientModeService::Observer:
  void OnAmbientModeEnabledStateChanged();

  PrefService* pref_service_;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace settings
}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_PERSONALIZATION_SECTION_H_
