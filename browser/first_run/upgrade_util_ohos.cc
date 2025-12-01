// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/first_run/upgrade_util.h"

#include "base/command_line.h"
#include "ohos/adapter/window/app_window_adapter.h"

namespace upgrade_util {

bool RelaunchChromeBrowserImpl(const base::CommandLine& command_line) {
  return ohos::adapter::window::AppWindowAdapter::GetInstance().Relaunch();
}

}  // namespace upgrade_util
