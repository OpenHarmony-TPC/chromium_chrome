// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/themes/theme_service_ohos.h"

#include "base/notreached.h"
#include "ohos/adapter/native_theme/native_theme_adapter.h"

using NativeThemeAdapter = ohos::adapter::native_theme::NativeThemeAdapter;

ThemeServiceOhos::~ThemeServiceOhos() = default;

void ThemeServiceOhos::InitFromPrefs() {
  ThemeService::InitFromPrefs();
  BrowserColorScheme color_scheme = GetBrowserColorScheme();
  OhosColorMode app_color_mode =
      ConvertBrowserColorSchemeToOhosColorMode(color_scheme);
  NativeThemeAdapter::GetInstance().SetAppColorMode(app_color_mode);
}

void ThemeServiceOhos::SetBrowserColorScheme(
    ThemeService::BrowserColorScheme color_scheme) {
  ThemeService::SetBrowserColorScheme(color_scheme);
  OhosColorMode app_color_mode =
      ConvertBrowserColorSchemeToOhosColorMode(color_scheme);
  NativeThemeAdapter::GetInstance().SetAppColorMode(app_color_mode);
}

OhosColorMode ThemeServiceOhos::ConvertBrowserColorSchemeToOhosColorMode(
    BrowserColorScheme color_scheme) {
  switch (color_scheme) {
    case ThemeService::BrowserColorScheme::kLight:
      return OhosColorMode::COLOR_MODE_LIGHT;
    case ThemeService::BrowserColorScheme::kDark:
      return OhosColorMode::COLOR_MODE_DARK;
    case ThemeService::BrowserColorScheme::kSystem:
      return OhosColorMode::COLOR_MODE_NOT_SET;
    default:
      NOTREACHED();
  }
}
