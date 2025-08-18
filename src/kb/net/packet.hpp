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

class Client;

struct awaitable_packet_t {
  auto await_ready() const noexcept { return false; }

  auto await_suspend(std::coroutine_handle<> p_handle);

  auto await_resume() -> std::optional<msgpack::object> {
    return std::move(m_data);
  }

private:
  friend class Client;

  awaitable_packet_t(Client * p_client, packet_type_t p_type)
    : m_client_ptr{ p_client }, m_type{ p_type }, m_data{ std::nullopt } {}

  Client * m_client_ptr;
  packet_type_t m_type;
  std::optional<msgpack::object> m_data;
};

} // end namespace kb::net

#endif  //KB_NETWORKING_PACKET_H
