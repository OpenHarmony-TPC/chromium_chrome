// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ohos_chrome_media_drm_bridge_client.h"

void OhosChromeMediaDrmBridgeClient::AddKeySystemUUIDMappings(
    KeySystemUuidMap* map) {}

media::OhosMediaDrmBridgeDelegate* OhosChromeMediaDrmBridgeClient::GetMediaDrmBridgeDelegate(
    const std::vector<uint8_t>& scheme_uuid) {
  if (scheme_uuid == wiseplay_delegate_.GetUUID()) {
    return &wiseplay_delegate_;
  }
  return nullptr;
}