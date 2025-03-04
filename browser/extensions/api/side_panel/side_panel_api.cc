// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/side_panel/side_panel_api.h"

#include "base/values.h"
#include "chrome/browser/extensions/api/side_panel/side_panel_service.h"
#include "chrome/common/extensions/api/side_panel.h"
#include "extensions/common/extension_features.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#if defined(OHOS_ARKWEB_EXTENSIONS)
#include "base/logging.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "ohos_nweb/src/cef_delegate/nweb_extension_side_panel_cef_delegate.h"
#endif // OHOS_ARKWEB_EXTENSIONS

namespace extensions {
namespace {

bool IsSidePanelApiAvailable() {
  return base::FeatureList::IsEnabled(
      extensions_features::kExtensionSidePanelIntegration);
}

}  // namespace

SidePanelApiFunction::SidePanelApiFunction() = default;
SidePanelApiFunction::~SidePanelApiFunction() = default;
SidePanelService* SidePanelApiFunction::GetService() {
  return extensions::SidePanelService::Get(browser_context());
}

ExtensionFunction::ResponseAction SidePanelApiFunction::Run() {
  if (!IsSidePanelApiAvailable())
    return RespondNow(Error("API Unavailable"));
  return RunFunction();
}

ExtensionFunction::ResponseAction SidePanelGetOptionsFunction::RunFunction() {
  LOG(INFO) << "SidePanelGetOptionsFunction::RunFunction";
  absl::optional<api::side_panel::GetOptions::Params> params =
      api::side_panel::GetOptions::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  auto tab_id = params->options.tab_id
                    ? absl::optional<int>(*(params->options.tab_id))
                    : absl::nullopt;
  const api::side_panel::PanelOptions& options =
      GetService()->GetOptions(*extension(), tab_id);
  return RespondNow(WithArguments(options.ToValue()));
}

ExtensionFunction::ResponseAction SidePanelSetOptionsFunction::RunFunction() {
  LOG(INFO) << "SidePanelSetOptionsFunction::RunFunction";
  absl::optional<api::side_panel::SetOptions::Params> params =
      api::side_panel::SetOptions::Params::Create(args());
#if defined(OHOS_ARKWEB_EXTENSIONS)
  std::optional<std::string> absolute_path;
  std::optional<bool> enabled;
  std::optional<int> tab_id;
  if (params->options.path.has_value()) {
    absolute_path = extension()->GetResourceURL(*params->options.path).spec();
  }
  if (params->options.enabled.has_value()) {
    enabled = *params->options.enabled;
  }
  if (params->options.tab_id.has_value()) {
    tab_id = *params->options.tab_id;
  }
#endif
  EXTENSION_FUNCTION_VALIDATE(params);
  // TODO(crbug.com/1328645): Validate the relative extension path exists.
  GetService()->SetOptions(*extension(), std::move(params->options));

#if defined(OHOS_ARKWEB_EXTENSIONS)
  OHOS::NWeb::NWebExtensionSidePanelCefDelegate::OnSetOptions(
      extension()->id(), enabled, tab_id, absolute_path);
#endif
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
SidePanelSetPanelBehaviorFunction::RunFunction() {
  LOG(INFO) << "SidePanelSetPanelBehaviorFunction::RunFunction";
  absl::optional<api::side_panel::SetPanelBehavior::Params> params =
      api::side_panel::SetPanelBehavior::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  if (params->behavior.open_panel_on_action_click.has_value()) {
    GetService()->SetOpenSidePanelOnIconClick(
        extension()->id(), *params->behavior.open_panel_on_action_click);
#if defined(OHOS_ARKWEB_EXTENSIONS)
    OHOS::NWeb::NWebExtensionSidePanelCefDelegate::OnSetPanelBehavior(
        extension()->id(), *params->behavior.open_panel_on_action_click);
#endif
  }

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
SidePanelGetPanelBehaviorFunction::RunFunction() {
  LOG(INFO) << "SidePanelGetPanelBehaviorFunction::RunFunction";
  api::side_panel::PanelBehavior behavior;
  behavior.open_panel_on_action_click =
      GetService()->OpenSidePanelOnIconClick(extension()->id());

  return RespondNow(WithArguments(behavior.ToValue()));
}

#ifdef OHOS_ARKWEB_EXTENSIONS
ExtensionFunction::ResponseAction SidePanelOpenFunction::RunFunction() {
  // Only available to extensions.
  EXTENSION_FUNCTION_VALIDATE(extension());

  // `sidePanel.open()` requires a user gesture.
  if (!user_gesture()) {
    return RespondNow(
        Error("`sidePanel.open()` may only be called in "
              "response to a user gesture."));
  }

  absl::optional<api::side_panel::Open::Params> params =
      api::side_panel::Open::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  if (!params->options.tab_id && !params->options.window_id) {
    return RespondNow(
        Error("At least one of `tabId` and `windowId` must be provided"));
  }

  OHOS::NWeb::NWebExtensionSidePanelCefDelegate::OnOpen(
      extension()->id(),
      params->options.tab_id.value_or(api::tabs::TAB_ID_NONE),
      params->options.window_id.value_or(api::windows::WINDOW_ID_NONE));

  // TODO(crbug.com/40064601): Should we wait for the side panel to be
  // created and load? That would probably be nice.
  return RespondNow(NoArguments());
}
#endif // OHOS_ARKWEB_EXTENSIONS

}  // namespace extensions
