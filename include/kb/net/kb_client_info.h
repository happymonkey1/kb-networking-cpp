//
// Created by happymonkey1 on 8/16/25.
//

#ifndef KB_NETWORKING_CPP_CLIENT_INFO_H
#define KB_NETWORKING_CPP_CLIENT_INFO_H

#include <stdint.h>

typedef uint32_t kb_conn_t;

typedef struct kb_client_info {
  kb_conn_t   conn;
  uint32_t    ip_v4;
  uint16_t    port;
  // Reserved to provide a stable ABI
  uint8_t     padding[10];
} kb_client_info_t;

#endif  //KB_NETWORKING_CPP_CLIENT_INFO_H
