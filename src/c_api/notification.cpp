#define MLN_BUILDING_C

#include "maplibre_native_c/notification.h"

#include "c_api/boundary.hpp"
#include "notification/notification.hpp"

extern "C" MLN_API auto mln_notification_source_create(
  mln_notification_source* out_source
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::create_notification_source(out_source);
  });
}

extern "C" MLN_API auto mln_notification_source_set_callback(
  mln_notification_source source, mln_notification_callback callback,
  void* user_data
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::set_notification_callback(source, callback, user_data);
  });
}

extern "C" MLN_API auto mln_notification_source_clear_callback(
  mln_notification_source source
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::clear_notification_callback(source);
  });
}

extern "C" MLN_API auto mln_notification_source_drain_ready(
  mln_notification_source source, mln_ready_batch* out_batch
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::drain_notification_ready(source, out_batch);
  });
}

extern "C" MLN_API auto mln_ready_batch_get(
  mln_ready_batch batch, mln_ready_batch_view* out_view
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::get_ready_batch(batch, out_view);
  });
}

extern "C" MLN_API void mln_ready_batch_release(
  mln_ready_batch batch
) noexcept {
  mln::core::release_ready_batch(batch);
}

extern "C" MLN_API void mln_notification_source_release(
  mln_notification_source source
) noexcept {
  mln::core::release_notification_source(source);
}
