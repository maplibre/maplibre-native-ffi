/**
 * @file maplibre_native_c/delivery.h
 * Public C API declarations for queue delivery of completions and host
 * callbacks.
 *
 * A host that cannot run code on a native thread, such as a Dart isolate or a
 * JavaScript engine, sets a delivery queue on a registration in place of its
 * callback. Native code then answers any synchronous decision itself, copies
 * the payload into an item that the queue owns, and wakes the queue when it
 * goes from empty to nonempty. The host drains owned batches on its own
 * thread, takes the handles it keeps, and releases each batch.
 *
 * Every registration whose callbacks a host need not answer on a native thread
 * accepts a queue: mln_completion, mln_runtime_event_handler, mln_log_handler,
 * mln_resource_provider, mln_resource_request_cancel_handler,
 * mln_resource_transform, mln_camera_transition_handler, and the custom
 * geometry and custom MVT vector source options.
 *
 * A queue is FIFO in push order. A map command queues the runtime events it
 * raises before pushing its completion. Events and completions that select
 * the same queue arrive in that order. Adjacent events share one delivery.
 *
 * A queue has no bound. A host that stops draining keeps every pushed item in
 * memory until it drains or releases the queue.
 *
 * Once process exit begins, pushes drop their items without disposing owned
 * values, and wakes stop.
 *
 * This header targets C23.
 */

#ifndef MAPLIBRE_NATIVE_C_DELIVERY_H
#define MAPLIBRE_NATIVE_C_DELIVERY_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"        // IWYU pragma: export
#include "completion.h"  // IWYU pragma: export
#include "logging.h"     // IWYU pragma: export
#include "map.h"         // IWYU pragma: export
#include "runtime.h"     // IWYU pragma: export
#include "style.h"       // IWYU pragma: export
#include "wake.h"        // IWYU pragma: export

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTBEGIN(modernize-use-using,modernize-use-trailing-return-type)

/** The kind of one drained delivery. */
typedef enum mln_delivery_type : uint32_t {
  /** The terminal result of a queue-mode mln_completion. */
  MLN_DELIVERY_TYPE_COMPLETION = 1,
  /** One record for a queue-mode mln_log_handler. */
  MLN_DELIVERY_TYPE_LOG_RECORD = 2,
  /** One request for a queue-mode mln_resource_provider. */
  MLN_DELIVERY_TYPE_RESOURCE_REQUEST = 3,
  /**
   * Last delivery of a callback registration in queue mode; replaces
   * release_user_data. No payload.
   */
  MLN_DELIVERY_TYPE_RETIRED = 4,
  /** One cancel for a queue-mode mln_resource_request_cancel_handler. */
  MLN_DELIVERY_TYPE_RESOURCE_REQUEST_CANCEL = 5,
  /** The end for a queue-mode mln_camera_transition_handler. */
  MLN_DELIVERY_TYPE_CAMERA_TRANSITION_END = 6,
  /** One tile fetch for a queue-mode custom source. */
  MLN_DELIVERY_TYPE_CUSTOM_SOURCE_FETCH_TILE = 7,
  /** One tile cancel for a queue-mode custom source. */
  MLN_DELIVERY_TYPE_CUSTOM_SOURCE_CANCEL_TILE = 8,
  /** Selected runtime and map events, in publication order. */
  MLN_DELIVERY_TYPE_RUNTIME_EVENTS = 9,
  /** One request for a queue-mode mln_resource_transform. */
  MLN_DELIVERY_TYPE_RESOURCE_TRANSFORM = 10,
} mln_delivery_type;

/** Copied mln_runtime_event_callback arguments. */
typedef struct mln_runtime_event_delivery {
  const mln_event_batch_view* events MLN_BINDING("lifetime=owner");
} mln_runtime_event_delivery MLN_BINDING("capture=manual");

/** Copied mln_log_callback arguments; fields mirror the callback parameters. */
typedef struct mln_log_delivery {
  mln_log_severity severity;
  mln_log_event event;
  int64_t code;
  /** UTF-8 message, owned by the batch and NUL-terminated after its bytes. */
  mln_buffer_view message MLN_BINDING("lifetime=owner");
} mln_log_delivery;

/** Copied mln_resource_provider_callback arguments. */
typedef struct mln_resource_request_delivery {
  /** The copied request, owned by the batch. */
  const mln_resource_request_info* request MLN_BINDING("lifetime=owner");
  /**
   * Owned by the batch until mln_delivery_batch_take(); releasing the batch
   * without taking it releases the handle, which fails the request.
   */
  mln_resource_request handle MLN_BINDING("ownership=owned");
} mln_resource_request_delivery;

