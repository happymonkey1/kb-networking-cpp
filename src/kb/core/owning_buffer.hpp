//
// Created by happymonkey1 on 8/18/25.
//

#ifndef KB_NETWORKING_BUFFER_H
#define KB_NETWORKING_BUFFER_H

#include "kb/types.h"
#include "kb/kb_networking.h"

#include <memory>
#include <cstring>

namespace kb::core {

// Owning buffer
class OwningBuffer {
public:
  using value_t = u8;

  // Casts in reserved logic assume that this serializer_t is 1 by
  static_assert(sizeof(value_t) == 1, "OwningBuffer value_t should be 1 byte");
  
  OwningBuffer(const std::size_t p_size = KB_OWNING_BUFFER_INIT_SIZE) noexcept
    : m_alloc_size{ p_size } {
    if (m_alloc_size) {
      m_data = static_cast<value_t*>(::malloc(m_size));
    }
  }

  ~OwningBuffer() {
    ::free(m_data);
  }

  OwningBuffer(const OwningBuffer& p_other) noexcept = delete;
  auto operator=(const OwningBuffer& p_other) noexcept = delete;

  OwningBuffer(OwningBuffer&& p_other) noexcept
    : m_data{ p_other.m_data }, m_size{ p_other.m_size }, m_alloc_size{ p_other.m_alloc_size } {
    p_other.m_data = nullptr;
    p_other.m_size = 0;
    p_other.m_alloc_size = 0;
  }

  auto operator=(OwningBuffer&& p_other) noexcept -> OwningBuffer& {
    ::free(m_data);

    m_data = p_other.m_data;
    m_size = p_other.m_size;
    m_alloc_size = p_other.m_alloc_size;
    p_other.m_data = nullptr;
    p_other.m_size = 0;
    p_other.m_alloc_size = 0;
    return *this;
  }

  static auto consume(
    value_t ** p_data,
    const std::size_t p_size
  ) noexcept -> OwningBuffer {
    OwningBuffer buffer{ p_size };
    buffer.m_data = *p_data;
    buffer.m_size = p_size;
    *p_data = nullptr;
    return buffer;
  }

  static auto copy(const OwningBuffer& p_other) noexcept -> OwningBuffer {
    OwningBuffer buffer{ p_other.m_size };
    std::memcpy(buffer.m_data, p_other.m_data, p_other.m_size);
    return std::move(buffer);
  }

  static auto copy(const value_t * p_data, const std::size_t p_size) noexcept -> OwningBuffer {
    OwningBuffer buffer{ p_size };
    std::memcpy(buffer.m_data, p_data, p_size);
    return std::move(buffer);
  }

  // Write into the end of the buffer
  // Internally resizes if the new data exceeds the current allocation size
  auto write(const value_t * p_data, std::size_t p_size) noexcept -> void {
    KB_ASSERT(m_data || m_size == 0, "OwningBuffer expected allocated buffer or to be empty");

    if (!m_data) {
      return;
    }

    if (m_alloc_size - m_size < p_size) {
      resize(p_size);
    }

    std::memcpy(m_data + m_size, p_data, p_size);
    m_size += p_size;
  }

  auto data() const noexcept -> const value_t* { return m_data; }
  auto data() noexcept -> value_t* { return m_data; }

  auto size() const noexcept -> std::size_t { return m_size; }

  auto clear() noexcept -> void {
    m_size = 0;
  }

  auto release() noexcept -> value_t * {
    value_t * temp = m_data;
    m_data = nullptr;
    m_size = 0;
    m_alloc_size = 0;
    return temp;
  }

  auto destroy() noexcept -> void {
    ::free(m_data);
    m_data = nullptr;
    m_size = 0;
    m_alloc_size = 0;
  }
private:
  auto resize(const std::size_t p_size) noexcept -> void {
    std::size_t new_alloc_size = m_alloc_size > 0 ?
      m_alloc_size * 2 : KB_OWNING_BUFFER_INIT_SIZE;

    while (m_size + p_size > new_alloc_size) {
      const auto resized = new_alloc_size * 2;
      if (resized <= new_alloc_size) {
        new_alloc_size = m_size + p_size;
        break;
      }

      new_alloc_size = resized;
    }

    auto * new_buffer = static_cast<value_t *>(::realloc(m_data, new_alloc_size));
    m_data = new_buffer;
    m_alloc_size = new_alloc_size;
  }

private:
  value_t * m_data = nullptr;
  std::size_t m_size = 0;
  std::size_t m_alloc_size = 0;
};

}



#endif  //KB_NETWORKING_BUFFER_H
