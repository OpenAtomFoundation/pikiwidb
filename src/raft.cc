#include "raft.h"
#include <braft/raft.h>
#include <butil/status.h>

namespace pikiwidb {

PRaft& PRaft::Instance() {
  static PRaft store;
  return store;
}

int PRaft::start() { return 0; }

bool PRaft::is_leader() const {
  if (!_node) {
    LOG(ERROR) << "Node is not initialized";
    return false;
  }
  return _node->is_leader();
}

butil::Status PRaft::add_peer(const std::string& peer) {
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

butil::Status PRaft::remove_peer(const std::string& peer) {
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
void PRaft::shutdown() {
  if (_node) {
    _node->shutdown(nullptr);
  }
}

// Blocking this thread until the node is eventually down.
void PRaft::join() {
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