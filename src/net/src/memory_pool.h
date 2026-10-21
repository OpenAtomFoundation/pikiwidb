// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PIKA_SRC_NET_SRC_MEMORY_POOL_H
#define PIKA_SRC_NET_SRC_MEMORY_POOL_H

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>

/**
 * page size = pageSize_ + pageOffset_
 * page layout
 * |-----------|------------------|
 * | page head | Available memory |
 * | ----------|------------------|
 * |   8 bit   |   pageSize_ bit  |
 * |-----------|------------------|
 * The page head is used to distinguish the current position in the page table.
 * If the page is extra, the page head is 0xFF
 */
namespace net {

class MemoryPool {
 public:
  explicit MemoryPool() : MemoryPool(512) {}
  explicit MemoryPool(int64_t pageSize) : pageSize_(pageSize) {
    for (int i = 0; i < 64; i++) {
      pages_[i] = nullptr;
    }
  }

  MemoryPool(const MemoryPool &) = delete;
  MemoryPool &operator=(const MemoryPool &) = delete;
  MemoryPool(MemoryPool &&) = delete;
  MemoryPool &operator=(MemoryPool &&) = delete;

  ~MemoryPool() {
    for (int i = 0; i < 64; i++) {
      if (pages_[i] != nullptr) {
        auto ptr = reinterpret_cast<uint8_t *>(pages_[i]);
        std::free(--ptr);
      }
    }
  };

  template <typename T, typename... Args>
  T *Allocate(Args &&...args) {
    if (sizeof(T) > pageSize_) {
      return AllocateExtend<T>(std::forward<Args>(args)...);
    }
    // If you are in a particularly high multithreaded race scenario
    // you can put bits_.load() into the for loop
    auto bit = bits_.load();
    for (int i = 0; i < 64;) {
      uint64_t _bit = 1ull << i;
      if (!(bit & _bit)) {  // If the i-th bit is 0, it means that the i-th page is not used
        if (bits_.compare_exchange_strong(bit, bit | _bit)) {  // Set the i-th bit to 1
          if (!pages_[i]) {  // If the i-th page is not allocated, allocate the i-th page
            pages_[i] = std::malloc(pageSize_ + pageOffset_);
            auto page = reinterpret_cast<uint8_t *>(pages_[i]);
            *page = uint8_t(i);  // Set the page head to i
            pages_[i] = ++page;
          }
          return new (pages_[i]) T(std::forward<Args>(args)...);
        } else {
          bit = bits_.load();
          if (!(bit & _bit)) {
            continue;  // If the i-th bit is 0, it means that the i-th page is not used
          }
        }
      }
      ++i;
    }

    // If we reach here, it means that all pages are full
    // We need to allocate a new page
    return AllocateExtend<T>(std::forward<Args>(args)...);
  }

  template <typename T>
  void Deallocate(T *ptr) {
    // Get the page head
    auto page = reinterpret_cast<uint8_t *>(ptr) - 1;
    ptr->~T();

    if (*page == extendFlag_) {  // If the page head is 0xFF, it means that the page is extra
      std::free(page);
      return;
    }

    auto index = *page;
    bits_.fetch_and(~(1ull << index));
  }

 private:
  template <typename T, typename... Args>
  T *AllocateExtend(Args &&...args) {
    auto newPage = std::aligned_alloc(alignof(T), sizeof(T) + pageOffset_);
    auto page = reinterpret_cast<uint8_t *>(newPage);
    *page = extendFlag_;  // Set the page head to 0xFF
    return new (++page) T(std::forward<Args>(args)...);
  }

 private:
  const int64_t pageSize_;                     // The size of each page
  const int8_t pageOffset_ = sizeof(uint8_t);  // The size of the page head
  const uint8_t extendFlag_ = 0xFF;
  std::atomic<uint64_t> bits_ = 0;  // The bits_ is used to record the status of each page
  std::array<void *, 64> pages_{};  // The pages_ is used to store the address of each page
};

}  // namespace net

#endif  // PIKA_SRC_NET_SRC_MEMORY_POOL_H
