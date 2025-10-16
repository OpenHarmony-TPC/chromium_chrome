/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "chrome/browser/ui/ohos/browser_exit_monitor_ohos.h"

#include "base/logging.h"
#include "base/time/time.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "ohos/adapter/browser/browser_adapter.h"
#include "ui/ozone/platform/ohos/host/ohos_window.h"

namespace chrome {

// static
BrowserExitMonitorOhos& BrowserExitMonitorOhos::GetInstance() {
  static base::NoDestructor<BrowserExitMonitorOhos> instance;
  return *instance;
}

void BrowserExitMonitorOhos::Register(ui::PlatformWindowManager* window_manager) {
  if (init_for_register_) {
    LOG(WARNING) << "BrowserExitMonitorOhos is registered";
    return;
  }

  // Register browser list observer
  BrowserList::AddObserver(this);
  LOG(INFO) << "BrowserExitMonitorOhos registed into BrowserList";

  bool regist_window_manager = false;
  if (window_manager) {
    ui::OhosWindowManager* ohos_window_manager =
        static_cast<ui::OhosWindowManager*>(window_manager);
    if (ohos_window_manager) {
      ohos_window_manager->AddObserver(this);
      regist_window_manager = true;
      LOG(INFO) << "BrowserExitMonitorOhos registed into OhosWindowMnager";
    }
  }
  if (!regist_window_manager) {
    LOG(ERROR) << "BrowserExitMonitorOhos registed into OhosWindowMnager fail";
  }

  init_for_register_ = true;
}

void BrowserExitMonitorOhos::OnBrowserCloseCancelled(
    Browser* browser,
    BrowserClosingStatus reason) {
  if (!browser) {
    return;
  }
  int32_t browser_id = browser->GetAcceleratedWidget();
  ExitStateOhos download_state = ExitStateOhos::WAIT;
  ExitStateOhos unload_state = ExitStateOhos::WAIT;
  if (reason == BrowserClosingStatus::kDeniedByUser ||
      reason == BrowserClosingStatus::kDeniedByPolicy) {
    download_state = ExitStateOhos::BLOCK;
  } else if (reason == BrowserClosingStatus::kPermitted) {
    download_state = ExitStateOhos::CLOSE;
    unload_state = ExitStateOhos::CLOSE;
  } else if (reason == BrowserClosingStatus::kDeniedUnloadHandlersNeedTime) {
    download_state = ExitStateOhos::CLOSE;
    unload_state = ExitStateOhos::WAIT;
  }
  LOG(INFO) << "BrowserExitMonitorOhos on close cancelled id:" << browser_id
            << " download_state:" << static_cast<int>(download_state)
            << " unload_state:" << static_cast<int>(unload_state);
  UpdateBrowserExitState(browser_id, ExitTaskOhos::DOWNLOAD, download_state,
                         false);
  UpdateBrowserExitState(browser_id, ExitTaskOhos::BEFOREUNLOAD, unload_state);
}

void BrowserExitMonitorOhos::OnBrowserClosing(Browser* browser) {
  int32_t browser_id = browser->GetAcceleratedWidget();
  UpdateBrowserExitState(browser_id, ExitTaskOhos::DOWNLOAD, ExitStateOhos::CLOSE,
                         false);
  UpdateBrowserExitState(browser_id, ExitTaskOhos::BEFOREUNLOAD, ExitStateOhos::CLOSE);
}
void BrowserExitMonitorOhos::OnBrowserRemoved(Browser* browser) {
  int32_t browser_id = browser->GetAcceleratedWidget();
  UpdateBrowserExitState(browser_id, ExitTaskOhos::DOWNLOAD, ExitStateOhos::CLOSED,
                         false);
  UpdateBrowserExitState(browser_id, ExitTaskOhos::BEFOREUNLOAD, ExitStateOhos::CLOSED);
}

// ui::OhosWindowObserver
void BrowserExitMonitorOhos::OnWindowRemoved(ui::OhosWindow* window) {
  if (!window) {
    return;
  }
  int32_t browser_id = window->GetWidget();
  UpdateBrowserExitState(browser_id, ExitTaskOhos::DOWNLOAD, ExitStateOhos::CLOSED,
                         false);
  UpdateBrowserExitState(browser_id, ExitTaskOhos::BEFOREUNLOAD, ExitStateOhos::CLOSED);
}

void BrowserExitMonitorOhos::OnWindowCloseEvent(ui::OhosWindow* window) {
  if (!window) {
    return;
  }
  int32_t browser_id = window->GetWidget();
  ResetBrowserState(browser_id);
}

base::WeakPtr<BrowserExitMonitorOhos> BrowserExitMonitorOhos::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

base::WeakPtr<const BrowserExitMonitorOhos> BrowserExitMonitorOhos::AsWeakPtr()
    const {
  return weak_factory_.GetWeakPtr();
}

BrowserExitMonitorOhos::BrowserExitMonitorOhos() {
  notify_control_ = true;
}

BrowserExitMonitorOhos::~BrowserExitMonitorOhos() = default;

void BrowserExitMonitorOhos::Reset() {
  app_pre_exit_state_.clear();
  browser_pre_exit_state_.clear();
  NotifyResetState();
}

void BrowserExitMonitorOhos::ResetBrowserState(int32_t browser_id) {
  browser_pre_exit_state_.erase(browser_id);
  NotifyBrowserExitState(browser_id);
}

void BrowserExitMonitorOhos::ResetAppState() {
  app_pre_exit_state_.clear();
  NotifyAppExitState();
}

void BrowserExitMonitorOhos::UpdateBrowserExitState(int32_t browser_id,
                                                    ExitTaskOhos task,
                                                    ExitStateOhos state,
                                                    bool notify) {
  if (browser_id < 0 || state == ExitStateOhos::UNKNOWN) {
    return;
  }
  auto& state_mp = browser_pre_exit_state_[browser_id];
  auto iter = state_mp.find(task);
  if (iter == state_mp.end()) {
    state_mp.emplace(task, state);
  } else {
    if (iter->second != ExitStateOhos::BLOCK ||
        state == ExitStateOhos::CLOSED) {
      iter->second = state;
      LOG(INFO) << "BrowserExitMonitorOhos update browser id:" << browser_id
                << " task:" << static_cast<int>(task)
                << " state:" << static_cast<int>(state);
    }
  }
  if (notify) {
    NotifyBrowserExitState(browser_id);
  }
}

void BrowserExitMonitorOhos::UpdateAppExitState(ExitTaskOhos task,
                                                ExitStateOhos state, bool notify) {
  if (state == ExitStateOhos::UNKNOWN) {
    return;
  }
  auto iter = app_pre_exit_state_.find(task);
  if (iter == app_pre_exit_state_.end()) {
    app_pre_exit_state_.emplace(task, state);
  } else {
    if (iter->second != ExitStateOhos::BLOCK ||
        state == ExitStateOhos::CLOSED) {
      iter->second = state;
      LOG(INFO) << "BrowserExitMonitorOhos update app task:"
                << static_cast<int>(task)
                << " state:" << static_cast<int>(state);
    }
  }
  if (notify) {
    NotifyAppExitState();
  }
}

ExitStateOhos BrowserExitMonitorOhos::GetExitState(
    std::unordered_map<ExitTaskOhos, ExitStateOhos>& task_state_mp) {
  if (task_state_mp.size() < static_cast<size_t>(ExitTaskOhos::TOTAL)) {
    return ExitStateOhos::WAIT;
  }
  ExitStateOhos state = ExitStateOhos::CLOSE;
  // one block, return block
  // one closed, return closed
  // all close, return close
  // otherwise, return wait
  for (const auto& task_iter : task_state_mp) {
    if (task_iter.second == ExitStateOhos::BLOCK) {
      return ExitStateOhos::BLOCK;
    }
  }
  for (const auto& task_iter : task_state_mp) {
    if (task_iter.second == ExitStateOhos::CLOSED) {
      return ExitStateOhos::CLOSED;
    }
  }
  for (const auto& task_iter : task_state_mp) {
    if (task_iter.second != ExitStateOhos::CLOSE) {
      return ExitStateOhos::WAIT;
    }
  }

  // all close
  return state;
}

ExitStateOhos BrowserExitMonitorOhos::GetBrowserExitState(int32_t browser_id) {
  if (browser_id < 0) {
    return ExitStateOhos::UNKNOWN;
  }
  const auto iter = browser_pre_exit_state_.find(browser_id);
  if (iter == browser_pre_exit_state_.end()) {
    return ExitStateOhos::UNKNOWN;
  }

  return GetExitState(iter->second);
}

ExitStateOhos BrowserExitMonitorOhos::GetAppExitState() {
  return GetExitState(app_pre_exit_state_);
}

ohos::adapter::BrowserCloseResponse ExitState2CloseResponse(ExitStateOhos state) {
  ohos::adapter::BrowserCloseResponse resp =
      ohos::adapter::BrowserCloseResponse::kUndetermined;
  if (state == ExitStateOhos::BLOCK) {
    resp = ohos::adapter::BrowserCloseResponse::kClosingInterrupt;
  } else if (state == ExitStateOhos::CLOSE) {
    resp = ohos::adapter::BrowserCloseResponse::kClosingContinue;
  } else if (state == ExitStateOhos::CLOSED) {
    resp = ohos::adapter::BrowserCloseResponse::kClosed;
  }
  return resp;
}

void BrowserExitMonitorOhos::NotifyBrowserExitState(int32_t browser_id) {
  if (!notify_control_) {
    return;
  }
  LOG(INFO) << "BrowserExitMonitorOhos exit_info " << this->ToString();
  ExitStateOhos state = GetBrowserExitState(browser_id);
  ohos::adapter::BrowserCloseResponse resp = ExitState2CloseResponse(state);

  ohos::adapter::BrowserAdapter::GetInstance().SetBrowserCloseResponse(
      browser_id, resp);
  LOG(INFO) << "BrowserExitMonitorOhos set browser close resp id:" << browser_id
             << " resp:" << static_cast<int>(resp)
             << " state:" << static_cast<int>(state);
}

void BrowserExitMonitorOhos::NotifyAppExitState() {
  if (!notify_control_) {
    return;
  }
  ExitStateOhos state = GetAppExitState();
  ohos::adapter::BrowserCloseResponse resp = ExitState2CloseResponse(state);

  ohos::adapter::BrowserAdapter::GetInstance().SetAppCloseResponse(resp);
  LOG(INFO) << "BrowserExitMonitorOhos set app close resp:" << static_cast<int>(resp)
             << " state:" << static_cast<int>(state);
}

void BrowserExitMonitorOhos::NotifyResetState() {
  if (!notify_control_) {
    return;
  }
  ohos::adapter::BrowserAdapter::GetInstance().ResetCloseResponse();
}

std::string BrowserExitMonitorOhos::ToString() {
  std::ostringstream out;
  out << "app_exit_state: ";
  for (auto iter : app_pre_exit_state_) {
    out << static_cast<int>(iter.first);
    out << "_";
    out << static_cast<int>(iter.second);
  }
  out << " browser_exit_states: ";
  for (auto iter : browser_pre_exit_state_) {
    out << "browser";
    out << iter.first;
    out << ":";
    for (auto iter2 : iter.second) {
      out << static_cast<int>(iter2.first);
      out << "_";
      out << static_cast<int>(iter2.second);
      out << ",";
    }
    out << " ";
  }
  return out.str();
}

}  // namespace chrome
