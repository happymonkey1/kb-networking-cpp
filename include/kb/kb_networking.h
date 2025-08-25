//
// Created by happymonkey1 on 8/16/25.
//

#ifndef KB_NETWORKING_CPP_KB_NETWORKING_H
#define KB_NETWORKING_CPP_KB_NETWORKING_H

#include "kb/kb_core.h"
#include "kb/net/kb_server.h"
#include "kb/net/kb_client.h"

#define KB_NETWORKING_VERSION_MAJOR 0
#define KB_NETWORKING_VERSION_MINOR 1
#define KB_NETWORKING_VERSION_PATCH 0

#define KB_MAGIC_BYTE_0 0x4B
#define KB_MAGIC_BYTE_1 0x42
#define KB_MAGIC_BYTE_2 0x4C
#define KB_MAGIC_BYTE_3 0x4B

#ifndef KB_OWNING_BUFFER_INIT_SIZE
#  define KB_OWNING_BUFFER_INIT_SIZE 8192
#endif

#ifdef __cplusplus
extern "C" {
#endif

KB_API void kb_networking_init();
KB_API void kb_networking_shutdown();

KB_API void kb_networking_get_version(
  uint32_t * major,
  uint32_t * minor,
  uint32_t * patch
);

KB_API void kb_free(void * p_ptr);

#ifdef __cplusplus
}
#endif

#endif  //KB_NETWORKING_CPP_KB_NETWORKING_H
