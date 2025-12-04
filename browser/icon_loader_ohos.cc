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

#include "base/base64.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/icon_loader.h"
#include "ohos/adapter/file_manager/file_manager_adapter.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "third_party/skia/include/core/SkBitmap.h"

// static
IconLoader::IconGroup IconLoader::GroupForFilepath(
    const base::FilePath& file_path) {
  base::FilePath::StringType ext = file_path.FinalExtension();
  auto& file_manager_adapter_instance =
      ohos::adapter::FileManagerAdapter::GetInstance();
  IconLoader::IconGroup icon_group =
      file_manager_adapter_instance.GetFileTypeIdByFileExtension(ext);
  if (icon_group.empty()) {
    return kDefaultFileTypeId;
  }
  return icon_group;
}

// static
scoped_refptr<base::TaskRunner> IconLoader::GetReadIconTaskRunner() {
  return content::GetUIThreadTaskRunner({});
}

void IconLoader::OnIconReady(std::optional<std::vector<uint8_t>> icon) {
  gfx::Image image;

  if (icon.has_value()) {
    SkBitmap bitmap = gfx::PNGCodec::Decode(icon.value());
    gfx::ImageSkia image_skia =
        gfx::ImageSkia::CreateFromBitmap(bitmap, scale_);
    image_skia.MakeThreadSafe();
    image = gfx::Image(image_skia);
  } else {
    LOG(ERROR) << "GetFileIconByFileTypeId failed, icon data is empty.";
    image = gfx::Image();
  }

  target_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback_), std::move(image), group_));
  delete this;
}

void IconLoader::ReadIcon() {
  TRACE_EVENT0("ui", "IconLoader::ReadIcon");
  auto on_icon_ready = [this](std::optional<std::vector<uint8_t>> icon) {
    IconLoader::GetReadIconTaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(&IconLoader::OnIconReady, base::Unretained(this), icon));
  };

  auto& file_manager_adapter_instance =
      ohos::adapter::FileManagerAdapter::GetInstance();
  file_manager_adapter_instance.GetFileIconByFileTypeId(group_, on_icon_ready);
}
