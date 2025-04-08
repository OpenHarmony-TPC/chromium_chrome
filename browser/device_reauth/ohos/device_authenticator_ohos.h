// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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
