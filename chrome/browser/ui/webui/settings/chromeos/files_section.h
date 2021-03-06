// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_FILES_SECTION_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_FILES_SECTION_H_

#include "chrome/browser/ui/webui/settings/chromeos/os_settings_section.h"

namespace content {
class WebUIDataSource;
}  // namespace content

namespace chromeos {
namespace settings {

// Provides UI strings and search tags for Files settings.
class FilesSection : public OsSettingsSection {
 public:
  FilesSection(Profile* profile, Delegate* per_page_delegate);
  ~FilesSection() override;

 private:
  // OsSettingsSection:
  void AddLoadTimeData(content::WebUIDataSource* html_source) override;
  void AddHandlers(content::WebUI* web_ui) override;
};

}  // namespace settings
}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_FILES_SECTION_H_
