// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_OS_INTEGRATION_WEB_APP_SHORTCUT_OHOS_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_OS_INTEGRATION_WEB_APP_SHORTCUT_OHOS_H_

#include "chrome/browser/web_applications/os_integration/web_app_shortcut.h"

namespace web_app {

namespace internals {

// Sanitizes |name| and returns a version of it that is safe to use as an
// on-disk file name.
base::FilePath GetSanitizedFileName(const std::u16string& name);

// Saves |image| to |icon_file| if the file is outdated. Returns true if
// icon_file is up to date or successfully updated.
// If |refresh_shell_icon_cache| is true, the shell's icon cache will be
// refreshed, ensuring the correct icon is displayed, but causing a flicker.
// Refreshing the icon cache is not necessary on shortcut creation as the shell
// will be notified when the shortcut is created.
// Creates the parent dir of icon_file, if it doesn't exist.
bool CheckAndSaveIcon(const base::FilePath& icon_file,
                      const gfx::ImageFamily& image,
                      bool refresh_shell_icon_cache);

base::FilePath GetIconFilePath(const base::FilePath& web_app_path,
                               const std::u16string& title);

}  // namespace internals

}  // namespace web_app

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_OS_INTEGRATION_WEB_APP_SHORTCUT_OHOS_H_
