// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/settings_utils.h"

#include "base/notreached.h"
#include "ohos/adapter/cert_manager/cert_manager_adapter.h"
#include "ohos/adapter/net/system_settings_adapter.h"

using namespace ohos::adapter;
namespace settings_utils {

namespace {
const std::string kPageNetworkUri = "pc_network_settings";
const std::string kPageProxySubUri = "proxy_page";
}  // namespace

void ShowNetworkProxySettings(content::WebContents* web_contents) {
  // Open your computer's proxy settings
  net::SystemSettingsAdapter::GetInstance().ShowSystemsSettings(
      kPageNetworkUri, kPageProxySubUri);
}

void ShowManageSSLCertificates(content::WebContents* web_contents) {
  // Open your computer's certificate manager
  CertManagerAdapter::GetInstance().ShowCertificateManagerDialog();
}

}  // namespace settings_utils
