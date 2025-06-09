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

#include "chrome/browser/notifications/notification_platform_bridge_ohos.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_display_service_impl.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/common/notifications/notification_operation.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "url/origin.h"

namespace {
using NotificationPlatformBridgePtr =
    std::unique_ptr<NotificationPlatformBridge>;
using NotificationImage = ohos::adapter::NotificationImage;
// One pixel of the picture is made up of four bits of binary data.
const int IMAGE_PIXEL_MAP = 4;
// OH currently supports only 3 notification buttons
const int MAX_BUTTON_NUM = 3;
// No corresponding notification information was found
const int EMPTY_SIZE = 0;
}  // namespace

NotificationPlatformBridgeOhos::NotificationPlatformBridgeOhos() {}

NotificationPlatformBridgeOhos::~NotificationPlatformBridgeOhos() {
  ohos::adapter::NotificationAdapter::GetInstance().DestroyCallback();
}

NotificationPlatformBridgePtr NotificationPlatformBridge::Create() {
  return std::make_unique<NotificationPlatformBridgeOhos>();
}

bool NotificationPlatformBridge::CanHandleType(
    NotificationHandler::Type notification_type) {
  return notification_type != NotificationHandler::Type::TRANSIENT;
}

void NotificationPlatformBridgeOhos::Display(
    NotificationHandler::Type notification_type,
    Profile* profile,
    const message_center::Notification& notification,
    std::unique_ptr<NotificationCommon::Metadata> metadata) {
  std::string title = base::UTF16ToUTF8(notification.title());
  std::string message = base::UTF16ToUTF8(notification.message());
  if (message.empty()) {
    // if the message is empty, the message section displays the contents of the
    // header
    message = title;
  }
  DCHECK(notification.buttons().size() <= MAX_BUTTON_NUM);
  ohos::adapter::NotificationRequest request = {
      GenerateNotificationId(notification, profile, notification_type),
      title,
      message,
      notification.never_timeout(),
      notification.silent(),
      notification.timestamp().InMillisecondsSinceUnixEpoch(),
      ConvertNotificationIcon(notification.icon(), profile),
      ConvertNotificationImage(notification.image()),
      ConvertNotificationButtons(notification.buttons())};
  ohos::adapter::NotificationAdapter::GetInstance().SendNotification(request);
}

void NotificationPlatformBridgeOhos::Close(Profile* profile,
                                           const std::string& notification_id) {
  DCHECK(notification_request_id_map_.count(notification_id));
  int notification_request_id = notification_request_id_map_[notification_id];
  ohos::adapter::NotificationAdapter::GetInstance().CloseNotification(
      notification_request_id);
  notification_data_map_.erase(notification_request_id);
  notification_request_id_map_.erase(notification_id);
}

void NotificationPlatformBridgeOhos::GetDisplayed(
    Profile* profile,
    GetDisplayedNotificationsCallback callback) const {
  if (profile == nullptr) {
    LOG(ERROR) << "get displayed fail: profile is nullptr!";
    return;
  }
  std::vector<int> notificationRequestIds =
      ohos::adapter::NotificationAdapter::GetInstance()
          .getAllDisplayedNotification();
  std::string profile_id = GetProfileId(profile);
  bool incognito = profile->IsOffTheRecord();
  std::set<std::string> displayed;
  for (const int& requestId : notificationRequestIds) {
    if (notification_data_map_.count(requestId) > 0) {
      NotificationData data = notification_data_map_.at(requestId);
      if (data.profileId == profile_id && data.isIncognito == incognito) {
        displayed.insert(data.notification.id());
      }
    }
  }
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), std::move(displayed), true));
}

