// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_THEMES_THEME_SERVICE_OHOS_H_
#define CHROME_BROWSER_THEMES_THEME_SERVICE_OHOS_H_

#include "chrome/browser/themes/theme_service.h"
#include "ohos/adapter/native_theme/native_theme_adapter.h"

using OhosColorMode = ohos::adapter::native_theme::OhosColorMode;

// A subclass of ThemeService that manages the CustomThemeSupplier which
// provides the native OHOS theme.
class ThemeServiceOhos : public ThemeService {
 public:
  using ThemeService::ThemeService;

  ThemeServiceOhos(const ThemeServiceOhos&) = delete;
  ThemeServiceOhos& operator=(const ThemeServiceOhos&) = delete;

  ~ThemeServiceOhos() override;

  void InitFromPrefs() override;
  void SetBrowserColorScheme(BrowserColorScheme color_scheme) override;

 private:
  OhosColorMode ConvertBrowserColorSchemeToOhosColorMode(
      BrowserColorScheme color_scheme);
};

#endif  // CHROME_BROWSER_THEMES_THEME_SERVICE_OHOS_H_
