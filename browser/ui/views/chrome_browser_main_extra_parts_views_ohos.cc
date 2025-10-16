/*
 * Copyright (c) 2023-2025 Haitai FangYuan Co., Ltd.
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

#include "chrome/browser/ui/views/chrome_browser_main_extra_parts_views_ohos.h"

#include <initializer_list>
#include <optional>
#include <string_view>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/trace_event/trace_log.h"
#include "chrome/browser/ui/ohos/browser_exit_monitor_ohos.h"
#include "chrome/browser/ui/ohos/execute_command_singleton.h"
#include "chrome/browser/ui/views/chrome_browser_main_extra_parts_views.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/tracing_controller.h"
#include "ohos/adapter/dev_config/dev_config.h"
#include "ohos/adapter/native_theme/native_theme_adapter.h"
#include "ohos/adapter/res_sched/res_sched.h"
#include "ohos/adapter/tracing/tracing_adapter.h"
#include "ui/native_theme/native_theme.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/platform_window_manager.h"

// Tracing related switches enabled by common events

enum TraceEvent : int32_t {
  /** start a chrome trace, optional take trace categories */
  kTraceBegin = 0,
  /** stop a chrome tracem optional take trace filename */
  kTraceEnd = 1,
  /** clear or update dev_config.json file */
  kUpdateDevConfig = 2
};

static const char kDefaultTraceFile[] = "chromium_trace";
static const char kDefaultTraceCategories[] =
    "benchmark,blink,blink_gc,v8,cc,gpu,navigation,toplevel,viz,ui,views,"
    "disk_cache,latency,renderer.scheduler,sequence_manager,timeline.frame,"
    "disabled-by-default-v8.gc,disabled-by-default-blink_gc";
static const char kDevConfigPath[] =
    "/data/storage/el2/base/preferences/dev_config.json";

/**
 * whole command is
 * `cem publish -e chromium_update_config`
 * `-d <package_name>:update_config`
 */
static constexpr std::string_view kUpdateConfigCommand = ":update_config";
/**
 * whole command is
 * `cem publish -e chromium_update_config`
 * `-d <package_name>:clear`
 */
static constexpr std::string_view kClearCommand = ":clear";

void OnTraceDataCollected(const std::string& file_path) {
  LOG(INFO) << "Tracing end, output file: " << file_path;
}

/** max depth to parse the json object (we only read depth 2) */
constexpr size_t kMaxJsonObjDepth = 4;

/** json array must have single element */
constexpr size_t kValidSize = 1;

/**
 * parse the json value, if the json is a json array with one string element,
 * return the path, otherwise return nullopt
 */
std::optional<std::string> ExtractPathFromDevConfigJson(
    const std::string& dev_config_json) {
  std::optional<std::string> result;
  std::optional<base::Value> jsonValue = base::JSONReader::Read(
      dev_config_json, base::JSONParserOptions::JSON_PARSE_RFC,
      kMaxJsonObjDepth);
  // expected the result is an array of single string
  if (!jsonValue.has_value()) {
    return result;
  }
  base::Value::List* list = jsonValue.value().GetIfList();
  if (!list || list->size() != kValidSize) {
    return result;
  }
  std::string* dev_config_path = list->front().GetIfString();
  if (dev_config_path) {
    result.emplace(*dev_config_path);
  }
  return result;
}

/**
 * task to be executed on IO thread, copy dev_config.json to browser file
 * @param dev_config_json the json array of one string contains the path
 */
void UpdateDevConfig(const std::string& dev_config_json) {
  std::optional<std::string> dev_config_path =
      ExtractPathFromDevConfigJson(dev_config_json);
  if (!dev_config_path.has_value()) {
    LOG(ERROR) << "UpdateDevConfig: invalid json format require an array of "
                  "single string";
    return;
  }

  base::FilePath source_path{dev_config_path.value()};
  base::FilePath target_path{kDevConfigPath};
  bool result = base::CopyFile(source_path, target_path);
  LOG(INFO) << "UpdateDevConfig result: " << result;
}

/**
 * callback for `RequestDevConfigDialog`
 * @param pathOrEmtpy a json value or empty string
 */
void OnPickerCallback(const std::string& pathOrEmtpy) {
  if (pathOrEmtpy.empty()) {
    return;
  }
  // copy is a blocking method, should run in IO thread pool
  content::GetIOThreadTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(UpdateDevConfig, pathOrEmtpy));
}

/**
 * clear the json config
 */
void ClearDevConfig() {
  base::FilePath target_path{kDevConfigPath};
  bool delete_result = base::DeleteFile(target_path);
  LOG(INFO) << "clean dev_config.json result: " << delete_result;
}

void HandleDevConfigUpdateError(const std::string& in_param) {
  LOG(ERROR) << "HandleDevConfigUpdate: invalid common event param: "
             << in_param;
}

enum class StrMatchMode {
  kFull = 0,
  kStartWith = 1,
};

/**
 * test `input` is same as join string of `join_strs`
 * @param input the input to be tested with
 * @param join_strs a list of strs to be tested
 * @param mode the mode is `kFull` for full match or `kStartWith` for is start
 * with `join_strs`
 */
