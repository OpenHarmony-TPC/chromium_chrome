// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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
