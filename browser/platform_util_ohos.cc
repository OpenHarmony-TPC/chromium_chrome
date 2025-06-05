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
