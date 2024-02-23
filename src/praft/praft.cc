#include "praft.h"
#include <cassert>
#include <string>
#include "client.h"
#include "config.h"
#include "pstd_string.h"
#include "braft/configuration.h"
#include "event_loop.h"
#include "pikiwidb.h"

namespace pikiwidb {

PRaft& PRaft::Instance() {
  static PRaft store;
  return store;
}

butil::Status PRaft::Init(std::string& cluster_id, bool initial_conf_is_null) {
  if (node_ && server_) {
    return {0, "OK"};
  }

  server_ = std::make_unique<brpc::Server>();
  DummyServiceImpl service(&PRAFT);
  auto port = g_config.port + RAFT_PORT_OFFSET;
  // Add your service into RPC server
  if (server_->AddService(&service, 
                        brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
      LOG(ERROR) << "Fail to add service";
      return {EINVAL, "Fail to add service"};
  }
  // raft can share the same RPC server. Notice the second parameter, because
  // adding services into a running server is not allowed and the listen
  // address of this server is impossible to get before the server starts. You
  // have to specify the address of the server.
  if (braft::add_service(server_.get(), port) != 0) {
      LOG(ERROR) << "Fail to add raft service";
      return {EINVAL, "Fail to add raft service"};
  }

  // It's recommended to start the server before Counter is started to avoid
  // the case that it becomes the leader while the service is unreacheable by
  // clients.
  // Notice the default options of server is used here. Check out details from
  // the doc of brpc if you would like change some options;
  if (server_->Start(port, NULL) != 0) {
      LOG(ERROR) << "Fail to start Server";
      return {EINVAL, "Fail to start Server"};
  }

  // It's ok to start PRaft;
  assert(cluster_id.size() == RAFT_DBID_LEN);
  this->dbid_ = cluster_id;

  // FIXME: g_config.ip is default to 127.0.0.0, which may not work in cluster.
  raw_addr_ = g_config.ip + ":" + std::to_string(port); 
  butil::EndPoint addr(butil::my_ip(), port);

  // Default init in one node.
  /*
  initial_conf takes effect only when the replication group is started from an empty node. 
  The Configuration is restored from the snapshot and log files when the data in the replication group is not empty. 
  initial_conf is used only to create replication groups. 
  The first node adds itself to initial_conf and then calls add_peer to add other nodes. 
  Set initial_conf to empty for other nodes. 
  You can also start empty nodes simultaneously by setting the same inital_conf(ip:port of multiple nodes) for multiple nodes.
  */
  std::string initial_conf("");
  if (!initial_conf_is_null) {
    initial_conf = raw_addr_ + ":0,";
  }
  if (node_options_.initial_conf.parse_from(initial_conf) != 0) {
    LOG(ERROR) << "Fail to parse configuration, address: " << raw_addr_;
    return {EINVAL, "Fail to parse address."};
  }

  // node_options_.election_timeout_ms = FLAGS_election_timeout_ms;
  node_options_.fsm = this;
  node_options_.node_owns_fsm = false;
  // node_options_.snapshot_interval_s = FLAGS_snapshot_interval;
  std::string prefix = "local://" + g_config.dbpath;
  node_options_.log_uri = prefix + "/log";
  node_options_.raft_meta_uri = prefix + "/raft_meta";
  node_options_.snapshot_uri = prefix + "/snapshot";
  // node_options_.disable_cli = FLAGS_disable_cli;
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

std::string PRaft::GetLeaderId() const {
  if (!node_) {
    LOG(ERROR) << "Node is not initialized";
    return std::string("Fail to get leader id");
  }
  return node_->leader_id().to_string();
}

std::string PRaft::GetNodeId() const {
  if (!node_) {
    LOG(ERROR) << "Node is not initialized";
    return std::string("Fail to get node id");
  }
  return node_->node_id().to_string();
}

std::string PRaft::GetClusterId() const {
  if (!node_) {
    LOG(ERROR) << "Node is not initialized";
    return std::string("Fail to get cluster id");
  }
  return dbid_;
}

void PRaft::SendNodeAddRequest(PClient *client) {
  assert(client);

  // Node id in braft are ip:port, the node id param in RAFT.NODE ADD cmd will be ignored.
  int unused_node_id = 0;
  auto port = g_config.port + RAFT_PORT_OFFSET;
  auto raw_addr = g_config.ip + ":" + std::to_string(port);
  UnboundedBuffer req;
  req.PushData("RAFT.NODE ADD ", 14);
  req.PushData(std::to_string(unused_node_id).c_str(), std::to_string(unused_node_id).size());
  req.PushData(" ", 1);
  req.PushData(raw_addr.data(), raw_addr.size());
  req.PushData("\r\n", 2);
  client->SendPacket(req);
}

std::tuple<int, bool> PRaft::ProcessClusterJoinCmdResponse(const char* start, int len) {
  assert(start);
  auto join_client = join_ctx_.GetClient();
  if (!join_client) {
    LOG(WARNING) << "No client when processing cluster join cmd response.";
    return std::make_tuple(0, true);
  }

  bool is_disconnet = true;
  std::string reply(start, len);
  if (reply.find("+OK") != std::string::npos) {
    pstd::StringTrimLeft(reply, "+OK");
    pstd::StringTrim(reply);

    // initialize the slave node
    auto s = PRAFT.Init(reply, true);
    if (!s.ok()) {
      join_client->SetRes(CmdRes::kErrOther, s.error_str());
      join_client->SendPacket(join_client->Message());
      return std::make_tuple(len, is_disconnet);
    }

    LOG(INFO) << "Joined Raft cluster, node id:" << PRAFT.GetNodeId() << "dbid:" << PRAFT.dbid_;
    join_client->SetRes(CmdRes::kOK);
    join_client->SendPacket(join_client->Message());
    is_disconnet = false;
  } else if (reply.find("-ERR wrong leader") != std::string::npos) {
    // Resolve the ip address of the leader
    pstd::StringTrimLeft(reply, "-ERR wrong leader");
    pstd::StringTrim(reply);
    braft::PeerId peerId;
    peerId.parse(reply);

    // Establish a connection with the leader and send the add request
    auto on_new_conn = [](TcpConnection* obj) {
      if (g_pikiwidb) {
        g_pikiwidb->OnNewConnection(obj);
      }
    };
    auto fail_cb = [&](EventLoop* loop, const char* peer_ip, int port) {
      PRAFT.OnJoinCmdConnectionFailed(loop, peer_ip, port);
    };

    auto loop = EventLoop::Self();
    auto peer_ip = std::string(butil::ip2str(peerId.addr.ip).c_str());
    auto port = peerId.addr.port;
    // FIXME: The client here is not smart pointer, may cause undefined behavior.
    // should use shared_ptr in DoCmd() rather than raw pointer.
    PRAFT.GetJoinCtx().Set(join_client, peer_ip, port);
    loop->Connect(peer_ip.c_str(), port, on_new_conn, fail_cb);

    // Not reply any message here, we will reply after the connection is established.
    join_client->Clear();
  } else {
    LOG(ERROR) << "Joined Raft cluster fail, " << start;
    join_client->SetRes(CmdRes::kErrOther, std::string(start, len));
    join_client->SendPacket(join_client->Message());
  }

  return std::make_tuple(len, is_disconnet);

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

// Shut this node and server down.
void PRaft::ShutDown() {
  if (node_) {
    node_->shutdown(nullptr);
  }

  if (server_) {
    server_->Stop(0);
  }
}

// Blocking this thread until the node is eventually down.
void PRaft::Join() {
  if (node_) {
    node_->join();
  }

  if (server_) {
    server_->Join();
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