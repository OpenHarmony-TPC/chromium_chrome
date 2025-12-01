// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/device_reauth/ohos/authenticator_ohos.h"

#include "base/notreached.h"
#include "chrome/browser/password_manager/password_manager_util_ohos.h"
#include "ohos/adapter/device_user_auth/device_user_auth.h"

using ohos::adapter::device_user_auth::DeviceUserAuthAdapter;
AuthenticatorOhos::AuthenticatorOhos() = default;

AuthenticatorOhos::~AuthenticatorOhos() = default;

bool AuthenticatorOhos::CheckIfBiometricsAvailable() {
  DeviceUserAuthAdapter::BiometricCheckResult res =
      DeviceUserAuthAdapter::GetInstance().CheckBiometricAvailable();
  if (res.can_biom) {
    DeviceUserAuthAdapter::GetInstance().SetBiomType(res.biom_type);
  }
  return res.can_biom;
}

bool AuthenticatorOhos::CheckIfBiometricsOrScreenLockAvailable() {
  return false;
}

bool AuthenticatorOhos::AuthenticateUserWithNonBiometrics(
    const std::u16string& message) {
  return password_manager_util_ohos::AuthenticateUser(message, false);
}
