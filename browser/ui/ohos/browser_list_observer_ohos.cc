/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include "chrome/browser/ui/ohos/browser_list_observer_ohos.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "ohos/adapter/context/context_adapter.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"

namespace chrome {

BrowserListObserverOhos::BrowserListObserverOhos() {
  BrowserList::AddObserver(this);
}

BrowserListObserverOhos::~BrowserListObserverOhos() {
  BrowserList::RemoveObserver(this);
}

void BrowserListObserverOhos::OnBrowserSetLastActive(Browser* browser) {
  auto widget_id = GetWidgetIdFromBrowser(browser);
  if (widget_id != gfx::kNullAcceleratedWidget) {
    ohos::adapter::ContextAdapter::GetInstance().SetLastActiveWidgetId(widget_id);
  }
}

gfx::AcceleratedWidget BrowserListObserverOhos::GetWidgetIdFromBrowser(Browser* browser) {
  if (!browser) {
    return gfx::kNullAcceleratedWidget;
  }
  auto browser_window = browser->window();
  if (!browser_window) {
    return gfx::kNullAcceleratedWidget;
  }
  auto native_window = browser_window->GetNativeWindow();
  if (!native_window) {
    return gfx::kNullAcceleratedWidget;
  }
  auto window_tree_host = native_window->GetHost();
  if (!window_tree_host) {
    return gfx::kNullAcceleratedWidget;
  }
  return window_tree_host->GetAcceleratedWidget();
}

}  // namespace chrome