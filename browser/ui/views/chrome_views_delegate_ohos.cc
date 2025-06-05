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

#include "chrome/browser/ui/views/chrome_views_delegate.h"

#include "chrome/browser/ui/views/native_widget_factory.h"

namespace {

NativeWidgetType GetNativeWidgetTypeForInitParams(
    const views::Widget::InitParams& params) {
  // If this is a security surface, always use a toplevel window,
  // otherwise it's possible for things like menus to obscure the view.
  if (params.z_order &&
      params.z_order.value() == ui::ZOrderLevel::kSecuritySurface)
    return NativeWidgetType::DESKTOP_NATIVE_WIDGET_AURA;

  if (params.use_accelerated_widget_override)
    return NativeWidgetType::DESKTOP_NATIVE_WIDGET_AURA;

  return (params.parent &&
          params.type != views::Widget::InitParams::TYPE_MENU &&
          params.type != views::Widget::InitParams::TYPE_TOOLTIP)
             ? NativeWidgetType::NATIVE_WIDGET_AURA
             : NativeWidgetType::DESKTOP_NATIVE_WIDGET_AURA;
}

} // namespace


views::NativeWidget* ChromeViewsDelegate::CreateNativeWidget(
    views::Widget::InitParams* params,
    views::internal::NativeWidgetDelegate* delegate) {
  return ::CreateNativeWidget(GetNativeWidgetTypeForInitParams(*params),
      params, delegate);
}