void NotificationPlatformBridgeOhos::GetDisplayedForOrigin(
    Profile* profile,
    const GURL& origin,
    GetDisplayedNotificationsCallback callback) const {
  if (profile == nullptr) {
    LOG(ERROR) << "get displayed fail: profile is nullptr!";
    return;
  }
  std::vector<int> notificationRequestIds =
      ohos::adapter::NotificationAdapter::GetInstance()
          .getAllDisplayedNotification();
  std::string profile_id = GetProfileId(profile);
  bool incognito = profile->IsOffTheRecord();
  std::set<std::string> displayed;
  for (const int& requestId : notificationRequestIds) {
    if (notification_data_map_.count(requestId) > 0) {
      NotificationData data = notification_data_map_.at(requestId);
      if (data.profileId == profile_id && data.isIncognito == incognito &&
          url::IsSameOriginWith(data.notification.origin_url(), origin)) {
        displayed.insert(data.notification.id());
      }
    }
  }
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), std::move(displayed), true));
}

void NotificationPlatformBridgeOhos::SetReadyCallback(
    NotificationBridgeReadyCallback callback) {
  std::move(callback).Run(true);
  auto& notificationInstance =
      ohos::adapter::NotificationAdapter::GetInstance();
  notificationInstance.RegisterOnClickCallback(std::bind(
      &NotificationPlatformBridgeOhos::OnClick, this, std::placeholders::_1));
  notificationInstance.RegisterOnCloseCallback(std::bind(
      &NotificationPlatformBridgeOhos::OnClose, this, std::placeholders::_1));
  notificationInstance.RegisterOnButtonClickCallback(
      std::bind(&NotificationPlatformBridgeOhos::OnButtonClick, this,
                std::placeholders::_1, std::placeholders::_2));
  InitNotificationRequestIdCounter();
}

void NotificationPlatformBridgeOhos::DisplayServiceShutDown(Profile* profile) {
  // Destroy the registered callback function when the service is shut down.
  ohos::adapter::NotificationAdapter::GetInstance().DestroyCallback();
}

void ForwardNotificationOperationOnUiThread(
    const NotificationData& notification_data,
    NotificationOperation operation,
    const std::optional<int>& action_index) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!notification_data.profileId.empty());
  g_browser_process->profile_manager()->LoadProfile(
      NotificationPlatformBridge::GetProfileBaseNameFromProfileId(
          notification_data.profileId),
      notification_data.isIncognito,
      base::BindOnce(&NotificationDisplayServiceImpl::ProfileLoadedCallback,
                     operation, notification_data.notificationType,
                     notification_data.notification.origin_url(),
                     notification_data.notification.id(),
                     action_index /* action_index */, std::nullopt /* reply */,
                     true, base::DoNothing()));
}

void ForwardNotificationOperation(const base::Location& location,
                                  const NotificationData& notification_data,
                                  NotificationOperation operation,
                                  const std::optional<int>& action_index) {
  content::GetUIThreadTaskRunner({})->PostTask(
      location, base::BindOnce(ForwardNotificationOperationOnUiThread,
                               notification_data, operation, action_index));
}

int NotificationPlatformBridgeOhos::GenerateNotificationId(
    const message_center::Notification& notification,
    Profile* profile,
    NotificationHandler::Type notificationType) {
  if (notification.renotify() &&
      notification_request_id_map_.count(notification.id()) > 0) {
    return notification_request_id_map_[notification.id()];
  }
  int id = ++id_counter_;
  notification_request_id_map_[notification.id()] = id;
  if (profile != nullptr) {
    NotificationData notificationData = {
        notification.id(), notification, GetProfileId(profile),
        profile->IsOffTheRecord(), notificationType};
    notification_data_map_.insert(std::make_pair(id, notificationData));
  }
  return id;
}

