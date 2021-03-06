// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/tab_strip/tab_strip_ui_util.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/webui/tab_strip/tab_strip_ui.h"
#include "components/tab_groups/tab_group_id.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/clipboard/clipboard_format_type.h"
#include "ui/base/clipboard/custom_data_helper.h"
#include "ui/base/dragdrop/os_exchange_data.h"

namespace tab_strip_ui {

base::Optional<tab_groups::TabGroupId> GetTabGroupIdFromString(
    TabGroupModel* tab_group_model,
    std::string group_id_string) {
  for (tab_groups::TabGroupId candidate : tab_group_model->ListTabGroups()) {
    if (candidate.ToString() == group_id_string) {
      return base::Optional<tab_groups::TabGroupId>{candidate};
    }
  }

  return base::nullopt;
}

Browser* GetBrowserWithGroupId(Profile* profile, std::string group_id_string) {
  for (auto* browser : *BrowserList::GetInstance()) {
    if (browser->profile() != profile) {
      continue;
    }

    base::Optional<tab_groups::TabGroupId> group_id = GetTabGroupIdFromString(
        browser->tab_strip_model()->group_model(), group_id_string);
    if (group_id.has_value()) {
      return browser;
    }
  }

  return nullptr;
}

void MoveTabAcrossWindows(Browser* source_browser,
                          int from_index,
                          Browser* target_browser,
                          int to_index,
                          base::Optional<tab_groups::TabGroupId> to_group_id) {
  bool was_active =
      source_browser->tab_strip_model()->active_index() == from_index;
  bool was_pinned = source_browser->tab_strip_model()->IsTabPinned(from_index);

  std::unique_ptr<content::WebContents> detached_contents =
      source_browser->tab_strip_model()->DetachWebContentsAt(from_index);

  int add_types = TabStripModel::ADD_NONE;
  if (was_active) {
    add_types |= TabStripModel::ADD_ACTIVE;
  }
  if (was_pinned) {
    add_types |= TabStripModel::ADD_PINNED;
  }

  target_browser->tab_strip_model()->InsertWebContentsAt(
      to_index, std::move(detached_contents), add_types, to_group_id);
}

bool DropTabsInNewBrowser(Browser* new_browser,
                          const ui::OSExchangeData& drop_data) {
  base::Pickle pickle;
  drop_data.GetPickledData(ui::ClipboardFormatType::GetWebCustomDataType(),
                           &pickle);

  base::string16 tab_id_str;
  ui::ReadCustomDataForType(pickle.data(), pickle.size(),
                            base::ASCIIToUTF16(kWebUITabIdDataType),
                            &tab_id_str);
  if (tab_id_str.empty())
    return false;

  // |tab_id_str| should contain the extension tab id as a string.
  int tab_id = -1;
  if (!base::StringToInt(tab_id_str, &tab_id))
    return false;

  Browser* source_browser = nullptr;
  int source_index = -1;
  if (!extensions::ExtensionTabUtil::GetTabById(
          tab_id, new_browser->profile(), /* include_incognito = */ false,
          &source_browser, /* tab_strip = */ nullptr,
          /* contents = */ nullptr, &source_index)) {
    return false;
  }

  MoveTabAcrossWindows(source_browser, source_index, new_browser, 0,
                       base::nullopt);
  new_browser->tab_strip_model()->ActivateTabAt(0);
  return true;
}

}  // namespace tab_strip_ui
