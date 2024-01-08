#include "pikiwidb_state_machine.h"
#include "log.h"

int PikiwidbStateMachine::start() { return 0; }

bool PikiwidbStateMachine::is_leader() const { return _leader_term.load(butil::memory_order_acquire) > 0; }

// Shut this node down.
void PikiwidbStateMachine::shutdown() {
  if (_node) {
    _node->shutdown(nullptr);
  }
}

// Blocking this thread until the node is eventually down.
void PikiwidbStateMachine::join() {
  if (_node) {
    _node->join();
  }
}

// @braft::StateMachine
void PikiwidbStateMachine::on_apply(braft::Iterator& iter) {
  // A batch of tasks are committed, which must be processed through
  // |iter|
  for (; iter.valid(); iter.next()) {
  }
}

struct SnapshotArg {
  braft::SnapshotWriter* writer;
  braft::Closure* done;
};

void PikiwidbStateMachine::on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) {}

int PikiwidbStateMachine::on_snapshot_load(braft::SnapshotReader* reader) { return 0; }

void PikiwidbStateMachine::on_leader_start(int64_t term) { _leader_term.store(term, butil::memory_order_release); }
void PikiwidbStateMachine::on_leader_stop(const butil::Status& status) {
  _leader_term.store(-1, butil::memory_order_release);
}

void PikiwidbStateMachine::on_shutdown() {}
void PikiwidbStateMachine::on_error(const ::braft::Error& e) {}
void PikiwidbStateMachine::on_configuration_committed(const ::braft::Configuration& conf) {}
void PikiwidbStateMachine::on_stop_following(const ::braft::LeaderChangeContext& ctx) {}
void PikiwidbStateMachine::on_start_following(const ::braft::LeaderChangeContext& ctx) {}
// end of @braft::StateMachine