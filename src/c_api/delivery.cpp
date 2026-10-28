#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "delivery/delivery_queue.hpp"
#include "maplibre_native_c.h"

auto mln_delivery_queue_create(
  const mln_delivery_queue_options* options, mln_delivery_queue* out_queue,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::delivery::create_delivery_queue(options, out_queue);
  });
}

void mln_delivery_queue_release(mln_delivery_queue queue) noexcept {
  mln::delivery::release_delivery_queue(queue);
}

auto mln_delivery_queue_dispatch(
  mln_delivery_queue queue, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::delivery::dispatch_delivery_queue(queue);
  });
}
