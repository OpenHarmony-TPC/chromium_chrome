// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ostream>

#include "base/notreached.h"
#include "chrome/browser/printing/printer_manager_dialog.h"
#include "ohos/adapter/print/ohos_print_adapter.h"

namespace printing {

void PrinterManagerDialog::ShowPrinterManagerDialog() {
  ohos::adapter::print::PrintAdapter::GetInstance().SystemPrinterSettings();
}

}  // namespace printing
