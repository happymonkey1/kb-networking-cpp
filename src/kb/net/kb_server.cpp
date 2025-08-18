//
// Created by happymonkey1 on 8/16/25.
//

#include "kb/net/kb_server.h"
#include "../../../include/kb/core/logger.hpp"
#include "server.hpp"

#include <mutex>

struct kb_server {
  kb::net::Server *m_impl = nullptr;

  kb_on_data_func_t m_on_data = nullptr;
  kb_on_connect_func_t m_on_connect = nullptr;
  kb_on_disconnect_func_t m_on_disconnect = nullptr;

  bool m_async = false;

  std::mutex m_mutex;
};

KB_API kb_server_t * kb_server_create(void) {
  return new kb_server_t{
    .m_impl = new kb::net::Server{},
    .m_on_data = nullptr,
    .m_on_connect = nullptr,
    .m_on_disconnect = nullptr,
    .m_async = false,
    .m_mutex = std::mutex{},
  };
}

KB_API void kb_server_destroy(kb_server_t * p_server) {
  if (!p_server) {
    return;
  }

  kb_server_stop(p_server);
  delete p_server->m_impl;
  delete p_server;
}

KB_API bool kb_server_start_async(kb_server_t * p_server, const uint16_t p_port) {
  if (!p_server) {
    KB_LOG_ERROR("Failed to start a null server");
    return false;
  }

  p_server->m_async = true;
  return p_server->m_impl->start_async(p_port);
}

KB_API void kb_server_stop(kb_server_t * p_server) {
  if (!p_server) {
    return;
  }

  p_server->m_impl->stop();
}

KB_API bool kb_server_start_manual(kb_server_t * p_server, uint16_t p_port) {
  if (!p_server) {
    return false;
  }

  return p_server->m_impl->start_manual(p_port);
}

KB_API void kb_server_poll(kb_server_t * p_server) {
  if (!p_server) {
    return;
  }

  p_server->m_impl->poll();
}

KB_API bool kb_server_send(kb_server_t * p_server, kb_conn_t p_conn,
                           const void * p_data, uint32_t p_len,
                           bool p_reliable) {
  if (!p_server) {
    return false;
  }

  return p_server->m_impl->send_raw(p_conn, p_data, p_len, p_reliable);
}

KB_API bool kb_server_broadcast(kb_server_t * p_server, const void* p_data,
                                uint32_t p_len, bool p_reliable) {
  if (!p_server) {
    return false;
  }

  return p_server->m_impl->broadcast_raw(p_data, p_len, p_reliable);
}

KB_API void kb_server_disconnect(kb_server_t * p_server, kb_conn_t p_conn,
                                 int p_reason_code) {
  if (!p_server) {
    return;
  }

  p_server->m_impl->disconnect(p_conn, p_reason_code);
}

KB_API bool kb_server_is_running(kb_server_t * p_server) {
  if (!p_server) {
    return false;
  }

  return p_server->m_impl->is_running();
}

KB_API uint16_t kb_server_port(kb_server_t * p_server) {
  if (!p_server) {
    return 0;
  }

  return p_server->m_impl->port();
}

