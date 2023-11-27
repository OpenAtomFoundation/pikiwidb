#include "cmd_hash.h"

#include "command.h"

namespace pikiwidb {

HGetCmd::HGetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryHash) {}

bool HGetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HGetCmd::DoCmd(PClient* client) {
  UnboundedBuffer reply;
  std::vector<std::string> params(client->argv_.begin(), client->argv_.end());
  PError err = hget(params, &reply);
  if (err != PError_ok) {
    if (err == PError_notExist) {
      client->AppendString("");
    } else {
      client->SetRes(CmdRes::kErrOther, "hget cmd error");
    }
    return;
  }
  client->AppendStringRaw(reply.ReadAddr());
}

HMSetCmd::HMSetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryHash) {}

bool HMSetCmd::DoInitial(PClient* client) {
  if (client->argv_.size() % 2 != 0) {
    client->SetRes(CmdRes::kWrongNum, kCmdNameHMSet);
    return false;
  }
  client->SetKey(client->argv_[1]);
  return true;
}

void HMSetCmd::DoCmd(PClient* client) {
  UnboundedBuffer reply;
  std::vector<std::string> params(client->argv_.begin(), client->argv_.end());
  PError err = hmset(params, &reply);
  if (err != PError_ok) {
    if (err == PError_notExist) {
      client->AppendString("");
    } else {
      client->SetRes(CmdRes::kErrOther, "hmset cmd error");
    }
    return;
  }
  client->AppendStringRaw(reply.ReadAddr());
}

HMGetCmd::HMGetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryHash) {}

bool HMGetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HMGetCmd::DoCmd(PClient* client) {
  UnboundedBuffer reply;
  std::vector<std::string> params(client->argv_.begin(), client->argv_.end());
  PError err = hmget(params, &reply);
  if (err != PError_ok) {
    if (err == PError_notExist) {
      client->AppendString("");
    } else {
      client->SetRes(CmdRes::kErrOther, "hmget cmd error");
    }
    return;
  }
  client->AppendStringRaw(reply.ReadAddr());
}

}  // namespace pikiwidb
