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
 
#ifndef CHROME_BROWSER_DEVICE_REAUTH_OHOS_DEVICE_AUTHENTICATOR_OHOS_H_
#define CHROME_BROWSER_DEVICE_REAUTH_OHOS_DEVICE_AUTHENTICATOR_OHOS_H_
 
#include "base/functional/callback.h"
#include "base/sequence_checker.h"
#include "chrome/browser/device_reauth/chrome_device_authenticator_factory.h"
#include "components/device_reauth/device_authenticator.h"
#include "components/device_reauth/device_authenticator_common.h"
 
class AuthenticatorOhosInterface;

namespace device::fido::ohos {
class TouchIdContext;
}  // namespace device::fido::ohos
 
class DeviceAuthenticatorOhos : public DeviceAuthenticatorCommon {
 public:
  DeviceAuthenticatorOhos(
      std::unique_ptr<AuthenticatorOhosInterface> authenticator,
      DeviceAuthenticatorProxy* proxy,
      const device_reauth::DeviceAuthParams& params);
  ~DeviceAuthenticatorOhos() override;
  // Creates an instance of DeviceAuthenticatorOhos for testing purposes
  // only.
  static scoped_refptr<DeviceAuthenticatorOhos> CreateForTesting(
      std::unique_ptr<AuthenticatorOhosInterface> authenticator);

  bool CanAuthenticateWithBiometrics() override;

  bool CanAuthenticateWithBiometricOrScreenLock() override;
 
  // Triggers an OS-level authentication flow.
  // If biometrics are available, it creates touchIdAuthentication object,
  // request user to authenticate(proper box with that information will appear
  // on the screen and the `message` will be displayed there) using his touchId
  // or if it's not setUp default one with password will appear. If biometrics
  // aren't available, it falls back to the legacy authentication flow.
 
  void AuthenticateWithMessage(const std::u16string& message,
                               AuthenticateCallback callback) override;
 
  // Should be called by the object using the authenticator if the purpose
  // for which the auth was requested becomes obsolete or the object is
  // destroyed.
  void Cancel() override;
 
 private:
  // Called when the authentication completes with the result |success|.
  void OnAuthenticationCompleted(bool success);
 
  // Callback to be executed after the authentication completes.
  AuthenticateCallback callback_;
 
  std::unique_ptr<AuthenticatorOhosInterface> authenticator_;
 
  // Factory for weak pointers to this class.
  base::WeakPtrFactory<DeviceAuthenticatorOhos> weak_ptr_factory_{this};
};
 
#endif  // CHROME_BROWSER_DEVICE_REAUTH_OHOS_DEVICE_AUTHENTICATOR_OHOS_H_
