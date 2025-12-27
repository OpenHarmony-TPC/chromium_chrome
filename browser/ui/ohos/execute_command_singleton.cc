// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ohos/execute_command_singleton.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/ohos/browser_exit_monitor_ohos.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/filename_util.h"
#include "ohos/adapter/browser/browser_adapter.h"
#include "ohos/adapter/file_manager/file_manager_adapter.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace chrome {

using namespace ohos::adapter;

namespace {
void ProcessSingletonNotificationCallbackImpl(
    base::CommandLine command_line,
    const base::FilePath& current_directory) {
  // Drop the request if the browser process is already shutting down.
  if (!g_browser_process || g_browser_process->IsShuttingDown() ||
      browser_shutdown::HasShutdownStarted()) {
    LOG(ERROR) << __func__
               << "Browser process is shutting down or has been shut down.";
    return;
  }

  StartupProfilePathInfo startup_profile_path_info =
      GetStartupProfilePath(current_directory, command_line,
                            /*ignore_profile_picker=*/false);
  DCHECK_NE(startup_profile_path_info.reason, StartupProfileModeReason::kError);
  StartupBrowserCreator::ProcessCommandLineAlreadyRunning(
      command_line, current_directory, startup_profile_path_info);
}
}

// static
ExecuteCommandSingleton* ExecuteCommandSingleton::GetInstance() {
  static base::NoDestructor<ExecuteCommandSingleton> factory;
  return factory.get();
}

void ExecuteCommandSingleton::CreateNewWindow(const std::string& url,
                                              const bool& is_webapp,
                                              Browser* browser) {
  LOG(INFO) << "[ohoswindow]"
            << "ExecuteCommandSingleton::CreateNewWindow begin, url is empty: "
            << url.empty();
  if (url.empty()) {
    LOG(INFO) << "[ohoswindow] ExecuteCommandSingleton::CreateNewWindow, url is empty, "
              << "execute command with disposition";
    browser->command_controller()->ExecuteCommandWithDisposition(
        IDC_NEW_WINDOW, WindowOpenDisposition::NEW_WINDOW,
        base::TimeTicks::Now());
  } else if (is_webapp) {
    std::vector<std::string> args = {"chrome", url};
    base::CommandLine command_line(args);
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&ProcessSingletonNotificationCallbackImpl,
                                  std::move(command_line), base::FilePath()));
  } else {
    LOG(INFO) << "[ohoswindow] ExecuteCommandSingleton::CreateNewWindow, "
              << "url is not empty, add tab";
    const GURL gurl(url);
    if (gurl.SchemeIsFile()) {
      std::string file_path_str;
      ohos::adapter::FileManagerAdapter::GetInstance().GetPathForUri(
          url.c_str(), file_path_str);
      if (!file_path_str.empty()) {
        base::FilePath file_path(file_path_str);
        const GURL file_url = net::FilePathToFileURL(file_path);
        chrome::AddTabAt(browser, file_url, -1, true);
      } else {
        LOG(ERROR) << "[ohoswindow] CreateNewWindow GetPathForUri failed!";
        chrome::AddTabAt(browser, gurl, -1, true);
      }
    } else {
      chrome::AddTabAt(browser, gurl, -1, true);
    }
  }
}

void ExecuteCommandSingleton::OnExecute(CommandParameter& param,
                                        std::shared_ptr<CommandResult> result) {
  LOG(INFO) << "[ohoswindow] in ExecuteCommandSingleton OnExecute, "
            << "param is " << param.ToString();
  auto task = [](base::WeakPtr<ExecuteCommandSingleton> executor,
                 const CommandParameter param,
                 std::shared_ptr<CommandResult> result) {
    do {
      LOG(INFO) << "[ohoswindow] ExecuteCommandSingleton::OnExecute, param: "
                << param.ToString();
      Browser* browser = BrowserList::GetInstance()->GetLastActive();
      if (browser == nullptr) {
        LOG(ERROR) << "[ohoswindow] Browser::OnExecute no active browser";
        break;
      }

      switch (param.type) {
        case CommandType::kNewWindow:
          result->ret_code = 0;
          executor->CreateNewWindow(param.url, param.is_webapp, browser);
          break;
        case CommandType::kGetLastActiveWidget:
          result->ret_code = 0;
          result->last_widget_Id = (int32_t)browser->GetAcceleratedWidget();
          break;
        case CommandType::kAppExit:
          BrowserExitMonitorOhos::GetInstance().Reset();
          chrome::AttemptUserExit();
          result->ret_code = 0;
          break;
        default:
          LOG(ERROR) << "[ohoswindow] unsupported Operator type:"
                     << (int)param.type;
          break;
      }
    } while (0);

    if (param.is_sync) {
      result->async_callback();
    }
  };

  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(task, AsWeakPtr(), std::move(param), result));
}

void ExecuteCommandSingleton::Register() {
  if (init_for_register_) {
    LOG(WARNING) << "[ohoswindow] Browser command executor is registered";
    return;
  }

  // Register arkui event
  auto& browser_adapter = BrowserAdapter::GetInstance();
  browser_adapter.RegisterBrowserCallback(
      std::bind(&ExecuteCommandSingleton::OnExecute, this,
                std::placeholders::_1, std::placeholders::_2));

  init_for_register_ = true;
}

base::WeakPtr<ExecuteCommandSingleton> ExecuteCommandSingleton::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

base::WeakPtr<const ExecuteCommandSingleton>
ExecuteCommandSingleton::AsWeakPtr() const {
  return weak_factory_.GetWeakPtr();
}

ExecuteCommandSingleton::ExecuteCommandSingleton() = default;

ExecuteCommandSingleton::~ExecuteCommandSingleton() = default;

}  // namespace chrome
