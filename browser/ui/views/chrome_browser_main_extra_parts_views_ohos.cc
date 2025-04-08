/*
 * Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "chrome/browser/ui/views/chrome_browser_main_extra_parts_views_ohos.h"

#include "base/logging.h"
#include "ohos/adapter/native_theme/native_theme_adapter.h"
#include "ui/native_theme/native_theme.h"

ChromeBrowserMainExtraPartsViewsOHOS::ChromeBrowserMainExtraPartsViewsOHOS() =
    default;

ChromeBrowserMainExtraPartsViewsOHOS::~ChromeBrowserMainExtraPartsViewsOHOS() =
    default;

void ChromeBrowserMainExtraPartsViewsOHOS::ToolkitInitialized() {
  ChromeBrowserMainExtraPartsViews::ToolkitInitialized();

  std::shared_ptr<ohos::adapter::native_theme::ThemeSourceEventCallback>
      theme_source_event_callback =
          std::make_shared<ui::ThemeSourceEventCallbackImpl>();
  if (!theme_source_event_callback) {
    LOG(ERROR) << "ChromeBrowserMainExtraPartsViewsOHOS::ToolkitInitialized "
               << "create theme_source_event_callback callback failed";
    return;
  }
  ohos::adapter::native_theme::NativeThemeAdapter::GetInstance()
      .RegisterThemeSourceEvent(theme_source_event_callback);
  ohos::adapter::native_theme::NativeThemeAdapter::GetInstance()
      .NotifyThemeSourceEvent(
          ohos::adapter::native_theme::NativeThemeAdapter ::GetInstance()
              .GetSystemThemeSource());
}
 