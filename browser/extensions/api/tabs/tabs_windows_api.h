// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_TABS_TABS_WINDOWS_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_TABS_TABS_WINDOWS_API_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/event_router.h"

#if BUILDFLAG(ARKWEB_ARKWEB_EXTENSIONS)
#include "ohos_nweb/src/capi/web_extension_tab_items.h"

struct NWebExtensionTabChangeInfo;
#endif // ARKWEB_ARKWEB_EXTENSIONS

namespace extensions {
class TabsEventRouter;
class WindowsEventRouter;

class TabsWindowsAPI : public BrowserContextKeyedAPI,
                       public EventRouter::Observer {
 public:
  explicit TabsWindowsAPI(content::BrowserContext* context);
  ~TabsWindowsAPI() override;

  // Convenience method to get the TabsWindowsAPI for a profile.
  static TabsWindowsAPI* Get(content::BrowserContext* context);

  TabsEventRouter* tabs_event_router();
  WindowsEventRouter* windows_event_router();

  // KeyedService implementation.
  void Shutdown() override;

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<TabsWindowsAPI>* GetFactoryInstance();

  // EventRouter::Observer implementation.
  void OnListenerAdded(const extensions::EventListenerInfo& details) override;

#if BUILDFLAG(ARKWEB_ARKWEB_EXTENSIONS)
#include "cef/ohos_cef_ext/libcef/browser/extensions/api/tabs/tabs_windows_api_for_include_file.cc"
#endif // ARKWEB_ARKWEB_EXTENSIONS

 private:
  friend class BrowserContextKeyedAPIFactory<TabsWindowsAPI>;

  raw_ptr<content::BrowserContext> browser_context_;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() {
    return "TabsWindowsAPI";
  }
  static const bool kServiceIsNULLWhileTesting = true;

  std::unique_ptr<TabsEventRouter> tabs_event_router_;
  std::unique_ptr<WindowsEventRouter> windows_event_router_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_TABS_TABS_WINDOWS_API_H_
