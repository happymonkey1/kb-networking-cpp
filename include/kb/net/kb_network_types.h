//
// Created by happymonkey1 on 8/18/25.
//

#ifndef KB_NETWORKING_KB_TYPES_H
#define KB_NETWORKING_KB_TYPES_H

#include <stdint.h>

typedef enum kb_status_t : int32_t {
  KB_STATUS_OK                    = 0,
  KB_STATUS_ERROR_INVALID_ARG     = -1,
  KB_STATUS_ERROR_BAD_STATE       = -2,
  KB_STATUS_ERROR_ALLOC           = -3,
  KB_STATUS_ERROR_IO              = -4,
  KB_STATUS_ERROR_TIMEOUT         = -5,
  KB_STATUS_ERROR_NOT_IMPLEMENTED = -6,
  // Reserved for stable ABI
  KB_STATUS_ERROR_INTERNAL         = -127,
} kb_status_t;

typedef uint32_t kb_packet_type_t;

// Internal reserved packet types
typedef enum kb_packet_id : int32_t {
  // Uncategorized packet serializer_t
  KB_PACKET_ID_UNKNOWN         = 0,
  // Reserved for stable ABI
  KB_PACKET_ID_RESERVED = 127,
} kb_packet_id_t;

typedef uint32_t kb_connection_t;
const kb_connection_t KB_CONNECTION_INVALID = 0;

#endif  //KB_NETWORKING_KB_TYPES_H
