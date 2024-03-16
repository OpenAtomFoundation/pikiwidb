/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include "base_cmd.h"

namespace pikiwidb {

class DelCmd : public BaseCmd {
 public:
  DelCmd(const std::string& name, int16_t arity);

 protected:
  bool DoInitial(PClient* client) override;

 private:
  void DoCmd(PClient* client) override;
};

class ExistsCmd : public BaseCmd {
 public:
  ExistsCmd(const std::string& name, int16_t arity);

 protected:
  bool DoInitial(PClient* client) override;

 private:
  void DoCmd(PClient* client) override;
};

class TypeCmd : public BaseCmd {
  public:
   TypeCmd(const std::string& name, int16_t arity);

  protected:
   bool DoInitial(PClient* client) override;

  private:
   void DoCmd(PClient* client) override;
};

class ExpireCmd : public BaseCmd {
  public:
   ExpireCmd(const std::string& name, int16_t arity);

  protected:
   bool DoInitial(PClient* client) override;

  private:
   void DoCmd(PClient* client) override;
};

class TtlCmd : public BaseCmd {
  public:
   TtlCmd(const std::string& name, int16_t arity);

  protected:
   bool DoInitial(PClient* client) override;

  private:
   void DoCmd(PClient* client) override;
};

class PExpireCmd : public BaseCmd {
 public:
  PExpireCmd(const std::string& name, int16_t arity);

 protected:
  bool DoInitial(PClient* client) override;

 private:
  void DoCmd(PClient* client) override;
};

class ExpireatCmd : public BaseCmd {
 public:
  ExpireatCmd(const std::string& name, int16_t arity);

 protected:
  bool DoInitial(PClient* client) override;

 private:
  void DoCmd(PClient* client) override;
};

class PExpireatCmd : public BaseCmd {
 public:
  PExpireatCmd(const std::string& name, int16_t arity);

 protected:
  bool DoInitial(PClient* client) override;

 private:
  void DoCmd(PClient* client) override;
};

}  // namespace pikiwidb
