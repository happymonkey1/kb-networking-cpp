# kb-networking-cpp

## Example Usage

```c++
auto main([[maybe_unused]] int argc, [[maybe_unused]] char ** argv) -> int {
  // Initialize underlying library resources
  kb_networking_init();
  KB_LOG_DEBUG("[test_client] Initialized kb-networking library");

  // Create a packet handling server which uses msgpack for underlying serialization
  auto server = kb::net::UdpPacketServer<kb::net::serialization_type_t::msgpack>::create();
  
  KB_LOG_INFO("[test_client] Created server");
  
  const auto bind_ping_res = server->bind_packet_handler(
    static_cast<kb::net::packet_type_t>(test_packet_type_t::Ping),
    [&server](
      kb_connection_t p_conn,
      const msgpack::object_handle & p_object
    ) -> void {
      KB_LOG_INFO("[test_client] Server handler received ping packet from conn: {}", static_cast<kb::u32>(p_conn));

      // Deserialize `Ping` data using msgpack
      ping_data_t ping_data{};
      try {
        const auto object = p_object.get();
        object.convert(ping_data);
      } catch (std::exception& err) {
        KB_LOG_ERROR("Test failed to deserialize expected ping data: {}", err.what());
        kb::core::Logger::get_core_logger()->flush();
        throw err;
      }

      // Send a `Pong` packet to the client which sent the `Ping` packet
      const auto pong_res = server->send(
        p_conn,
        static_cast<kb::net::packet_type_t>(test_packet_type_t::Pong),
        pong_data_t{ .packet_id = k_expected_pong_id },
        true
      );
    }
  );
  
  const kb::u16 port = 12345;
  // Start the server on a dedicated thread
  const auto start_async_res = server->start(port);

  // Create a packet handling client which uses msgpack for underlying serialization
  auto client = kb::net::udp::create_msgpack_client();
  
  KB_LOG_INFO("[test_client] Created client");
  
  // Bind a `Pong` handler on the client
  // Handler is invoked on any `Pong` packet
  const auto bind_pong_res = client->bind_packet_handler(
    static_cast<kb::net::packet_type_t>(test_packet_type_t::Pong),
    [&client](
      const msgpack::object_handle & p_object
    ) -> void {
      KB_LOG_INFO("[test_client] Received pong packet from server");
    }
  );

  // Connect to the server
  const auto connect_res = client->connect("127.0.0.1", port);

  // Wait for connection to succeed
  constexpr kb::u32 max_conn_iters = 1000 / 10;
  kb::u32 conn_iter = 0;
  while (client->get_connection_status() != kb::net::UdpClient::connection_status_t::connected) {
    if (conn_iter++ >= max_conn_iters) {
      // Failed to connect in timeout period
      return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Send a `Ping` packet to the server
  const auto send_pack_res = client->send_packet(
    static_cast<kb::net::packet_type_t>(test_packet_type_t::Ping),
    ping_data_t{ .packet_id = k_expected_ping_id },
    true
  );

  // Wait for `Pong` response from server
  constexpr kb::u32 max_iters = 1000 / 10;
  kb::u32 iter = 0;
  while (pong_count == 0) {
    if (iter++ >= max_iters) {
      // Failed to connect in timeout period
      return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Close client and free underlying resources
  client->stop();
  // Close server and free underlying resources
  server->stop();

  KB_LOG_INFO("Destroying kb-networking library");
  
  // Free underlying library resources
  kb_networking_shutdown();
  
  return 0;
}
```

More examples are provided in `examples` and `tests`.

## Build From Source

Add an env variable for the `kbnetworkingcpp` vcpkg overlay and the root of the project:
```bash
export KB_NETWORKING_ROOT=/PATH/TO/kb-networking-cpp
export VCPKG_OVERLAY_PORTS=PATH/TO/kb-networking-cpp/vcpkg-overlays
```

Run the build script:
```bash
./scripts/build-all.sh
```

### Manual

Set up the build files:
`cmake --preset default -B ./build`

After build CMake files:
`cmake --build build --config Release -j --clean-first --target kb-networking kb-networking-cli`