/*
* Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "cmd_keys.h"

namespace pikiwidb {
DelCmd::DelCmd(const std::string& name, int16_t arity) {}
bool DelCmd::DoInitial(PClient* client) { return false; }
void DelCmd::DoCmd(PClient* client) {}
}  // namespace pikiwidb