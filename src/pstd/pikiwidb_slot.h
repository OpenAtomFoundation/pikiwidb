// Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PIKIWIDB_SLOT_H_
#define PIKIWIDB_SLOT_H_

#include <stdint.h>
#include <memory>
#include <string>

// get db instance number of the key
int32_t GetSlotID(const std::string& str);

// get db instance number of the key
int32_t GetSlotsID(const std::string& str, uint32_t* pcrc, int* phastag);

#endif