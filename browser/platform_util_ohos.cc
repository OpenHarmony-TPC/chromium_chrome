// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/platform_util.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "ohos/adapter/external_protocol/external_protocol_adapter.h"
#include "ohos/adapter/file_manager/file_manager_adapter.h"
#include "url/gurl.h"

namespace platform_util {

void ShowItemInFolder(Profile* profile, const base::FilePath& full_path) {
  base::stat_wrapper_t stat_buf;
  if (base::File::Stat(full_path, &stat_buf) < 0) {
    LOG(ERROR) << "file has been deleted or moved!";
    return;
  }
  auto& file_manager_adapter_instance = ohos::adapter::FileManagerAdapter::GetInstance();
  file_manager_adapter_instance.OpenItemInFolder(full_path.value());
}

namespace internal {

void PlatformOpenVerifiedItem(const base::FilePath& path, OpenItemType type) {
  auto& file_manager_adapter_instance =
      ohos::adapter::FileManagerAdapter::GetInstance();
  if (type == OPEN_FOLDER) {
    file_manager_adapter_instance.OpenItemInFolder(path.value());
  } else {
    file_manager_adapter_instance.OpenVerifiedItem(path.value());
  }
}

}  // namespace internal

void OpenExternal(const GURL& url) {
  auto& external_protocol_adapter_instance =
      ohos::adapter::ExternalProtocolAdapter::GetInstance();
  external_protocol_adapter_instance.OpenExternal(url.spec());
}

}  // namespace platform_util
