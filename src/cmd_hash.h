/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <random>

#include "base_cmd.h"
#include "hash.h"

namespace pikiwidb {

class HSetCmd : public BaseCmd {
 public:
  HSetCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class HGetCmd : public BaseCmd {
 public:
  HGetCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class HMSetCmd : public BaseCmd {
 public:
  HMSetCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class HMGetCmd : public BaseCmd {
 public:
  HMGetCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class HGetAllCmd : public BaseCmd {
 public:
  HGetAllCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class HKeysCmd : public BaseCmd {
 public:
  HKeysCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class HLenCmd : public BaseCmd {
 public:
  HLenCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class HStrLenCmd : public BaseCmd {
 public:
  HStrLenCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class HRandFieldCmd : public BaseCmd {
 public:
  HRandFieldCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoWithPositiveCount(PClient *client, const PHash *hash, int64_t count, bool with_value);
  void DoWithNegativeCount(PClient *client, const PHash *hash, int64_t count, bool with_value);

  std::random_device rd_;

  static const inline std::string kWithValueString{"withvalues"};
};

}  // namespace pikiwidb
