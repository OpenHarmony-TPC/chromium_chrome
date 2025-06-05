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

#include "base/logging.h"
#include "base/path_service.h"
#include "base/trace_event/trace_log.h"
#include "chrome/browser/ui/views/chrome_browser_main_extra_parts_views.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/tracing_controller.h"
#include "ohos/adapter/native_theme/native_theme_adapter.h"
#include "ohos/adapter/tracing/tracing_adapter.h"
#include "ui/native_theme/native_theme.h"

// Tracing related switches enabled by common events

enum TraceEvent : int32_t { kTraceBegin = 0, kTraceEnd = 1 };

static const char kDefaultTraceFile[] = "chromium_trace";
static const char kDefaultTraceCategories[] =
    "benchmark,blink,blink_gc,v8,cc,gpu,navigation,toplevel,viz,ui,views,"
    "disk_cache,latency,renderer.scheduler,sequence_manager,timeline.frame,"
    "disabled-by-default-v8.gc,disabled-by-default-blink_gc";

void OnTraceDataCollected(const std::string& file_path) {
  LOG(INFO) << "Tracing end, output file: " << file_path;
}

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
}

void ChromeBrowserMainExtraPartsViewsOHOS::PostCreateMainMessageLoop() {
  ohos::adapter::tracing::TracingAdapter& instance =
      ohos::adapter::tracing::TracingAdapter::GetInstance();
  instance.RegisterTracingCallback(TraceControllerCallback);
}
