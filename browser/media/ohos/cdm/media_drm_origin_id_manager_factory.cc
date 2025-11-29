// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/ohos/cdm/media_drm_origin_id_manager_factory.h"

#include <utility>

#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/media/ohos/cdm/media_drm_origin_id_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "media/base/media_switches.h"

// static
MediaDrmOriginIdManager* MediaDrmOriginIdManagerFactory::GetForProfile(
    Profile* profile) {
  return static_cast<MediaDrmOriginIdManager*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
MediaDrmOriginIdManagerFactory* MediaDrmOriginIdManagerFactory::GetInstance() {
  return base::Singleton<MediaDrmOriginIdManagerFactory>::get();
}

MediaDrmOriginIdManagerFactory::MediaDrmOriginIdManagerFactory()
    : ProfileKeyedServiceFactory(
          "MediaDrmOriginIdManager",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .WithGuest(ProfileSelection::kOriginalOnly)
              .Build()) {}

MediaDrmOriginIdManagerFactory::~MediaDrmOriginIdManagerFactory() = default;

bool MediaDrmOriginIdManagerFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return false;
}

std::unique_ptr<KeyedService>
MediaDrmOriginIdManagerFactory::BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  return std::make_unique<MediaDrmOriginIdManager>(profile->GetPrefs());
}
