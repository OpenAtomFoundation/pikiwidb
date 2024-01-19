#include "raft.h"
#include <braft/raft.h>
#include <butil/status.h>
#include <cassert>
#include <memory>
#include "config.h"

namespace pikiwidb {

PRaft& PRaft::Instance() {
  static PRaft store;
  return store;
}

butil::Status PRaft::Init(std::string& clust_id) {
  assert(clust_id.size() == RAFT_DBID_LEN);
  this->_dbid = clust_id;

  // FIXME: g_config.ip is default to 127.0.0.0, which may not work in cluster.
  auto raw_addr = g_config.ip + ":" + std::to_string(g_config.port + RAFT_PORT_OFFSET);
  butil::EndPoint addr(butil::my_ip(), g_config.port + RAFT_PORT_OFFSET);
  braft::NodeOptions node_options;

  // Default init in one node.
  if (node_options.initial_conf.parse_from(raw_addr) != 0) {
    LOG(ERROR) << "Fail to parse configuration, address: " << raw_addr;
    return {EINVAL, "Fail to parse address."};
  }

  node_options.fsm = this;
  node_options.node_owns_fsm = false;
  std::string prefix = "local://" + g_config.dbpath;
  node_options.log_uri = prefix + "/log";
  node_options.raft_meta_uri = prefix + "/raft_meta";
  node_options.snapshot_uri = prefix + "/snapshot";

  _node = std::make_unique<braft::Node>(clust_id, braft::PeerId(addr));
  if (_node->init(node_options) != 0) {
    _node.reset();
    LOG(ERROR) << "Fail to init raft node";
    return {EINVAL, "Fail to init raft node"};
  }

  return {0, "OK"};
}

bool PRaft::IsLeader() const {
  if (!_node) {
    LOG(ERROR) << "Node is not initialized";
    return false;
  }
  return _node->is_leader();
}

butil::Status PRaft::AddPeer(const std::string& peer) {
  if (!_node) {
    LOG(ERROR) << "Node is not initialized";
    return {EINVAL, "Node is not initialized"};
  }

  braft::SynchronizedClosure done;
  _node->add_peer(peer, &done);
  done.wait();

  if (!done.status().ok()) {
    LOG(WARNING) << "Fail to add peer " << peer << " to node " << _node->node_id() << ", status " << done.status();
    return done.status();
  }

  return {0, "OK"};
}

butil::Status PRaft::RemovePeer(const std::string& peer) {
  if (!_node) {
    LOG(ERROR) << "Node is not initialized";
    return {EINVAL, "Node is not initialized"};
  }

  braft::SynchronizedClosure done;
  _node->remove_peer(peer, &done);
  done.wait();

  if (!done.status().ok()) {
    LOG(WARNING) << "Fail to remove peer " << peer << " from node " << _node->node_id() << ", status " << done.status();
    return done.status();
  }

  return {0, "OK"};
}

// Shut this node down.
void PRaft::ShutDown() {
  if (_node) {
    _node->shutdown(nullptr);
  }
}

// Blocking this thread until the node is eventually down.
void PRaft::Join() {
  if (_node) {
    _node->join();
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