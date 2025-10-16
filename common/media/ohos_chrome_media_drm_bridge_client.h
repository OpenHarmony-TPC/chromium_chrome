// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OHOS_CHROME_MEDIA_DRM_BRIDGE_CLIENT_H
#define OHOS_CHROME_MEDIA_DRM_BRIDGE_CLIENT_H

#include "media/base/ohos/ohos_media_drm_bridge_client.h"
#include "components/cdm/common/wiseplay_drm_delegate_ohos.h"

class OhosChromeMediaDrmBridgeClient : public media::OhosMediaDrmBridgeClient {
public:
  OhosChromeMediaDrmBridgeClient() = default;
  ~OhosChromeMediaDrmBridgeClient() override = default;

private:
  void AddKeySystemUUIDMappings(KeySystemUuidMap* map) override;
  media::OhosMediaDrmBridgeDelegate* GetMediaDrmBridgeDelegate(
      const media::UUID& scheme_uuid) override;

  std::vector<std::string> key_system_uuid_mappings_;
  cdm::WiseplayDrmDelegateOhos wiseplay_delegate_;
};

#endif  // OHOS_CHROME_MEDIA_DRM_BRIDGE_CLIENT_H