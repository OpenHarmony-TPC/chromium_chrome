// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVICE_REAUTH_OHOS_AUTHENTICATOR_OHOS_H_
#define CHROME_BROWSER_DEVICE_REAUTH_OHOS_AUTHENTICATOR_OHOS_H_

#include "components/device_reauth/device_authenticator_common.h"

// This interface is need to simplify testing as OHOS authentication happens
// through free function which is hard to mock.
class AuthenticatorOhosInterface {
 public:
  virtual ~AuthenticatorOhosInterface() = default;
  virtual bool CheckIfBiometricsAvailable() = 0;
  virtual bool CheckIfBiometricsOrScreenLockAvailable() = 0;
  virtual bool AuthenticateUserWithNonBiometrics(
      const std::u16string& message) = 0;
};

// Implementation of the interface that handles communication with the OS.
class AuthenticatorOhos : public AuthenticatorOhosInterface {
 public:
  AuthenticatorOhos();
  ~AuthenticatorOhos() override;
  bool CheckIfBiometricsAvailable() override;
  bool CheckIfBiometricsOrScreenLockAvailable() override;
  bool AuthenticateUserWithNonBiometrics(
      const std::u16string& message) override;
};

#endif  // CHROME_BROWSER_DEVICE_REAUTH_OHOS_AUTHENTICATOR_OHOS_H_