NotificationImage NotificationPlatformBridgeOhos::ConvertImage(
    const gfx::ImageSkia* image) {
  if (image == nullptr) {
    LOG(ERROR) << "parse notification image pixel fail image is nullptr!";
    return {};
  }
  const SkBitmap* bitmap = image->bitmap();
  if (bitmap == nullptr) {
    LOG(ERROR) << "parse notification image pixel fail:bitmap is nullptr!";
    return {};
  }
  int width = bitmap->width();
  int height = bitmap->height();
  size_t row_bytes = width * IMAGE_PIXEL_MAP;
  size_t buffer_size = bitmap->computeByteSize();
  std::unique_ptr<char[]> buff = std::make_unique<char[]>(buffer_size);
  SkPixmap pixmap(SkImageInfo::MakeN32Premul(width, height), buff.get(),
                  row_bytes);
  image->bitmap()->peekPixels(&pixmap);
  if (pixmap.readPixels(SkImageInfo::MakeN32Premul(width, height), buff.get(),
                        row_bytes, 0, 0)) {
    return {width, height, std::move(buff)};
  } else {
    LOG(ERROR) << "parse notification image pixel fail!";
    return {};
  }
}

NotificationImage NotificationPlatformBridgeOhos::ConvertNotificationIcon(
    const ui::ImageModel& icon,
    Profile* profile) {
  if (!icon.IsEmpty()) {
    gfx::ImageSkia iconSkia = icon.Rasterize(
        ThemeServiceFactory::GetForProfile(profile)->GetColorProvider());
    return ConvertImage(&iconSkia);
  } else {
    return {};
  }
}

NotificationImage NotificationPlatformBridgeOhos::ConvertNotificationImage(
    const gfx::Image& image) {
  if (!image.IsEmpty()) {
    return ConvertImage(image.ToImageSkia());
  } else {
    return {};
  }
}

std::vector<ohos::adapter::NotificationButton> NotificationPlatformBridgeOhos::ConvertNotificationButtons(
    const std::vector<message_center::ButtonInfo>& buttons) {
  std::vector<ohos::adapter::NotificationButton> requestButtons;
  if (buttons.size() > 0) {
    int buttonIndex = 0;
    for (const auto& button : buttons) {
      requestButtons.push_back({base::UTF16ToUTF8(button.title), buttonIndex});
      buttonIndex++;
    }
  }
  return requestButtons;
}

void NotificationPlatformBridgeOhos::InitNotificationRequestIdCounter() {
  std::vector<int> notificationRequestIds =
      ohos::adapter::NotificationAdapter::GetInstance()
          .getAllDisplayedNotification();
  if (notificationRequestIds.size() > 0) {
    int maxnotificationRequedstId = 0;
    for (const int& notificationRequedstId : notificationRequestIds) {
      maxnotificationRequedstId =
          notificationRequedstId > maxnotificationRequedstId
              ? notificationRequedstId
              : maxnotificationRequedstId;
    }
    id_counter_ = maxnotificationRequedstId;
  }
}

void NotificationPlatformBridgeOhos::OnClick(int32_t id) {
  if (notification_data_map_.count(id) == EMPTY_SIZE) {
    return;
  }
  NotificationData& notificationData = notification_data_map_.at(id);
  DCHECK(!notificationData.profileId.empty());
  ForwardNotificationOperation(FROM_HERE, notificationData,
                               NotificationOperation::kClick, std::nullopt);
}

void NotificationPlatformBridgeOhos::OnClose(int32_t id) {
  if (notification_data_map_.count(id) == EMPTY_SIZE) {
    return;
  }
  NotificationData& notificationData = notification_data_map_.at(id);
  DCHECK(!notificationData.profileId.empty());
  ForwardNotificationOperation(FROM_HERE, notificationData,
                               NotificationOperation::kClose, std::nullopt);
  notification_data_map_.erase(id);
  notification_request_id_map_.erase(notificationData.notificationId);
}

void NotificationPlatformBridgeOhos::OnButtonClick(int32_t id,
                                                   int32_t buttonIndex) {
  if (notification_data_map_.count(id) == EMPTY_SIZE) {
    return;
  }
  NotificationData& notificationData = notification_data_map_.at(id);
  int buttonSize = notificationData.notification.buttons().size();
  if (buttonIndex >= buttonSize) {
    return;
  }
  DCHECK(!notificationData.profileId.empty());
  ForwardNotificationOperation(FROM_HERE, notificationData,
                               NotificationOperation::kClick, buttonIndex);
}