/** Copied mln_resource_request_cancel_callback arguments. */
typedef struct mln_resource_request_cancel_delivery {
  /**
   * The cancelled request. The provider still holds its own reference, so the
   * batch owns nothing here.
   */
  mln_resource_request handle;
} mln_resource_request_cancel_delivery;

/** Copied mln_resource_transform_callback arguments. */
typedef struct mln_resource_transform_delivery {
  mln_resource_kind kind;
  /** The URL to transform, owned by the batch and NUL-terminated. */
  mln_buffer_view url MLN_BINDING("lifetime=owner");
  /**
   * Owned by the batch until mln_delivery_batch_take(); releasing the batch
   * without taking it releases the request, which keeps its URL.
   */
  mln_resource_transform_request request MLN_BINDING("ownership=owned");
} mln_resource_transform_delivery;

/** Copied mln_camera_transition_end_callback arguments. */
typedef struct mln_camera_transition_end_delivery {
  /** The copied end, owned by the batch. */
  const mln_camera_transition_end* end MLN_BINDING("lifetime=owner");
} mln_camera_transition_end_delivery;

/**
 * Copied mln_custom_source_fetch_tile_callback or
 * mln_custom_source_cancel_tile_callback arguments.
 */
typedef struct mln_custom_source_tile_delivery {
  mln_canonical_tile_id tile_id;
} mln_custom_source_tile_delivery;

/** Payload of one delivery, selected by mln_delivery.type. */
typedef union mln_delivery_payload {
  /**
   * The copied terminal result. Its diagnostic and value are owned by the
   * batch rather than borrowed for a callback.
   */
  mln_completion_result completion
    MLN_BINDING("variant=MLN_DELIVERY_TYPE_COMPLETION");
  mln_log_delivery log_record
    MLN_BINDING("variant=MLN_DELIVERY_TYPE_LOG_RECORD");
  mln_resource_request_delivery resource_request
    MLN_BINDING("variant=MLN_DELIVERY_TYPE_RESOURCE_REQUEST");
  mln_resource_request_cancel_delivery resource_request_cancel
    MLN_BINDING("variant=MLN_DELIVERY_TYPE_RESOURCE_REQUEST_CANCEL");
  mln_camera_transition_end_delivery camera_transition_end
    MLN_BINDING("variant=MLN_DELIVERY_TYPE_CAMERA_TRANSITION_END");
  mln_custom_source_tile_delivery custom_source_fetch_tile
    MLN_BINDING("variant=MLN_DELIVERY_TYPE_CUSTOM_SOURCE_FETCH_TILE");
  mln_custom_source_tile_delivery custom_source_cancel_tile
    MLN_BINDING("variant=MLN_DELIVERY_TYPE_CUSTOM_SOURCE_CANCEL_TILE");
  mln_runtime_event_delivery runtime_events
    MLN_BINDING("variant=MLN_DELIVERY_TYPE_RUNTIME_EVENTS");
  mln_resource_transform_delivery resource_transform
    MLN_BINDING("variant=MLN_DELIVERY_TYPE_RESOURCE_TRANSFORM");
} mln_delivery_payload;

/**
 * One drained delivery.
 *
 * Step through deliveries by mln_delivery_batch_view.delivery_size.
 */
typedef struct mln_delivery {
  mln_delivery_type type;
  uint32_t reserved MLN_BINDING("kind=reserved");
  /**
   * Opaque host token from the registration or completion, returned unchanged.
   * Native code never dereferences or releases it. The host keeps token state
   * until it handles the delivery or abandons the queue.
   */
  void* user_data MLN_BINDING("kind=context");
  /** Typed payload selected by type. */
  mln_delivery_payload payload
    MLN_BINDING("tag=type;empty_variant=MLN_DELIVERY_TYPE_RETIRED");
} mln_delivery;

/**
 * A borrowed view of one owned delivery batch, which
 * mln_delivery_queue_drain() writes.
 *
 * Every pointer in the view, including those inside each delivery's payload,
 * stays valid until the batch is released, independent of the queue, the
 * runtime, and any map.
 */
typedef struct mln_delivery_batch_view {
  uint32_t size;
  /**
   * Stride of one delivery in bytes, at least sizeof(mln_delivery) in the
   * header a caller compiled against. Index deliveries with this value.
   */
  uint32_t delivery_size;
  /** Borrowed array of delivery_count deliveries in push order. */
  const mln_delivery* deliveries
    MLN_BINDING("length=delivery_count;stride=delivery_size");
  /** Number of deliveries in deliveries. */
  size_t delivery_count;
} mln_delivery_batch_view;

