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

KB_API kb_server *kb_server_create(void) {
  return new kb_server{
    .m_impl = new kb::net::Server{},
    .m_on_data = nullptr,
    .m_on_connect = nullptr,
    .m_on_disconnect = nullptr,
    .m_async = false,
    .m_mutex = std::mutex{},
  };
}

KB_API void kb_server_destroy(kb_server *p_server) {
  if (!p_server) {
    return;
  }

  kb_server_stop(p_server);
  delete p_server->m_impl;
  delete p_server;
}

KB_API bool kb_server_start_async(kb_server * p_server, const uint16_t p_port) {
  if (!p_server) {
    KB_LOG_ERROR("Failed to start a null server");
    return false;
  }

  p_server->m_async = true;
  return p_server->m_impl->start_async(p_port);
}

KB_API void kb_server_stop(kb_server * p_server) {
  if (!p_server) {
    return;
  }

  p_server->m_impl->stop();
}

