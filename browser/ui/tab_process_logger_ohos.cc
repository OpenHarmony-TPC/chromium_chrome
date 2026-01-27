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

#include "chrome/browser/ui/tab_process_logger_ohos.h"

#include "base/hash/hash.h"
#include "base/logging.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

WEB_CONTENTS_USER_DATA_KEY_IMPL(TabProcessLogger);

namespace {

// Find the index of this WebContents in the TabStripModel of all Browsers.
int GetTabIndexForWebContents(content::WebContents* contents) {
  if (!contents) {
    return -1;
  }

  for (Browser* browser : *BrowserList::GetInstance()) {
    if (!browser) {
      continue;
    }
    TabStripModel* tab_strip_model = browser->tab_strip_model();
    if (!tab_strip_model) {
      continue;
    }

    int index = tab_strip_model->GetIndexOfWebContents(contents);
    if (index != TabStripModel::kNoTab) {
      return index;
    }
  }
  return -1;
}

}  // namespace

TabProcessLogger::TabProcessLogger(content::WebContents* contents)
    : content::WebContentsObserver(contents),
      content::WebContentsUserData<TabProcessLogger>(*contents) {}

void TabProcessLogger::DidFinishNavigation(content::NavigationHandle* handle) {
  if (!handle->IsInMainFrame() || !handle->HasCommitted()) {
    return;
  }

  content::RenderFrameHost* render_frame_host = handle->GetRenderFrameHost();
  if (!render_frame_host) {
    return;
  }
  content::RenderProcessHost* render_process_host = render_frame_host->GetProcess();
  if (!render_process_host || !render_process_host->IsReady()) {
    return;
  }

  auto process_os_id = render_process_host->GetProcess().Pid();

  const int tab_index = GetTabIndexForWebContents(web_contents());
  const int tab_order = (tab_index >= 0) ? (tab_index + 1) : -1;

  const GURL url = handle->GetURL();
  const std::string& url_str = url.spec();
  const uint32_t url_hash = base::PersistentHash(url_str);

  LOG(INFO) << " render process id=" << process_os_id
            << " TabOrderInWindow=" << tab_order
            << " URLHash=0x" << std::hex << url_hash << std::dec;
}