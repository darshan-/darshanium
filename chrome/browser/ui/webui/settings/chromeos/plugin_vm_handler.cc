// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/chromeos/plugin_vm_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "chrome/browser/chromeos/file_manager/path_util.h"
#include "chrome/browser/chromeos/guest_os/guest_os_share_path.h"
#include "chrome/browser/chromeos/plugin_vm/plugin_vm_manager.h"
#include "chrome/browser/chromeos/plugin_vm/plugin_vm_manager_factory.h"
#include "chrome/browser/chromeos/plugin_vm/plugin_vm_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"

namespace chromeos {
namespace settings {

PluginVmHandler::PluginVmHandler(Profile* profile) : profile_(profile) {}

PluginVmHandler::~PluginVmHandler() = default;

void PluginVmHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getPluginVmSharedPathsDisplayText",
      base::BindRepeating(
          &PluginVmHandler::HandleGetPluginVmSharedPathsDisplayText,
          weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "removePluginVmSharedPath",
      base::BindRepeating(&PluginVmHandler::HandleRemovePluginVmSharedPath,
                          weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "removePluginVm",
      base::BindRepeating(&PluginVmHandler::HandleRemovePluginVm,
                          weak_ptr_factory_.GetWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "requestPluginVmInstallerView",
      base::BindRepeating(&PluginVmHandler::HandleRequestPluginVmInstallerView,
                          weak_ptr_factory_.GetWeakPtr()));
}

void PluginVmHandler::HandleGetPluginVmSharedPathsDisplayText(
    const base::ListValue* args) {
  AllowJavascript();
  CHECK_EQ(2U, args->GetSize());
  std::string callback_id = args->GetList()[0].GetString();

  base::ListValue texts;
  for (const auto& path : args->GetList()[1].GetList()) {
    texts.AppendString(file_manager::util::GetPathDisplayTextForSettings(
        profile_, path.GetString()));
  }
  ResolveJavascriptCallback(base::Value(callback_id), texts);
}

void PluginVmHandler::HandleRemovePluginVmSharedPath(
    const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  std::string vm_name = args->GetList()[0].GetString();
  std::string path = args->GetList()[1].GetString();

  guest_os::GuestOsSharePath::GetForProfile(profile_)->UnsharePath(
      vm_name, base::FilePath(path),
      /*unpersist=*/true,
      base::BindOnce(
          [](const std::string& path, bool result,
             const std::string& failure_reason) {
            if (!result) {
              LOG(ERROR) << "Error unsharing " << path << ": "
                         << failure_reason;
            }
          },
          path));
}

void PluginVmHandler::HandleRemovePluginVm(const base::ListValue* args) {
  CHECK_EQ(0U, args->GetSize());

  auto* manager = plugin_vm::PluginVmManagerFactory::GetForProfile(profile_);
  if (!manager) {
    LOG(ERROR) << "removePluginVm called from an invalid profile.";
    return;
  }
  manager->UninstallPluginVm();
}

void PluginVmHandler::HandleRequestPluginVmInstallerView(
    const base::ListValue* args) {
  CHECK(args->empty());

  if (plugin_vm::IsPluginVmEnabled(profile_)) {
    LOG(ERROR) << "requestPluginVmInstallerView called from a profile which "
                  "already has Plugin VM installed.";
    return;
  }
  plugin_vm::ShowPluginVmInstallerView(profile_);
}

}  // namespace settings
}  // namespace chromeos
