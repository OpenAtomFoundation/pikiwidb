// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

/*
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <charconv>
#include <string>
#include <vector>

namespace pstd {

int StringMatchLen(const char* pattern, int patternLen, const char* string, int stringLen, int nocase);
int StringMatch(const char* p, const char* s, int nocase);
bool StringEqualCaseInsensitive(const std::string& str1, const std::string& str2);
long long Memtoll(const char* p, int* err);
uint32_t Digits10(uint64_t v);

int Ll2string(char* dst, size_t dstlen, int64_t val);

/* Convert a integral into a string. Returns the string after installation. */
template <std::integral T>
inline std::string Int2string(T val) {
  return std::to_string(val);
}

/* Convert a string into a integral. Returns 1 if the string could be parsed
 * into a integer, 0 otherwise. The value will be set to
 * the parsed value when appropriate. */
template <std::integral T>
int String2int(const char* s, size_t slen, T* val) {
  auto [ptr, ec] = std::from_chars(s, s + slen, *val);
  if (ec != std::errc()) {
    return 0;
  } else {
    return 1;
  }
}

template <std::integral T>
inline int String2int(const std::string& s, T* val) {
  return String2int(s.data(), s.size(), val);
}

int String2d(const char* s, size_t slen, double* val);
inline int String2d(const std::string& s, double* val) { return String2d(s.data(), s.size(), val); }

int D2string(char* buf, size_t len, double value);

std::vector<std::string>& StringSplit(const std::string& s, char delim, std::vector<std::string>& elems);
std::string StringConcat(const std::vector<std::string>& elems, char delim);
std::string& StringToLower(std::string& ori);
std::string& StringToUpper(std::string& ori);
std::string IpPortString(const std::string& ip, int port);
std::string ToRead(const std::string& str);
bool ParseIpPortString(const std::string& ip_port, std::string& ip, int& port);
std::string StringTrim(const std::string& ori, const std::string& charlist = " ");
std::string RandomHexChars(size_t len);
std::string RandomString(size_t len);
std::string RandomStringWithNumber(size_t len);

bool StringHasSpaces(const std::string& str);

}  // namespace pstd
