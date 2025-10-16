/*
 * Copyright (c) 2023-2025 Haitai FangYuan Co., Ltd.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
