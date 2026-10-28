#define MLN_BUILDING_C

#include <cstddef>

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

auto mln_delivery_queue_drain(
  mln_delivery_queue queue, mln_delivery_batch* out_batch,
  mln_delivery_batch_view* out_view, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::delivery::drain_delivery_queue(queue, out_batch, out_view);
  });
}

auto mln_delivery_queue_dispatch(
  mln_delivery_queue queue, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::delivery::dispatch_delivery_queue(queue);
  });
}

auto mln_delivery_batch_take(
  mln_delivery_batch batch, std::size_t index, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::delivery::take_delivery(batch, index);
  });
}

void mln_delivery_batch_release(mln_delivery_batch batch) noexcept {
  mln::delivery::release_delivery_batch(batch);
}
