//
// Created by happymonkey1 on 8/17/25.
//

#ifndef KB_NETWORKING_AWAITABLE_HPP
#define KB_NETWORKING_AWAITABLE_HPP
#include <coroutine>
#include <mutex>
#include <optional>

namespace kb::async {
template <typename T>
class awaitable {
public:
  awaitable() = default;

  awaitable(const awaitable& p_other) = default;
  awaitable& operator=(const awaitable&) = default;
  awaitable(awaitable&& p_other) noexcept
    : m_handle{ p_other.m_handle }, m_exception{ p_other.m_exception },
      m_mutex{ std::move(p_other.m_mutex) }, m_result{ p_other.m_result } {}
  awaitable& operator=(awaitable&&) noexcept = default;

  auto set_result(T p_value) -> void {
    std::unique_lock lock{ m_mutex };
    m_result = std::move(p_value);

    if (!m_handle) {
      return;
    }

    const auto handle = m_handle;
    lock.unlock();
    handle.resume();
  }

  auto set_exception(std::exception_ptr p_exception_ptr) {
    std::unique_lock lock{ m_mutex };
    m_exception = p_exception_ptr;
    if (!m_handle) {
      return;
    }

    const auto handle = m_handle;
    lock.unlock();
    handle.resume();
  }

  struct awaiter {
    awaitable * m_parent;

    auto await_ready() noexcept -> bool {
      std::lock_guard lock{ m_parent->m_mutex };
      return m_parent->m_result.has_value() || m_parent->m_exception;
    }

    auto await_suspend(std::coroutine_handle<> p_handle) -> void {
      std::lock_guard lock{ m_parent->m_mutex };
      m_parent->m_handle = p_handle;
    }

    auto await_resume() -> T {
      std::lock_guard lock{ m_parent->m_mutex };
      if (m_parent->m_exception) {
        std::rethrow_exception(m_parent->m_exception);
      }

      return std::move(*m_parent->m_result);
    }
  };

  awaiter operator co_await() noexcept { return awaiter{ this }; }

private:
  std::mutex m_mutex;
  std::optional<T> m_result;
  std::exception_ptr m_exception;
  // Handler which is set when the coroutine is suspended
  std::coroutine_handle<> m_handle{};
};

template <>
class awaitable<void> {
public:
  auto set_result() -> void {
    std::unique_lock lock{ m_mutex };
    m_ready = true;

    if (!m_handle) {
      return;
    }

    const auto handle = m_handle;
    lock.unlock();
    handle.resume();
  }

  auto set_exception(std::exception_ptr p_exception_ptr) -> void {
    std::unique_lock lock{ m_mutex };
    m_exception = p_exception_ptr;
    if (!m_handle) {
      return;
    }

    const auto handle = m_handle;
    lock.unlock();
    handle.resume();
  }

  struct awaiter {
    awaitable* m_parent;

    auto await_ready() noexcept -> bool{
      std::lock_guard lock{ m_parent->m_mutex };
      return m_parent->m_ready || m_parent->m_exception;
    }

    auto await_suspend(std::coroutine_handle<> p_handle) -> void {
      std::lock_guard lk{ m_parent->m_mutex };
      m_parent->m_handle = p_handle;
    }

    auto await_resume() -> void {
      std::lock_guard lock{ m_parent->m_mutex };
      if (m_parent->m_exception) {
        std::rethrow_exception(m_parent->m_exception);
      }
    }
  };

  awaiter operator co_await() noexcept { return awaiter{ this }; }

private:
  std::mutex m_mutex;
  bool m_ready = false;
  std::exception_ptr m_exception;
  std::coroutine_handle<> m_handle{};
};

// Simple coroutine serializer_t for void coroutines
struct task {
  struct promise_type {
    task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
};

} // end namespace kb::async

#endif  //KB_NETWORKING_AWAITABLE_HPP
