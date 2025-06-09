// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_GENERATED_ADVANCED_SECURITY_MODE_PREF_H_
#define CHROME_BROWSER_SAFE_BROWSING_GENERATED_ADVANCED_SECURITY_MODE_PREF_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "chrome/browser/extensions/api/settings_private/generated_pref.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_change_registrar.h"

// The generated pref for Advanced-Security-Mode. Only used for enabling/disabling
// the setting in the Security UI settings page.
extern const char kGeneratedAdvancedSecurityModePref[];

class GeneratedAdvancedSecurityModePref
    : public extensions::settings_private::GeneratedPref {
 public:
  explicit GeneratedAdvancedSecurityModePref(Profile* profile);
  ~GeneratedAdvancedSecurityModePref() override;

  // Generated Preference Interface.
  extensions::settings_private::SetPrefResult SetPref(
      const base::Value* value) override;
  extensions::api::settings_private::PrefObject GetPrefObject() const override;

  // Fired when preferences used to generate this preference are changed.
  void OnSourcePreferencesChanged();

 private:
  // Non-owning pointer to the profile this preference is generated for.
  const raw_ptr<Profile> profile_;
  PrefChangeRegistrar user_prefs_registrar_;
};

bool IsAdvancedSecurityModeEnabled();

#endif // CHROME_BROWSER_SAFE_BROWSING_GENERATED_ADVANCED_SECURITY_MODE_PREF_H_