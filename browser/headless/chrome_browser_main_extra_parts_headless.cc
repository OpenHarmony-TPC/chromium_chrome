// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/headless/chrome_browser_main_extra_parts_headless.h"

#include "chrome/browser/headless/headless_mode_util.h"
#if !BUILDFLAG(IS_OHOS)
#include "components/headless/clipboard/headless_clipboard.h"
#endif  // !BUILDFLAG(IS_OHOS)

namespace headless {

ChromeBrowserMainExtraPartsHeadless::ChromeBrowserMainExtraPartsHeadless() =
    default;
ChromeBrowserMainExtraPartsHeadless::~ChromeBrowserMainExtraPartsHeadless() =
    default;

void ChromeBrowserMainExtraPartsHeadless::PreBrowserStart() {
  // Headless mode uses platform independent clipboard which we need to set
  // before generic clipboard implementation sets the platform specific one.
#if !BUILDFLAG(IS_OHOS)
  if (IsHeadlessMode()) {
    SetHeadlessClipboardForCurrentThread();
  }
#endif  // !BUILDFLAG(IS_OHOS)
}

}  // namespace headless
