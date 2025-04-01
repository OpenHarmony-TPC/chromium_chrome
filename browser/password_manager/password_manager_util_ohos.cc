// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_manager_util_ohos.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/grit/branded_strings.h"
#include "chrome/grit/generated_resources.h"
#include "ohos/adapter/device_user_auth/device_user_auth.h"
#include "ui/base/l10n/l10n_util.h"

namespace password_manager_util_ohos {
using ::ohos::adapter::device_user_auth::DeviceUserAuthAdapter;

bool AuthenticateUser(std::u16string prompt_string, bool enable_biom) {
  int biomtype = DeviceUserAuthAdapter::GetInstance().GetBiomType();
  bool result = DeviceUserAuthAdapter::GetInstance().StartUserAuth(
      base::UTF16ToUTF8(prompt_string).c_str(), enable_biom, biomtype);
  return result;
}

}  // namespace password_manager_util_ohos