bool StrMatchJoinStrs(std::string_view input,
                      std::initializer_list<std::string_view> join_strs,
                      StrMatchMode mode = StrMatchMode::kFull) {
  if (join_strs.size() < 0) {
    return input.empty();
  }
  size_t left_index = 0;
  for (std::string_view elem : join_strs) {
    if (input.size() < left_index + elem.size()) {
      return false;
    }
    std::string_view sub_input = input.substr(left_index, elem.size());
    if (sub_input != elem) {
      return false;
    }
    left_index += elem.size();
  }
  if (mode == StrMatchMode::kFull) {
    return left_index == input.size();
  }
  return true;
}

/**
 * handle `chromium_update_config` common event,
 * the allowed format are:
 * `cem publish -e chromium_update_config -d <in_param>`,
 * and the `in_param` should be two format:
 * - `<bundle_name>:update_config`
 *   open a file picker dialog to chose dev_config.json and copy it to
 * preferences folder
 * - `<bundle_name>:clear`
 *   clear the previous copied dev_config.json file
 */
void HandleDevConfigUpdate(const std::string& in_param) {
  const std::string& bundle_name =
      ohos::adapter::res_sched::ResSchedManager::GetInstance().bundle_name();
  if (StrMatchJoinStrs(in_param, {bundle_name, kUpdateConfigCommand})) {
    ohos::adapter::RequestDevConfigDialog(OnPickerCallback);
  } else if (StrMatchJoinStrs(in_param, {bundle_name, kClearCommand})) {
    content::GetIOThreadTaskRunner()->PostTask(FROM_HERE,
                                               base::BindOnce(ClearDevConfig));
  } else {
    HandleDevConfigUpdateError(in_param);
  }
}

/**
 * the mechanism of common event is the following:
 * - start chromium tracing:
 *     `cem publish -e chromium_tracing_start`
 *     `-d <categories like blink,v8,ui,renderer and so on>`
 *     start the chromium tracing
 * - stop chromium tracing:
 *     `cem publish -e chromium_tracing_stop`
 *     `-d <filename to save>`
 *     stop chromium tracing and save file with name to temp dir
 * - open a file picker for user to select a dev_config.json to push to
 * preference folder
 *     `cem publish -e chromium_update_config`
 *     `-d <bundle_name>:update_config`
 *     will trigger file picker to select a dev_config file
 * - clear the dev_config.json file
 *     `cem publish -e chromium_update_config`
 *     `-d <bundle_name>:clear`
 *     will deleted added dev_config.json file if exist
 */
void TraceControllerCallback(int32_t event_id, const std::string& in_param) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(TraceControllerCallback, event_id, in_param));
    return;
  }

  switch (event_id) {
    case TraceEvent::kTraceBegin: {
      // start tracing
      const std::string trace_categories =
          in_param.empty() ? kDefaultTraceCategories : in_param;
      base::trace_event::TraceConfig trace_config =
          base::trace_event::TraceConfig(trace_categories, "");

      LOG(INFO) << "Tracing begin, categories: " << trace_categories;
      content::TracingController::GetInstance()->StartTracing(
          trace_config, content::TracingController::StartTracingDoneCallback());

      ohos::adapter::tracing::TracingAdapter::GetInstance()
          .EnableAdapterTrace();
      break;
    }
    case TraceEvent::kTraceEnd: {
      // stop tracing
      std::string file_name = in_param.empty() ? kDefaultTraceFile : in_param;
      base::FilePath file_path;
      base::PathService::Get(base::DIR_TEMP, &file_path);
      file_path = file_path.Append(file_name);

      LOG(INFO) << "Tracing end, file: " << file_path;
      content::TracingController::GetInstance()->StopTracing(
          content::TracingController::CreateFileEndpoint(
              file_path,
              base::BindOnce(&OnTraceDataCollected, file_path.value())));
      ohos::adapter::tracing::TracingAdapter::GetInstance()
          .DisableAdapterTrace();
      break;
    }
#if defined(ENABLE_CEM_UPDATE_DEVCONFIG)
    case TraceEvent::kUpdateDevConfig: {
      HandleDevConfigUpdate(in_param);
      break;
    }
#endif
    default:
      break;
  }
}

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

  chrome::ExecuteCommandSingleton::GetInstance()->Register();

  ui::PlatformWindowManager* window_manager = nullptr;
  ui::OzonePlatform* ozone_platform = ui::OzonePlatform::GetInstance();
  if (ozone_platform) {
    window_manager = ozone_platform->GetPlatformWindowManager();
  }
  chrome::BrowserExitMonitorOhos::GetInstance().Register(window_manager);
}

void ChromeBrowserMainExtraPartsViewsOHOS::PostCreateMainMessageLoop() {
  ohos::adapter::tracing::TracingAdapter& instance =
      ohos::adapter::tracing::TracingAdapter::GetInstance();
  instance.RegisterTracingCallback(TraceControllerCallback);
}
