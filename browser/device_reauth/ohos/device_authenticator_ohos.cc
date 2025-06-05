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

#include "chrome/browser/device_reauth/ohos/device_authenticator_ohos.h"

#include "base/functional/bind.h"
#include "base/metrics/histogram_functions.h"
#include "base/notreached.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/device_reauth/ohos/authenticator_ohos.h"
#include "chrome/browser/password_manager/password_manager_util_ohos.h"
#include "chrome/grit/branded_strings.h"
#include "components/device_reauth/device_authenticator.h"
#include "components/password_manager/core/browser/features/password_features.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"

DeviceAuthenticatorOhos::DeviceAuthenticatorOhos(
    std::unique_ptr<AuthenticatorOhosInterface> authenticator,
    DeviceAuthenticatorProxy* proxy,
    const device_reauth::DeviceAuthParams& params)
    : DeviceAuthenticatorCommon(proxy,
                                params.GetAuthenticationValidityPeriod(),
                                params.GetAuthResultHistogram()),
      authenticator_(std::move(authenticator)) {}

DeviceAuthenticatorOhos::~DeviceAuthenticatorOhos() = default;

bool DeviceAuthenticatorOhos::CanAuthenticateWithBiometrics() {
  bool is_available = authenticator_->CheckIfBiometricsAvailable();
  base::UmaHistogramBoolean("PasswordManager.CanUseBiometricsOhos",
                            is_available);
  if (is_available) {
    // If biometrics is available, we should record that at one point in time
    // biometrics was available on this device. This will never be set to false
    // after setting to true here as we only record this when biometrics is
    // available.
    g_browser_process->local_state()->SetBoolean(
        password_manager::prefs::kHadBiometricsAvailable, /*value=*/true);
  }
  return is_available;
}

bool DeviceAuthenticatorOhos::CanAuthenticateWithBiometricOrScreenLock() {
  // We check if we can authenticate strictly with biometrics first as this
  // function has important side effects such as logging metrics related to how
  // often users have biometrics available, and setting a pref that denotes that
  // at one point biometrics was available on this device.
  if (CanAuthenticateWithBiometrics()) {
    return true;
  }

  // TODO(crbug.com/4555994): Add metrics logging for the only screen lock
  // available case.
  return authenticator_->CheckIfBiometricsOrScreenLockAvailable();
}

void DeviceAuthenticatorOhos::Cancel() {
  if (callback_) {
    // No code should be run after the callback as the callback could already be
    // destroying "this".
    std::move(callback_).Run(/*success=*/false);
  }
}

void DeviceAuthenticatorOhos::AuthenticateWithMessage(
    const std::u16string& message,
    AuthenticateCallback callback) {
  // Callers must ensure that previous authentication is canceled.
  DCHECK(!callback_);
  if (!NeedsToAuthenticate()) {
    // No code should be run after the callback as the callback could already be
    // destroying "this".
    std::move(callback).Run(/*success=*/true);
    return;
  }
  callback_ = std::move(callback);
  if (!CanAuthenticateWithBiometrics()) {
    OnAuthenticationCompleted(authenticator_->AuthenticateUserWithNonBiometrics(
        l10n_util::GetStringFUTF16(IDS_PASSWORDS_AUTHENTICATION_PROMPT_PREFIX,
                                   message)));
    return;
  }
  OnAuthenticationCompleted(
      password_manager_util_ohos::AuthenticateUser(message, true));
  return;
}

void DeviceAuthenticatorOhos::OnAuthenticationCompleted(bool success) {
  if (!callback_) {
    return;
  }
  RecordAuthenticationTimeIfSuccessful(success);
  // No code should be run after the callback as the callback could already be
  // destroying "this".
  std::move(callback_).Run(success);
}
