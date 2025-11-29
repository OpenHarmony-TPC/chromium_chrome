// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_applications/os_integration/web_app_shortcut_ohos.h"

#include <cstddef>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/hash/md5.h"
#include "base/i18n/file_util_icu.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "base/notreached.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/scoped_blocking_call.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/web_applications/web_app_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/installer/util/util_constants.h"
#include "content/public/browser/browser_thread.h"
#include "ohos/adapter/web_app/web_app_adapter.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_family.h"

namespace web_app {
namespace {

constexpr base::FilePath::CharType kIconChecksumFileExt[] =
    FILE_PATH_LITERAL(".ico.md5");

}  // namespace

namespace internals {
namespace {

// Calculates checksum of an icon family using MD5.
// The checksum is derived from all of the icons in the family.
void GetImageCheckSum(const gfx::ImageFamily& image, base::MD5Digest* digest) {
  DCHECK(digest);
  base::MD5Context md5_context;
  base::MD5Init(&md5_context);

  for (gfx::ImageFamily::const_iterator it = image.begin(); it != image.end();
       ++it) {
    SkBitmap bitmap = it->AsBitmap();

    std::string_view image_data(
        reinterpret_cast<const char*>(bitmap.getPixels()),
        bitmap.computeByteSize());
    base::MD5Update(&md5_context, image_data);
  }

  base::MD5Final(digest, &md5_context);
}

// Saves |image| as an |icon_file| with the checksum.
bool SaveIconWithCheckSum(const base::FilePath& icon_file,
                          const gfx::ImageFamily& image) {
  for (gfx::ImageFamily::const_iterator it = image.begin(); it != image.end();
       ++it) {
    int width = it->Width();
    scoped_refptr<base::RefCountedMemory> png_data = it->As1xPNGBytes();
    if (png_data->size() == 0) {
      // If the bitmap could not be encoded to PNG format, skip it.
      LOG(WARNING) << "Could not encode icon " << icon_file << ".png at size "
                   << width << ".";
      continue;
    }
    if (!base::WriteFile(icon_file, *png_data)) {
      LOG(ERROR) << "SaveIconWithCheckSum WriteFile failed.";
      return false;
    }
  }
  base::MD5Digest digest;
  GetImageCheckSum(image, &digest);

  base::FilePath cheksum_file(icon_file.ReplaceExtension(kIconChecksumFileExt));
  // Passing digest as one element in a span of digest fields, therefore the 1u,
  // and then having as_bytes converting it to a new span of uint8_t's.
  return base::WriteFile(cheksum_file,
                         base::as_bytes(base::span(&digest, 1u)));
}

// Returns true if |icon_file| is missing or different from |image|.
bool ShouldUpdateIcon(const base::FilePath& icon_file,
                      const gfx::ImageFamily& image) {
  base::FilePath checksum_file(
      icon_file.ReplaceExtension(kIconChecksumFileExt));

  // Returns true if icon_file or checksum file is missing.
  if (!base::PathExists(icon_file) || !base::PathExists(checksum_file)) {
    return true;
  }

  base::MD5Digest persisted_image_checksum;
  if (sizeof(persisted_image_checksum) !=
      base::ReadFile(checksum_file,
                     reinterpret_cast<char*>(&persisted_image_checksum),
                     sizeof(persisted_image_checksum))) {
    return true;
  }

  base::MD5Digest downloaded_image_checksum;
  GetImageCheckSum(image, &downloaded_image_checksum);

  // Update icon if checksums are not equal.
  return memcmp(&persisted_image_checksum, &downloaded_image_checksum,
                sizeof(base::MD5Digest)) != 0;
}

}  // namespace

base::FilePath GetSanitizedFileName(const std::u16string& name) {
  std::string file_name = base::UTF16ToUTF8(name);
  base::i18n::ReplaceIllegalCharactersInPath(&file_name, '_');
  base::ReplaceChars(file_name, " ", "_", &file_name);
  return base::FilePath(file_name);
}

bool CheckAndSaveIcon(const base::FilePath& icon_file,
                      const gfx::ImageFamily& image) {
  if (!ShouldUpdateIcon(icon_file, image)) {
    return true;
  }

  if (!base::CreateDirectory(icon_file.DirName())) {
    return false;
  }

  if (!SaveIconWithCheckSum(icon_file, image)) {
    return false;
  }

  return true;
}

void CreatePlatformShortcuts(const base::FilePath& web_app_path,
                             const ShortcutLocations& creation_locations,
                             ShortcutCreationReason creation_reason,
                             const ShortcutInfo& shortcut_info,
                             CreateShortcutsCallback callback) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);
  // Generates file name to use with persisted ico and shortcut file.
  base::FilePath icon_file = GetIconFilePath(web_app_path, shortcut_info.title);
  if (!CheckAndSaveIcon(icon_file, shortcut_info.favicon)) {
    LOG(ERROR) << "CreatePlatformShortcuts CheckAndSaveIcon error.";
    std::move(callback).Run(false);
    return;
  }

  ohos::adapter::DesktopShortcut shortcut = {
      shortcut_info.app_id, base::UTF16ToUTF8(shortcut_info.title),
      icon_file.value(), "", shortcut_info.open_as_window};
  bool result =
      ohos::adapter::WebAppAdapter::GetInstance().CreateDesktopShortcut(
          shortcut);
  std::move(callback).Run(result);
}

base::FilePath GetIconFilePath(const base::FilePath& web_app_path,
                               const std::u16string& title) {
  return web_app_path.Append(GetSanitizedFileName(title))
      .AddExtension(FILE_PATH_LITERAL(".png"));
}

}  // namespace internals

}  // namespace web_app
