/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once
#include "base_cmd.h"

namespace pikiwidb {

class ZAddCmd : public BaseCmd {
 public:
  ZAddCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  std::string key_;
  std::vector<storage::ScoreMember> score_members_;
  void DoCmd(PClient *client) override;
};

class ZRevrangeCmd : public BaseCmd {
 public:
  ZRevrangeCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class ZRangebyscoreCmd : public BaseCmd {
 public:
  ZRangebyscoreCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class ZRevRangeByScoreCmd : public BaseCmd {
 public:
  ZRevRangeByScoreCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class ZCardCmd : public BaseCmd {
 public:
  ZCardCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class ZRankCmd : public BaseCmd {
 public:
  ZRankCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class ZRevrankCmd : public BaseCmd {
 public:
  ZRevrankCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class ZRemCmd : public BaseCmd {
 public:
  ZRemCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class ZIncrbyCmd : public BaseCmd {
 public:
  ZIncrbyCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

}  // namespace pikiwidb