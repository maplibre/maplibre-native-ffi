#pragma once

#include <memory>
#include <string>

#include "handles/handle_table.hpp"
#include "maplibre_native_c/base.h"

namespace mln::core {

struct BufferObject {
  std::string bytes;
};

template <>
struct HandleTraits<BufferObject> {
  static constexpr auto kind = HandleKind::Buffer;
  static constexpr auto leasable = false;
};

auto buffer_table() -> HandleTable<BufferObject>&;
auto create_buffer(std::string bytes, mln_buffer* out_buffer) -> mln_status;
auto buffer_get(mln_buffer buffer, mln_buffer_view* out_view) -> mln_status;
auto buffer_destroy(mln_buffer buffer) -> void;

}  // namespace mln::core
