#include "kb/net/packet.hpp"
#include "kb/net/client.hpp"

namespace kb::net {

auto awaitable_packet_t::await_suspend(std::coroutine_handle<> p_handle) {
  m_client_ptr->register_packet_awaiter(
    m_type,
    {
      .m_handle = p_handle,
      .m_data = &m_data,
    }
  );
}

}