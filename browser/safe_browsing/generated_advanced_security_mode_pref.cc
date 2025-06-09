// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/generated_advanced_security_mode_pref.h"

#include "base/feature_list.h"
#include "base/logging.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/extensions/api/settings_private.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace settings_api = extensions::api::settings_private;

const char kGeneratedAdvancedSecurityModePref[] =
    "generated.advanced_security_mode_enabled";

GeneratedAdvancedSecurityModePref::GeneratedAdvancedSecurityModePref(Profile* profile)
    : profile_(profile) {
  user_prefs_registrar_.Init(profile->GetPrefs());
  user_prefs_registrar_.Add(
      prefs::kAdvancedSecurityModeEnabled,
      base::BindRepeating(
          &GeneratedAdvancedSecurityModePref::OnSourcePreferencesChanged,
          base::Unretained(this)));
}

GeneratedAdvancedSecurityModePref::~GeneratedAdvancedSecurityModePref() = default;

void GeneratedAdvancedSecurityModePref::OnSourcePreferencesChanged() {
  NotifyObservers(kGeneratedAdvancedSecurityModePref);
}

extensions::settings_private::SetPrefResult
GeneratedAdvancedSecurityModePref::SetPref(const base::Value* value) {
  if (!value->is_bool()) {
    return extensions::settings_private::SetPrefResult::PREF_TYPE_MISMATCH;
  }

  if (!profile_->GetPrefs()
           ->FindPreference(prefs::kAdvancedSecurityModeEnabled)
           ->IsUserModifiable()) {
    return extensions::settings_private::SetPrefResult::PREF_NOT_MODIFIABLE;
  }

  profile_->GetPrefs()->SetBoolean(prefs::kAdvancedSecurityModeEnabled,
                                   value->GetBool());
  return extensions::settings_private::SetPrefResult::SUCCESS;
}

settings_api::PrefObject GeneratedAdvancedSecurityModePref::GetPrefObject() const {
  auto* backing_preference =
      profile_->GetPrefs()->FindPreference(prefs::kAdvancedSecurityModeEnabled);

  settings_api::PrefObject pref_object;
  pref_object.key = kGeneratedAdvancedSecurityModePref;
  pref_object.type = settings_api::PrefType::kBoolean;
  pref_object.value = base::Value(backing_preference->GetValue()->GetBool());

  if (!backing_preference->IsUserModifiable()) {
    pref_object.enforcement = settings_api::Enforcement::kEnforced;
    extensions::settings_private::GeneratedPref::ApplyControlledByFromPref(
        &pref_object, backing_preference);
  } else if (backing_preference->GetRecommendedValue()) {
    pref_object.enforcement =
        settings_api::Enforcement::kEnforced;
    pref_object.recommended_value =
        base::Value(backing_preference->GetRecommendedValue()->GetBool());
  }

  return pref_object;
}
