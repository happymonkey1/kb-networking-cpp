#include "kb/kb_networking.h"
#include "kb_testing.h"
#include <stddef.h>
#include <string.h>

int main(int argc, char **argv) {
  kb_networking_init();

  kb_log_internal_str(KB_LOG_LEVEL_INFO, "Initialized kb-networking with C ABI");
  kb_server_t *server = kb_server_create();
  const uint16_t port = 11124;
  kb_server_start_manual(server, port);

  KB_ASSERT(server != NULL, "Server is null?");
  KB_ASSERT(kb_server_is_running(server), "Server should be running");
  KB_ASSERT_EQ(kb_server_port(server), port, "Port should be %d", port);

  const char * message = "Hello from server!";
  kb_server_broadcast(server, message, strlen(message), true);

  kb_server_destroy(server);
  kb_log_internal_str(KB_LOG_LEVEL_INFO, "Destroyed server");
  kb_log_internal_str(KB_LOG_LEVEL_INFO, "Destroying kb-networking with C ABI");
  kb_networking_shutdown();

  return 0;
}