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
