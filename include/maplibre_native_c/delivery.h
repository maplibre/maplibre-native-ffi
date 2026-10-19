/**
 * @file maplibre_native_c/delivery.h
 * Public C API declarations for queue delivery of completions, log records,
 * and resource provider requests.
 *
 * A host that cannot run code on a native thread, such as a Dart isolate or a
 * JavaScript engine, sets a delivery queue on a registration in place of its
 * callback. Native code then answers any synchronous decision itself, copies
 * the payload into an item that the queue owns, and wakes the queue when it
 * goes from empty to nonempty. The host drains owned batches on its own
 * thread, takes the handles it keeps, and releases each batch.
 *
 * Three registrations accept a queue: mln_completion, mln_log_handler, and
 * mln_resource_provider. Runtime events keep their own queue and wake; see
 * mln_runtime_drain_events().
 *
 * A queue is FIFO in push order. It is not ordered against any runtime's event
 * queue. A map command still queues its runtime events before it pushes its
 * completion, so a host that wants a command's events after its completion
 * drains the runtime's events when it handles the completion.
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
#include "runtime.h"     // IWYU pragma: export
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
   * Last delivery of a log handler or resource provider registration; replaces
   * release_user_data in queue mode. No payload.
   */
  MLN_DELIVERY_TYPE_RETIRED = 4,
} mln_delivery_type;

/** Copied mln_log_callback arguments; fields mirror the callback's parameters.
 */
typedef struct mln_log_delivery {
  /** One of mln_log_severity. */
  uint32_t severity MLN_BINDING("enum=mln_log_severity");
  /** One of mln_log_event. */
  uint32_t event MLN_BINDING("enum=mln_log_event");
  int64_t code;
  /** Null-terminated UTF-8 message, owned by the batch. */
  const char* message MLN_BINDING("lifetime=owner");
} mln_log_delivery;

/** Copied mln_resource_provider_callback arguments. */
typedef struct mln_resource_request_delivery {
  /** The copied request, owned by the batch. */
  const mln_resource_request* request MLN_BINDING("lifetime=owner");
  /**
   * Owned by the batch until mln_delivery_batch_take(); releasing the batch
   * without taking it releases the handle, which fails the request.
   */
  mln_resource_request_handle handle;
} mln_resource_request_delivery;

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
} mln_delivery_payload;

/**
 * One drained delivery.
 *
 * Step through deliveries by mln_delivery_batch_view.delivery_size; the
 * payload stays last so a later variant only widens the stride.
 */
typedef struct mln_delivery {
  /** One of mln_delivery_type. */
  uint32_t type MLN_BINDING("enum=mln_delivery_type");
  uint32_t reserved MLN_BINDING("kind=reserved");
  /** user_data of the registration or completion, returned unchanged. */
  void* user_data MLN_BINDING("kind=context");
  /** Typed payload selected by type. */
  mln_delivery_payload payload
    MLN_BINDING("tag=type;empty_variant=MLN_DELIVERY_TYPE_RETIRED");
} mln_delivery;

/**
 * A borrowed view of one owned delivery batch.
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
 * It discards pending items: an untaken owned completion value is disposed,
 * and an untaken request handle is released, which fails its request.
 *
 * Registrations that still name the queue keep working, but the queue refuses
 * their later pushes: a completion is discarded and its owned value disposed,
 * a log record goes to the platform logger, and a provider request passes
 * through to native loading.
 *
 * No wake call starts after release. The wake's release_user_data runs once
 * in-flight wake calls return, on whichever thread finishes last. The host may
 * release the queue from inside its own wake and from a finalizer. Batches
 * already drained stay readable.
 */
MLN_API void mln_delivery_queue_release(mln_delivery_queue queue) MLN_NOEXCEPT;

/**
 * Moves every queued delivery into a new owned batch.
 *
 * Deliveries that arrive later enter the next batch, and the wake fires again
 * when the first of them arrives. This function may be called from any
 * thread. Concurrent drains are serialized and each delivery enters exactly
 * one returned batch. Release each batch with mln_delivery_batch_release().
 *
 * Returns:
 * - MLN_STATUS_OK when a batch holding at least one delivery is published in
 *   *out_batch.
 * - MLN_STATUS_NOT_READY when no delivery is queued. This is not an error:
 *   *out_batch is left unchanged, and the caller drains again after the next
 *   wake.
 * - MLN_STATUS_INVALID_ARGUMENT when queue is an invalid handle, or out_batch
 *   is null or does not point to the null handle.
 * - MLN_STATUS_INVALID_STATE when queue has been released.
 * - MLN_STATUS_NATIVE_ERROR when the batch could not be allocated. The
 *   deliveries stay queued.
 */
MLN_BINDING("execution=event_batch")
MLN_API mln_status mln_delivery_queue_drain(
  mln_delivery_queue queue,
  mln_delivery_batch* out_batch MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Borrows the delivery view stored by an owned delivery batch.
 *
 * Returns:
 * - MLN_STATUS_OK when out_view receives the borrowed view.
 * - MLN_STATUS_INVALID_ARGUMENT when batch is an invalid handle, or out_view is
 *   null or out_view->size is too small.
 * - MLN_STATUS_INVALID_STATE when batch has been released.
 */
MLN_API mln_status mln_delivery_batch_get(
  mln_delivery_batch batch,
  mln_delivery_batch_view* out_view MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Transfers the owned handles of one delivery to the caller.
 *
 * An owned completion value, such as the mln_map from
 * mln_runtime_create_map(), or a resource request's handle becomes the
 * caller's to release. A delivery that owns nothing is unchanged. The
 * delivery's view stays readable.
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
 * Owned values that the host did not take are disposed: a map without
 * blocking, as mln_map_dispose() does, and a request handle through
 * mln_resource_request_release(), which fails its request.
 */
MLN_API void mln_delivery_batch_release(mln_delivery_batch batch) MLN_NOEXCEPT;

// NOLINTEND(modernize-use-using,modernize-use-trailing-return-type)

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_DELIVERY_H
