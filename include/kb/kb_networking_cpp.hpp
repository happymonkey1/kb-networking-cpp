//
// Created by happymonkey1 on 8/16/25.
//

#ifndef KB_NETWORKING_CPP_KB_NETWORKING_CPP_H
#define KB_NETWORKING_CPP_KB_NETWORKING_CPP_H

#ifndef __cplusplus
#  error "This header should only be used with C++"
#endif

#include "core/logger.hpp"
#include "kb_networking.h"

// TODO: these shouldn't be exposed
#include "kb/types.h"
#include "kb/net/udp/udp.hpp"
#include "kb/net/udp/server/udp_packet_server.hpp"
#include "kb/net/udp/server/udp_server.hpp"
#include "kb/net/udp/client/udp_packet_client.hpp"
#include "kb/net/udp/client/udp_client.hpp"
#include "kb/net/udp/client/async_udp_packet_client.h"
#include "kb/net/udp/serde.hpp"

#endif  //KB_NETWORKING_CPP_KB_NETWORKING_CPP_H
