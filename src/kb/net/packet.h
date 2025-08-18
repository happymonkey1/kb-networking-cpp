//
// Created by happymonkey1 on 8/17/25.
//

#ifndef KB_NETWORKING_PACKET_H
#define KB_NETWORKING_PACKET_H

#include "../types.h"

#include <msgpack.hpp>

#include <coroutine>

namespace kb::net {

using packet_type_t = u32;

struct awaitable_packet_t {
  struct promise_type_t {
    msgpack::object m_result;
    std::coroutine_handle<> m_handle;

    auto get_return_object() -> awaitable_packet_t {
      return awaitable_packet_t{
        .m_handle = std::coroutine_handle<promise_type_t>::from_promise(*this)
      };
    }
  };

  std::coroutine_handle<promise_type_t> m_handle;

  auto await_ready() const noexcept { return false; }

  auto await_suspend(std::coroutine_handle<promise_type_t> p_handle) -> void {
    p_handle.promise().m_handle = p_handle;
  }

  auto await_resume() -> msgpack::object {
    return m_handle.promise().m_result;
  }
};

} // end namespace kb::net

#endif  //KB_NETWORKING_PACKET_H
