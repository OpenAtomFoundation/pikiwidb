#include "raft.h"
#include <cassert>
#include <string>
#include "client.h"
#include "config.h"

namespace pikiwidb {

PRaft& PRaft::Instance() {
  static PRaft store;
  return store;
}

butil::Status PRaft::Init(std::string& cluster_id) {
  assert(clust_id.size() == RAFT_DBID_LEN);
  this->_dbid = cluster_id;

  // FIXME: g_config.ip is default to 127.0.0.0, which may not work in cluster.
  auto raw_addr = g_config.ip + ":" + std::to_string(g_config.port + RAFT_PORT_OFFSET);
  butil::EndPoint addr(butil::my_ip(), g_config.port + RAFT_PORT_OFFSET);

  // Default init in one node.
  if (node_options_.initial_conf.parse_from(raw_addr) != 0) {
    LOG(ERROR) << "Fail to parse configuration, address: " << raw_addr;
    return {EINVAL, "Fail to parse address."};
  }

  node_options_.fsm = this;
  node_options_.node_owns_fsm = false;
  std::string prefix = "local://" + g_config.dbpath;
  node_options_.log_uri = prefix + "/log";
  node_options_.raft_meta_uri = prefix + "/raft_meta";
  node_options_.snapshot_uri = prefix + "/snapshot";

  node_ = std::make_unique<braft::Node>(cluster_id, braft::PeerId(addr));
  if (node_->init(node_options_) != 0) {
    node_.reset();
    LOG(ERROR) << "Fail to init raft node";
    return {EINVAL, "Fail to init raft node"};
  }

  return {0, "OK"};
}

bool PRaft::IsLeader() const {
  if (!node_) {
    LOG(ERROR) << "Node is not initialized";
    return false;
  }
  return node_->is_leader();
}

void PRaft::SendNodeAddRequest(PClient *client) {
  assert(client);

  UnboundedBuffer req;
  req.PushData("RAFT.NODE ADD ", 13);
  // Node id in braft are ip:port, the node id param in RAFT.NODE ADD cmd will be ignored.
  int unused_node_id = 0;
  req.PushData(std::to_string(unused_node_id).c_str(), std::to_string(unused_node_id).size());
  req.PushData(" ", 1);
  req.PushData(raw_addr_.data(), raw_addr_.size());
  client->SendPacket(req);
}

int PRaft::ProcessClusterJoinCmdResponse(const char* start, int len) {
  assert(start);
  auto cli = join_ctx_.GetClient();
  if (!cli) {
    LOG(WARNING) << "No client when processing cluster join cmd response.";
    return 0;
  }

  // TODO(KKorpse): Check the response.
  cli->SetRes(CmdRes::kOK);
  cli->SendPacket(cli->Message());
  return len;

  // Redis process cluster join cmd response like this:
  
  // static void handleNodeAddResponse(redisAsyncContext *c, void *r, void *privdata)
  // {
  //     Connection *conn = privdata;
  //     JoinLinkState *state = ConnGetPrivateData(conn);
  //     RedisRaftCtx *rr = ConnGetRedisRaftCtx(conn);

  //     redisReply *reply = r;

  //     if (!reply) {
  //         LOG_WARNING("RAFT.NODE ADD failed: connection dropped.");
  //         ConnMarkDisconnected(conn);
  //     } else if (reply->type == REDIS_REPLY_ERROR) {
  //         /* -MOVED? */
  //         if (strlen(reply->str) > 6 && !strncmp(reply->str, "MOVED ", 6)) {
  //             NodeAddr addr;
  //             if (!parseMovedReply(reply->str, &addr)) {
  //                 LOG_WARNING("RAFT.NODE ADD failed: invalid MOVED response: %s", reply->str);
  //             } else {
  //                 LOG_VERBOSE("Join redirected to leader: %s:%d", addr.host, addr.port);
  //                 NodeAddrListAddElement(&state->addr, &addr);
  //             }
  //         } else if (strlen(reply->str) > 12 && !strncmp(reply->str, "CLUSTERDOWN ", 12)) {
  //             LOG_WARNING("RAFT.NODE ADD error: %s, retrying.", reply->str);
  //         } else {
  //             LOG_WARNING("RAFT.NODE ADD failed: %s", reply->str);
  //             state->failed = true;
  //         }
  //     } else if (reply->type != REDIS_REPLY_ARRAY || reply->elements != 2) {
  //         LOG_WARNING("RAFT.NODE ADD invalid reply.");
  //     } else {
  //         LOG_NOTICE("Joined Raft cluster, node id: %lu, dbid: %.*s",
  //                    (unsigned long) reply->element[0]->integer,
  //                    (int) reply->element[1]->len, reply->element[1]->str);

  //         strncpy(rr->snapshot_info.dbid, reply->element[1]->str, reply->element[1]->len);
  //         rr->snapshot_info.dbid[RAFT_DBID_LEN] = '\0';

  //         rr->config.id = reply->element[0]->integer;
  //         state->complete_callback(state->req);
  //         RedisModule_Assert(rr->state == REDIS_RAFT_UP);

  //         ConnAsyncTerminate(conn);
  //     }

  //     redisAsyncDisconnect(c);
  // }
}

butil::Status PRaft::AddPeer(const std::string& peer) {
  if (!node_) {
    LOG(ERROR) << "Node is not initialized";
    return {EINVAL, "Node is not initialized"};
  }

  braft::SynchronizedClosure done;
  node_->add_peer(peer, &done);
  done.wait();

  if (!done.status().ok()) {
    LOG(WARNING) << "Fail to add peer " << peer << " to node " << node_->node_id() << ", status " << done.status();
    return done.status();
  }

  return {0, "OK"};
}

butil::Status PRaft::RemovePeer(const std::string& peer) {
  if (!node_) {
    LOG(ERROR) << "Node is not initialized";
    return {EINVAL, "Node is not initialized"};
  }

  braft::SynchronizedClosure done;
  node_->remove_peer(peer, &done);
  done.wait();

  if (!done.status().ok()) {
    LOG(WARNING) << "Fail to remove peer " << peer << " from node " << node_->node_id() << ", status " << done.status();
    return done.status();
  }

  return {0, "OK"};
}

void PRaft::OnJoinCmdConnectionFailed([[maybe_unused]] EventLoop* loop, const char* peer_ip, int port) {
  auto cli = join_ctx_.GetClient();
  if (cli) {
    cli->SetRes(CmdRes::kErrOther,
                "RAFT.NODE ADD failed: connection dropped.  " + std::string(peer_ip) + ":" + std::to_string(port));
    cli->SendPacket(cli->Message());
  }
}

// Shut this node down.
void PRaft::ShutDown() {
  if (node_) {
    node_->shutdown(nullptr);
  }
}

// Blocking this thread until the node is eventually down.
void PRaft::Join() {
  if (node_) {
    node_->join();
  }
}

// @braft::StateMachine
void PRaft::on_apply(braft::Iterator& iter) {
  // A batch of tasks are committed, which must be processed through
  // |iter|
  for (; iter.valid(); iter.next()) {
  }
}

void PRaft::on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) {}

int PRaft::on_snapshot_load(braft::SnapshotReader* reader) { return 0; }

void PRaft::on_leader_start(int64_t term) {}
void PRaft::on_leader_stop(const butil::Status& status) {}

void PRaft::on_shutdown() {}
void PRaft::on_error(const ::braft::Error& e) {}
void PRaft::on_configuration_committed(const ::braft::Configuration& conf) {}
void PRaft::on_stop_following(const ::braft::LeaderChangeContext& ctx) {}
void PRaft::on_start_following(const ::braft::LeaderChangeContext& ctx) {}

}  // namespace pikiwidb