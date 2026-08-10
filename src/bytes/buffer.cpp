#include <memory>
#include <string>
#include <utility>

#include "bytes/buffer.hpp"

#include "diagnostics/diagnostics.hpp"

namespace mln::core {

auto buffer_table() -> HandleTable<BufferObject>& {
  static auto table = HandleTable<BufferObject>{};
  return table;
}

auto create_buffer(std::string bytes, mln_buffer* out_buffer) -> mln_status {
  if (out_buffer == nullptr) {
    set_thread_error("out_buffer must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (*out_buffer != MLN_HANDLE_NULL) {
    set_thread_error("*out_buffer must be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto object =
    std::make_shared<BufferObject>(BufferObject{.bytes = std::move(bytes)});
  *out_buffer = buffer_table().insert(std::move(object));
  return MLN_STATUS_OK;
}

auto buffer_get(mln_buffer buffer, mln_buffer_view* out_view) -> mln_status {
  if (out_view == nullptr) {
    set_thread_error("out_view must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto lock = std::scoped_lock{buffer_table().mutex()};
  const auto* object = buffer_table().resolve_locked(buffer);
  if (object == nullptr) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  *out_view = mln_buffer_view{
    .data = object->bytes.data(),
    .size = object->bytes.size(),
  };
  return MLN_STATUS_OK;
}

auto buffer_destroy(mln_buffer buffer) -> void {
  if (buffer == MLN_HANDLE_NULL) {
    return;
  }
  buffer_table().remove(buffer);
}

}  // namespace mln::core
