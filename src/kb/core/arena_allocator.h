//
// Created by happy on 8/24/2025.
//

#ifndef KB_NETWORKING_ARENA_ALLOCATOR_H
#define KB_NETWORKING_ARENA_ALLOCATOR_H

#include "kb/types.h"

#include <memory>
#include <mutex>
#include <stdexcept>
#include <atomic>

namespace kb::core {

template <bool TrivialDeallocationOnly = true>
class Arena {
 public:
  explicit Arena(std::size_t arena_size_bytes) {
    if (arena_size_bytes == 0) {
      throw std::invalid_argument("Arena size must be greater than zero.");
    }
    m_memory_block = std::make_unique<std::byte[]>(arena_size_bytes);
    m_start_ptr = m_memory_block.get();
    m_current_ptr.store(m_start_ptr, std::memory_order_relaxed);
    m_end_ptr = m_start_ptr + arena_size_bytes;
    m_total_size = arena_size_bytes;
  }

  Arena(const Arena &) = delete;
  auto operator=(const Arena &) -> Arena & = delete;
  Arena(Arena &&) = delete;
  auto operator=(Arena &&) -> Arena & = delete;

  ~Arena() = default;

  /**
   * @brief Allocates a block of memory of a given size and alignment.
   *
   * This is the primary allocation function. It is thread-safe.
   *
   * @param size The number of bytes to allocate.
   * @param alignment The required alignment of the memory. Must be a power of two.
   * @return A pointer to the allocated memory, or nullptr if the arena is full.
   */
  [[nodiscard]] auto allocate(size_t size, size_t alignment = alignof(std::max_align_t)) -> void * {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::byte * current_local = m_current_ptr.load(std::memory_order_relaxed);

    void * aligned_ptr = current_local;
    size_t available_space = m_end_ptr - current_local;

    if (std::align(alignment, size, aligned_ptr, available_space)) {
      std::byte * new_current_ptr = static_cast<std::byte*>(aligned_ptr) + size;
      m_current_ptr.store(new_current_ptr, std::memory_order_relaxed);
      return aligned_ptr;
    }

    return nullptr;
  }

  /**
   * @brief A helper to allocate and construct an object of type T.
   *
   * This function allocates memory and then uses placement new to construct
   * an object in place. It forwards arguments to the object's constructor.
   *
   * @tparam T The type of object to create.
   * @tparam TArgs The types of arguments for T's constructor.
   * @param args The arguments to forward to the constructor of T.
   * @return A pointer to the newly constructed object, or nullptr on allocation failure.
   */
  template <typename T, typename... TArgs>
  [[nodiscard]] auto create(TArgs&&... args) -> T * {
    if constexpr (TrivialDeallocationOnly) {
      static_assert(std::is_trivially_destructible_v<T>,
                    "Cannot create objects with non-trivial destructors in this arena type. "
                    "Instantiate Arena<false> to override.");
    }

    void * ptr = allocate(sizeof(T), alignof(T));
    if (ptr) {
      return new (ptr) T(std::forward<TArgs>(args)...);
    }
    return nullptr;
  }

  /**
   * @brief Resets the arena, invalidating all previously allocated pointers.
   */
  auto reset() noexcept -> void {
    std::lock_guard<std::mutex> lock{ m_mutex };
    m_current_ptr.store(m_start_ptr, std::memory_order_relaxed);
  }

  /**
   * @brief Returns the total allocation size of the underlying memory buffer.
   */
  auto get_total_size() const noexcept -> std::size_t { return m_total_size; }

  /**
   * @brief Returns the currently allocated size of the underlying memory buffer.
   */
  auto get_used_size() const noexcept -> std::size_t { return m_current_ptr.load(std::memory_order_relaxed) - m_start_ptr; }

  /**
   * @brief Returns the size of the un-allocated memory in the underlying memory buffer.
   */
  auto get_remaining_size() const noexcept -> std::size_t { return get_total_size() - get_used_size(); }

 private:
  std::unique_ptr<std::byte[]>   m_memory_block;
  std::byte                    * m_start_ptr = nullptr;
  std::byte                    * m_end_ptr = nullptr;
  std::size_t                    m_total_size = 0;
  std::atomic<std::byte*>        m_current_ptr{nullptr};
  mutable std::mutex             m_mutex;
};

} // end namespace kb::core

#endif  //KB_NETWORKING_ARENA_ALLOCATOR_H
