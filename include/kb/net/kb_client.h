//
// Created by happymonkey1 on 8/18/25.
//

#ifndef KB_NETWORKING_KB_CLIENT_H
#define KB_NETWORKING_KB_CLIENT_H

#include "kb/kb_core.h"
#include "kb/net/kb_network_types.h"
#include <stddef.h>

typedef struct kb_client kb_client_t;
typedef struct kb_wait   kb_wait_t;

typedef enum kb_client_connection_status : uint32_t {
  KB_CLIENT_CONNECTION_DISCONNECTED = 0,
  KB_CLIENT_CONNECTION_CONNECTING   = 1,
  KB_CLIENT_CONNECTION_CONNECTED    = 2,
  KB_CLIENT_CONNECTION_FAILED       = 3,
} kb_client_connection_status_t;

typedef void (* kb_packet_handler_func_t)(
  kb_packet_type_t type, const void* payload, size_t payload_len, void* user);

typedef void (* kb_connect_callback_func_t)(int success, void* user);

typedef void (* kb_packet_callback_func_t)(
  const void* payload, size_t payload_len, void* user);

// Create a client
KB_API kb_client_t * kb_client_create(void);
// Destroy a client and free allocated memory
KB_API void          kb_client_destroy(kb_client_t * p_client);
// Disconnect from server and stop reserved thread
KB_API void          kb_client_stop(kb_client_t * p_client);
// Manually poll for incoming messages
KB_API void          kb_client_poll(kb_client_t * p_client);
// Retrieves the current connection status
KB_API void          kb_client_get_connection_status(kb_client_t * p_client);

KB_API kb_status_t
kb_client_connect(kb_client_t* p_client, const char* p_address, uint16_t p_port);

KB_API kb_status_t
kb_client_async_connect(kb_client_t               * p_client,
                        const char                * p_address,
                        uint16_t                    p_port,
                        kb_connect_callback_func_t  p_connect_callback,
                        void                      * p_user_data);

KB_API kb_wait_t *
kb_client_wait_connect(kb_client_t * p_client,
                       const char  * p_address,
                       uint16_t      p_port);

KB_API kb_status_t
kb_client_bind_handler(kb_client_t* p_client,
                       kb_packet_type_t p_type,
                       kb_packet_handler_func_t p_handler,
                       void* p_user_data);

KB_API void
kb_client_unbind_handler(kb_client_t* p_client, kb_packet_type_t p_type);

KB_API kb_status_t
kb_client_send_raw(kb_client_t* p_client,
                   const void* p_user_data,
                   size_t p_len,
                   int p_reliable);

KB_API kb_status_t
kb_client_send_msgpack(kb_client_t * p_client,
                       kb_packet_type_t p_type,
                       const void* p_msgpack_data,
                       size_t p_msgpack_len,
                       int p_reliable);

/* Async wait API */

// Asynchronously wait for a packet serializer_t, invoking a callback function on recv.
KB_API kb_status_t
kb_client_async_wait_for_packet(kb_client_t* p_client,
                                kb_packet_type_t p_type,
                                kb_packet_callback_func_t p_on_packet_callback,
                                void* p_user_data);

// Asynchronously wait for a packet serializer_t by a wait handle.
// Caller can block or poll, then copy_steam_to_ip_address data out.
KB_API kb_wait_t *
kb_client_wait_for_packet(kb_client_t      * p_client,
                          kb_packet_type_t   p_packet_type);

// TODO: consider moving to dedicated header

// Check whether a wait handle is ready
KB_API int32_t
kb_wait_try_ready(kb_wait_t * p_wait);

// Block on a wait handle.
// Caller should check for KB_STATUS_OK or KB_STATUS_ERROR_TIMEOUT
KB_API kb_status_t
kb_wait_block(kb_wait_t * p_wait, uint32_t p_timeout_ms);

// Steal a wait handle's data
// Caller should check for KB_STATUS_OK or KB_STATUS_ERROR_TIMEOUT
KB_API kb_status_t
kb_wait_take(kb_wait_t * p_wait, void ** p_out, size_t * p_out_len);

// Free memory allocated for a wait handle
KB_API void kb_wait_destroy(kb_wait_t * p_wait);

#endif  //KB_NETWORKING_KB_CLIENT_H
