/**
 * @file maplibre_native_c/notification.h
 * Receiver-scoped notification and ready-batch API.
 *
 * This header targets C23.
 */

#ifndef MAPLIBRE_NATIVE_C_NOTIFICATION_H
#define MAPLIBRE_NATIVE_C_NOTIFICATION_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c/base.h"  // IWYU pragma: export

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTBEGIN(modernize-use-using,modernize-use-trailing-return-type)

/** The kind of service endpoint reported in a ready batch. */
typedef enum mln_notification_endpoint_kind : uint32_t {
  MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS = 1,
  MLN_NOTIFICATION_ENDPOINT_OPERATION = 2,
  MLN_NOTIFICATION_ENDPOINT_ADAPTER_RESOURCE_REQUESTS = 3,
  MLN_NOTIFICATION_ENDPOINT_ADAPTER_LOG_RECORDS = 4,
  MLN_NOTIFICATION_ENDPOINT_RENDER_FRAMES = 5,
  MLN_NOTIFICATION_ENDPOINT_DRIVER_WORK = 6,
} mln_notification_endpoint_kind;

/** One endpoint that was ready when a ready batch was drained. */
typedef struct mln_ready_endpoint {
  uint32_t size;
  /** One mln_notification_endpoint_kind value. */
  uint32_t kind;
  /** The endpoint's opaque C handle value. */
  uint64_t id;
} mln_ready_endpoint;

/**
 * A borrowed view of one owned ready batch.
 *
 * Step through endpoints by endpoint_size. The pointers remain valid until the
 * ready-batch handle is released.
 */
typedef struct mln_ready_batch_view {
  uint32_t size;
  uint32_t endpoint_size;
  const mln_ready_endpoint* endpoints;
  size_t endpoint_count;
} mln_ready_batch_view;

/**
 * Notifies the receiver that services a notification source.
 *
 * Native code may invoke this function from any thread. Calls may coalesce or
 * overlap. The callback receives no borrowed payload. It may schedule receiver
 * work or use non-blocking C API calls to service endpoints inline, including
 * mln_notification_source_drain_ready(). A host that services inline must
 * serialize drains of the same source.
 *
 * The callback must not replace or clear itself, release its notification
 * source, or call an API that can block. Inline service is suitable only when
 * the callback thread is valid for every endpoint it services.
 */
typedef void (*mln_notification_callback)(void* user_data);

/** Creates an unregistered notification source for one host receiver. */
MLN_API mln_status mln_notification_source_create(
  mln_notification_source* out_source
) MLN_NOEXCEPT;

/**
 * Installs or replaces the callback for a notification source.
 *
 * The call prevents new entries into the replaced callback and waits for every
 * in-flight entry to return before it releases the old callback and user_data.
 * If an endpoint is already ready, the new callback is invoked before this call
 * returns. The callback and user_data remain borrowed until replacement,
 * clearing, or source release.
 */
MLN_API mln_status mln_notification_source_set_callback(
  mln_notification_source source, mln_notification_callback callback,
  void* user_data
) MLN_NOEXCEPT;

/**
 * Clears a notification callback and waits for every in-flight entry to return.
 * Ready endpoint state remains stored by the source.
 */
MLN_API mln_status mln_notification_source_clear_callback(
  mln_notification_source source
) MLN_NOEXCEPT;

/**
 * Drains an owned snapshot of the endpoints that are ready.
 *
 * Runtime-event and adapter-queue endpoints remain ready until their typed
 * drain reports that their queue is empty. Operation-completion readiness is
 * consumed by this drain. An empty drain and the check that clears the source's
 * signaled state are atomic with endpoint publication, so a later publication
 * schedules another callback.
 */
MLN_API mln_status mln_notification_source_drain_ready(
  mln_notification_source source, mln_ready_batch* out_batch
) MLN_NOEXCEPT;

/** Borrows the endpoint view stored by an owned ready batch. */
MLN_API mln_status mln_ready_batch_get(
  mln_ready_batch batch, mln_ready_batch_view* out_view
) MLN_NOEXCEPT;

/** Releases an owned ready batch. A null handle is a no-op. */
MLN_API void mln_ready_batch_release(mln_ready_batch batch) MLN_NOEXCEPT;

/**
 * Releases the public notification source handle.
 *
 * This prevents new callback entries, waits for every in-flight entry, and
 * retires the public handle. Associated endpoints retain the source's internal
 * state until they detach, but no longer produce callbacks or public readiness.
 * A null or already released handle is a no-op.
 */
MLN_API void mln_notification_source_release(
  mln_notification_source source
) MLN_NOEXCEPT;

// NOLINTEND(modernize-use-using,modernize-use-trailing-return-type)

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_NOTIFICATION_H
