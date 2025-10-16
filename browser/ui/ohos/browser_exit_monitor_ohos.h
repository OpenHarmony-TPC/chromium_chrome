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

#ifndef CHROME_BROWSER_UI_OHOS_BROWSER_EXIT_MONITOR_OHOS_H_
#define CHROME_BROWSER_UI_OHOS_BROWSER_EXIT_MONITOR_OHOS_H_

#include <map>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "ui/ozone/platform/ohos/host/ohos_window_observer.h"
#include "ui/ozone/platform/ohos/host/ohos_window_manager.h"
#include "ui/ozone/public/platform_window_manager.h"

class Browser;

namespace chrome {

// unknown,wait  ---  prepare wait till timeout, then continue exit
// block,cancel  ---  prepare stop exit
// close         ---  prepare continue exit
// closed        ---  prepare continue exit, destroy continue exit
enum class ExitStateOhos { UNKNOWN = 0, WAIT, CLOSE, CLOSED, BLOCK };

// download      ---  block browser exit
// beforeunload  ---  block tab exit
enum class ExitTaskOhos { DOWNLOAD = 0, BEFOREUNLOAD, TOTAL };

class BrowserExitMonitorOhos : public BrowserListObserver,
                               public ui::OhosWindowObserver {
 public:
  static BrowserExitMonitorOhos& GetInstance();

  void Register(ui::PlatformWindowManager* window_manager);

  // BrowserListObserver
  void OnBrowserCloseCancelled(Browser* browser,
                               BrowserClosingStatus reason) override;
  void OnBrowserClosing(Browser* browser) override;
  void OnBrowserRemoved(Browser* browser) override;

  // ui::OhosWindowObserver
  void OnWindowRemoved(ui::OhosWindow* window) override;
  void OnWindowCloseEvent(ui::OhosWindow* window) override;

  void Reset();
  void ResetBrowserState(int32_t browser_id);
  void ResetAppState();

  void UpdateBrowserExitState(int32_t browser_id,
                              ExitTaskOhos task,
                              ExitStateOhos state,
                              bool notify = true);
  void UpdateAppExitState(ExitTaskOhos task,
                          ExitStateOhos state,
                          bool notify = true);

  ExitStateOhos GetBrowserExitState(int browser_id);
  ExitStateOhos GetAppExitState();

  std::string ToString();

 private:
  friend class base::NoDestructor<BrowserExitMonitorOhos>;

  bool init_for_register_ = false;

  std::unordered_map<int32_t, std::unordered_map<ExitTaskOhos, ExitStateOhos>>
      browser_pre_exit_state_;
  std::unordered_map<ExitTaskOhos, ExitStateOhos> app_pre_exit_state_;

  // private constructor
  BrowserExitMonitorOhos();
  // delete copy constructor
  BrowserExitMonitorOhos(const BrowserExitMonitorOhos&) = delete;
  // delete copy assignment operator
  BrowserExitMonitorOhos& operator=(const BrowserExitMonitorOhos&) = delete;

  ~BrowserExitMonitorOhos();

  bool notify_control_;
  ExitStateOhos GetExitState(
      std::unordered_map<ExitTaskOhos, ExitStateOhos>& task_state_mp);
  void NotifyBrowserExitState(int32_t id);
  void NotifyAppExitState();
  void NotifyResetState();

  base::WeakPtr<BrowserExitMonitorOhos> AsWeakPtr();
  base::WeakPtr<const BrowserExitMonitorOhos> AsWeakPtr() const;

  // The following factory is used to close the frame at a later time.
  base::WeakPtrFactory<BrowserExitMonitorOhos> weak_factory_{this};
};  // class BrowserExitMonitorOhos

}  // namespace chrome

#endif  // CHROME_BROWSER_UI_OHOS_BROWSER_EXIT_MONITOR_OHOS_H_