/** Options for mln_delivery_queue_create(). All-zero members are defaults. */
typedef struct mln_delivery_queue_options {
  uint32_t size;
  /**
   * Wakes the receiver when the queue goes from empty to nonempty. A disabled
   * wake leaves service to the host's schedule.
   */
  mln_wake wake;
} mln_delivery_queue_options;

/**
 * Creates a delivery queue.
 *
 * MLN_STATUS_OK transfers the wake's callback, user_data, and
 * release_user_data to the queue. A rejected call leaves the wake's user_data
 * with the caller. The output must point to the null handle and receives
 * ownership of the queue on success.
 *
 * Returns:
 * - MLN_STATUS_OK when out_queue receives an owned queue.
 * - MLN_STATUS_INVALID_ARGUMENT when options is null, options->size is too
 *   small, the wake descriptor is invalid, or out_queue is null or does not
 *   point to the null handle.
 * - MLN_STATUS_NATIVE_ERROR when the queue could not be allocated.
 */
MLN_API mln_status mln_delivery_queue_create(
  const mln_delivery_queue_options* options,
  mln_delivery_queue* out_queue MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases a delivery queue. A null handle is a no-op.
 *
 * The call consumes the handle, closes the queue, and returns without blocking.
 * It discards pending items: an untaken provider request is released, which
 * fails it, and an untaken transform request is released, which keeps its
 * URL.
 *
 * Registrations that still name the queue keep working, but the queue refuses
 * their later pushes: a completion is discarded, a log record goes to the
 * platform logger, a provider request fails, a transform request keeps its
 * URL, and other queued callbacks are dropped.
 *
 * No wake call starts after release. The wake's release_user_data runs once
 * in-flight wake calls return, on whichever thread finishes last. The host may
 * release the queue from inside its own wake and from a finalizer. Batches
 * already drained stay readable.
 */
MLN_API void mln_delivery_queue_release(mln_delivery_queue queue) MLN_NOEXCEPT;

/**
 * Moves every queued delivery into a new owned batch and borrows its view.
 *
 * Deliveries that arrive later enter the next batch, and the wake fires again
 * when the first of them arrives. This function may be called from any
 * thread. Concurrent drains are serialized, and each delivery enters exactly
 * one returned batch. Release each batch with mln_delivery_batch_release().
 *
 * Returns:
 * - MLN_STATUS_OK when a batch holding at least one delivery is published in
 *   *out_batch and its view in *out_view.
 * - MLN_STATUS_NOT_READY when no delivery is queued. This is not an error:
 *   *out_batch and *out_view are left unchanged, and the caller drains again
 *   after the next wake.
 * - MLN_STATUS_INVALID_ARGUMENT when queue is an invalid handle, out_batch is
 *   null or does not point to the null handle, or out_view is null or
 *   out_view->size is too small.
 * - MLN_STATUS_INVALID_STATE when queue has been released.
 * - MLN_STATUS_NATIVE_ERROR when the batch could not be allocated. The
 *   deliveries stay queued in order, and the next push wakes the queue even
 *   though it is not empty. A host that cannot wait for that push drains
 *   again on its own schedule.
 */
MLN_API mln_status mln_delivery_queue_drain(
  mln_delivery_queue queue,
  mln_delivery_batch* out_batch MLN_BINDING("direction=out"),
  mln_delivery_batch_view* out_view MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Transfers the owned handles of one delivery to the caller.
 *
 * A resource or transform request's handle becomes the caller's to release. A
 * delivery that owns nothing is unchanged. The delivery's view stays readable.
 *
 * Returns:
 * - MLN_STATUS_OK when the delivery's handles now belong to the caller, or the
 *   delivery owns none.
 * - MLN_STATUS_INVALID_ARGUMENT when batch is an invalid handle, or index is
 *   not less than the batch's delivery count.
 * - MLN_STATUS_INVALID_STATE when batch has been released, or the delivery was
 *   already taken.
 */
MLN_API mln_status mln_delivery_batch_take(
  mln_delivery_batch batch, size_t index, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases an owned delivery batch. A null handle is a no-op.
 *
 * Request handles that the host did not take are released: a provider
 * request fails, and a transform request keeps its URL.
 */
MLN_API void mln_delivery_batch_release(mln_delivery_batch batch) MLN_NOEXCEPT;

// NOLINTEND(modernize-use-using,modernize-use-trailing-return-type)

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_DELIVERY_H
