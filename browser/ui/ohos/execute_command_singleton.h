// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_OHOS_EXECUTE_COMMAND_SINGLETON_H_
#define CHROME_BROWSER_UI_OHOS_EXECUTE_COMMAND_SINGLETON_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"

#include "ui/gfx/native_widget_types.h"

namespace ohos::adapter {
class CommandParameter;
class CommandResult;
}

class Browser;

namespace chrome {

class ExecuteCommandSingleton {
 public:
  static ExecuteCommandSingleton* GetInstance();

  void CreateNewWindow(const std::string& url, const bool& is_webapp, Browser* browser);

  void OnExecute(ohos::adapter::CommandParameter& param,
                 std::shared_ptr<ohos::adapter::CommandResult> result);

  void Register();

 private:
  friend class base::NoDestructor<ExecuteCommandSingleton>;

  ExecuteCommandSingleton();
  ~ExecuteCommandSingleton();

  base::WeakPtr<ExecuteCommandSingleton> AsWeakPtr();
  base::WeakPtr<const ExecuteCommandSingleton> AsWeakPtr() const;

  bool init_for_register_ = false;

  // The following factory is used to close the frame at a later time.
  base::WeakPtrFactory<ExecuteCommandSingleton> weak_factory_{this};
};

}  // namespace chrome

#endif  // CHROME_BROWSER_UI_OHOS_EXECUTE_COMMAND_SINGLETON_H_
