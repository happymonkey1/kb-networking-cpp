//
// Created by happy on 8/19/2025.
//

#include <string_view>
#include "../kb_testing.h"
#include "kb/core/owning_buffer.hpp"
#include "kb/kb_networking_cpp.hpp"

auto test_owning_buffer_consume_succeeds() -> void {
  constexpr std::string_view data = "kablunk";
  auto * data_copy = new kb::u8[data.size()];
  std::memcpy(data_copy, data.data(), data.size());

  const auto buffer = kb::core::OwningBuffer::consume(
    &data_copy,
    data.size()
  );

  KB_ASSERT_EQ(data_copy, nullptr, "Expected data_copy pointer to be set to null");

  const auto * data_ptr = reinterpret_cast<const char *>(buffer.data());
  for (size_t i = 0; i < data.size(); ++i) {
    KB_LOG_INFO("Checking byte: {}", i);
    KB_ASSERT_EQ(data_ptr[i], data[i], "Expected %c, found %c at byte %d", data[i], data_ptr[i], i);
  }
}

auto main([[maybe_unused]] int argc, [[maybe_unused]] char ** argv) -> int {
  kb_networking_init();
  test_owning_buffer_consume_succeeds();
  kb_networking_shutdown();
  return 0;
}