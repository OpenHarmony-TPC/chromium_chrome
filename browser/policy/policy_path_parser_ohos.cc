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

#include "chrome/browser/policy/policy_path_parser.h"

#include <string>

#include "base/logging.h"
#include "base/notreached.h"

namespace policy {

namespace path_parser {

const char kMachineNamePolicyVarName[] = "${machine_name}";
const char kUserNamePolicyVarName[] = "${user_name}";

base::FilePath::StringType ExpandPathVariables(
    const base::FilePath::StringType& untranslated_string) {
  base::FilePath::StringType result(untranslated_string);
  if (result.length() == 0)
    return result;

  // Policy paths may be wrapped in quotes, which should be removed.
  constexpr int offset = 2;
  if (result.length() > 1 &&
      ((result.front() == '"' && result.back() == '"') ||
       (result.front() == '\'' && result.back() == '\''))) {
    // Strip first and last char which should be matching quotes now.
    result = result.substr(1, result.length() - offset);
  }

  // Translate two special variables ${user_name} and ${machine_name}
  // TODO(crbug.com/1231482): Integrate with platform provided values, as
  // they become available.
  size_t position = result.find(kUserNamePolicyVarName);
  if (position != std::string::npos) {
    NOTIMPLEMENTED() << "Username variable not implemented.";
    result.replace(position, strlen(kUserNamePolicyVarName), "user");
  }
  position = result.find(kMachineNamePolicyVarName);
  if (position != std::string::npos) {
    NOTIMPLEMENTED() << "Machine name variable not implemented.";
    result.replace(position, strlen(kMachineNamePolicyVarName), "machine");
  }

  return result;
}
}  // namespace path_parser
}  // namespace policy
