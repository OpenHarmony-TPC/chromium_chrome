/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
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

#include "chrome/browser/ui/views/frame/browser_frame_view_ohos.h"

#include "ui/aura/window_tree_host_platform.h"
#include "ui/display/display.h"
#include "ui/display/screen_ohos.h"
#include "ui/ozone/platform/ohos/host/ohos_toplevel_window.h"

namespace {
constexpr int kDefaultCaptionButtonWidth = 140;
}  // namespace

BrowserFrameViewOhos::BrowserFrameViewOhos(BrowserFrame* frame,
                                           BrowserView* browser_view,
                                           OpaqueBrowserFrameViewLayout* layout)
    : OpaqueBrowserFrameView(frame, browser_view, layout) {}

gfx::Rect BrowserFrameViewOhos::GetBoundsForTabStripRegion(
    const gfx::Size& tabstrip_minimum_size) const {
  const int x = CaptionButtonsOnLeadingEdge() ? CaptionButtonsRegionWidth() : 0;
  int end_x = width();
  if (!CaptionButtonsOnLeadingEdge()) {
    end_x = std::min(width() - CaptionButtonsRegionWidth(), end_x);
  }
  return gfx::Rect(0, 0, std::max(0, end_x - x),
                   tabstrip_minimum_size.height());
}

bool BrowserFrameViewOhos::CaptionButtonsOnLeadingEdge() const {
  return base::i18n::IsRTL();
}

int BrowserFrameViewOhos::CaptionButtonsRegionWidth() const {
  aura::WindowTreeHostPlatform* host =
      static_cast<aura::WindowTreeHostPlatform*>(
          browser_view()->GetNativeWindow()->GetHost());
  if (host && host->platform_window()) {
    ui::OhosToplevelWindow* toplevel_window =
        static_cast<ui::OhosToplevelWindow*>(host->platform_window());
    gfx::Rect caption_button_rect = toplevel_window->GetCaptionButtonRect();
    display::Display display =
        display::Screen::GetScreen()->GetDisplayNearestWindow(
            browser_view()->GetNativeWindow());
    return display::ohos::ScreenOhos::ConvertPixelToDIP(display,
                                                        caption_button_rect)
        .width();
  }
  return kDefaultCaptionButtonWidth;
}
