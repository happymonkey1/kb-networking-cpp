//
// Created by happymonkey1 on 8/16/25.
//

#ifndef KB_NETWORKING_CPP_KB_SERVER_H
#define KB_NETWORKING_CPP_KB_SERVER_H

#include "kb/kb_core.h"
#include "kb/net/kb_network_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kb_server kb_server_t;
typedef uint32_t kb_conn_t;

typedef void (* kb_on_data_func_t)(kb_server_t * p_server,
                              kb_conn_t    p_conn,
                              const void * p_data,
                              uint32_t     p_len,
                              void *       p_user_data);

typedef void (* kb_on_connect_func_t)(kb_server_t* p_server,
                                 kb_conn_t p_conn,
                                 void *    p_user_data);

typedef void (* kb_on_disconnect_func_t)(kb_server_t * p_server,
                                    kb_conn_t p_conn,
                                    void *    p_user_data);

KB_API kb_server_t * kb_server_create(void);
KB_API void          kb_server_destroy(kb_server_t *p_server);

KB_API void kb_server_set_callbacks(kb_server_t *             p_server,
                                    kb_on_data_func_t       p_on_data,
                                    kb_on_connect_func_t    p_on_connect,
                                    kb_on_disconnect_func_t p_on_disconnect,
                                    void * p_user_data);

KB_API bool kb_server_start_async(kb_server_t * p_server, uint16_t p_port);
KB_API void kb_server_stop(kb_server_t * p_server);

KB_API bool kb_server_start_manual(kb_server_t * p_server, uint16_t p_port);
KB_API void kb_server_poll(kb_server_t * p_server);

// Send raw data to a specific client
KB_API bool kb_server_send_raw(kb_server_t * p_server,
                           kb_conn_t   p_conn,
                           const void * p_data,
                           uint32_t     p_len,
                           bool         p_reliable);

// Send raw data to all clients
KB_API bool kb_server_broadcast_raw(kb_server_t * p_server,
                                const void * p_data,
                                uint32_t     p_len,
                                bool         p_reliable);

KB_API void kb_server_disconnect(kb_server_t * p_server,
                                 kb_conn_t   p_conn,
                                 int         p_reason_code);

KB_API bool     kb_server_is_running(const kb_server_t * p_server);
KB_API uint16_t kb_server_port(const kb_server_t * p_server);


#ifdef __cplusplus
}
#endif

#endif  //KB_NETWORKING_CPP_KB_SERVER_H
