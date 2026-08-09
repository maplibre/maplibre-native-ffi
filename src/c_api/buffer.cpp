#include "bytes/buffer.hpp"

#include "c_api/boundary.hpp"
#include "maplibre_native_c/base.h"

extern "C" {

mln_status mln_buffer_get(
  mln_buffer buffer, mln_buffer_view* out_view
) noexcept {
  return mln::c_api::status_boundary([&] {
    return mln::core::buffer_get(buffer, out_view);
  });
}

void mln_buffer_destroy(mln_buffer buffer) noexcept {
  mln::core::buffer_destroy(buffer);
}

}  // extern "C"
