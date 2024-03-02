/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <string_view>
#include "base_cmd.h"

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

class HDelCmd : public BaseCmd {
 public:
  HDelCmd(const std::string &name, int16_t arity);

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

class HScanCmd : public BaseCmd {
 public:
  HScanCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;

  static constexpr const char *kMatchSymbol = "match";
  static constexpr const char *kCountSymbol = "count";
};

class HValsCmd : public BaseCmd {
 public:
  HValsCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class HIncrbyFloatCmd : public BaseCmd {
 public:
  HIncrbyFloatCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class HSetNXCmd : public BaseCmd {
 public:
  HSetNXCmd(const std::string &name, int16_t arity);
  
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
  static constexpr std::string_view kWithValueString = "withvalues";
};

}  // namespace pikiwidb
