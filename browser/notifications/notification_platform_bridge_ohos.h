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

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_OHOS_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_OHOS_H_

#include <unordered_map>

#include "chrome/browser/notifications/notification_platform_bridge.h"
#include "ohos/adapter/notification/notification_adapter.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkImage.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/message_center/public/cpp/notification.h"

struct NotificationData {
  std::string notificationId;
  message_center::Notification notification;
  std::string profileId;
  bool isIncognito;
  NotificationHandler::Type notificationType;
};

class NotificationPlatformBridgeOhos : public NotificationPlatformBridge {
 public:
  NotificationPlatformBridgeOhos();
  NotificationPlatformBridgeOhos(const NotificationPlatformBridgeOhos&) =
      delete;
  NotificationPlatformBridgeOhos& operator=(
      const NotificationPlatformBridgeOhos&) = delete;
  ~NotificationPlatformBridgeOhos() override;

  void Display(NotificationHandler::Type notification_type,
               Profile* profile,
               const message_center::Notification& notification,
               std::unique_ptr<NotificationCommon::Metadata> metadata) override;
  void Close(Profile* profile, const std::string& notification_id) override;
  void GetDisplayed(Profile* profile,
                    GetDisplayedNotificationsCallback callback) const override;
  void GetDisplayedForOrigin(
      Profile* profile,
      const GURL& origin,
      GetDisplayedNotificationsCallback callback) const override;
  void SetReadyCallback(NotificationBridgeReadyCallback callback) override;
  void DisplayServiceShutDown(Profile* profile) override;

  int GenerateNotificationId(const message_center::Notification& notification,
                             Profile* profile,
                             NotificationHandler::Type notificationType);
  ohos::adapter::NotificationImage ConvertImage(const gfx::ImageSkia* image);
  ohos::adapter::NotificationImage ConvertNotificationIcon(
      const ui::ImageModel& icon,
      Profile* profile);
  ohos::adapter::NotificationImage ConvertNotificationImage(
      const gfx::Image& image);
  std::vector<ohos::adapter::NotificationButton> ConvertNotificationButtons(
      const std::vector<message_center::ButtonInfo>& buttons);
  void InitNotificationRequestIdCounter();
  void OnClick(int32_t id);
  void OnClose(int32_t id);
  void OnButtonClick(int32_t id, int32_t buttonIndex);

 private:
  int id_counter_ = 0;
  std::unordered_map<std::string, int> notification_request_id_map_;
  std::unordered_map<int, NotificationData> notification_data_map_;
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_OHOS_H_
