// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/shell_integration.h"

#include "base/files/safe_base_name.h"
#include "base/notreached.h"
#include "base/logging.h"
#include "ohos/adapter/default_application/default_application_adapter.h"

namespace shell_integration {

bool SetAsDefaultBrowser() {
  return ohos::adapter::DefaultApplicationAdapter::StartSettingsAbility();
}

bool SetAsDefaultClientForScheme(const std::string& scheme) {
  NOTIMPLEMENTED_LOG_ONCE();
  return false;
}

std::u16string GetApplicationNameForScheme(const GURL& url) {
  NOTIMPLEMENTED_LOG_ONCE();
  return {};
}

DefaultWebClientState GetDefaultBrowser() {
  bool ret = ohos::adapter::DefaultApplicationAdapter::IsDefaultApplication();
  VLOG(1) << "GetDefaultBrowser ret: " << ret;
  if (ret) {
    return DefaultWebClientState::IS_DEFAULT;
  }
  return DefaultWebClientState::NOT_DEFAULT;
}

bool IsFirefoxDefaultBrowser() {
  NOTIMPLEMENTED_LOG_ONCE();
  return false;
}

DefaultWebClientState IsDefaultClientForScheme(const std::string& scheme) {
  return GetDefaultBrowser();
}

namespace internal {

DefaultWebClientSetPermission GetPlatformSpecificDefaultWebClientSetPermission(
    WebClientSetMethod method) {
  NOTIMPLEMENTED_LOG_ONCE();
  return SET_DEFAULT_UNATTENDED;
}

std::optional<base::SafeBaseName> GetUniqueWebShortcutFilename(
    const std::string& name) {
  NOTIMPLEMENTED_LOG_ONCE();
  return std::nullopt;
}

std::string GetDesktopFileContentsForUrlShortcut(
    const std::string& title,
    const GURL& url,
    const base::FilePath& icon_path,
    const base::FilePath& profile_path) {
  NOTIMPLEMENTED_LOG_ONCE();
  return "";
}

}  // namespace internal

}  // namespace shell_integration
