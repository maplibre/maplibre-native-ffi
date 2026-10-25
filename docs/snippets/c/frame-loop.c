// One turn of a display-paced host loop: service caller-driver work, drain
// frame results, and keep one frame demand waiting for the next update.

#include <maplibre_native_c.h>
#include <stdatomic.h>

typedef struct frame_receiver {
  atomic_bool driver_ready;
  atomic_bool frames_ready;
  void (*schedule)(void* user_data);
  void* schedule_user_data;
  uint64_t next_frame_token;
} frame_receiver;

static void mark_ready(frame_receiver* receiver, atomic_bool* flag) {
  atomic_store_explicit(flag, true, memory_order_release);
  receiver->schedule(receiver->schedule_user_data);
}

void wake_driver(void* user_data) {
  frame_receiver* receiver = user_data;
  mark_ready(receiver, &receiver->driver_ready);
}

void wake_frames(void* user_data) {
  frame_receiver* receiver = user_data;
  mark_ready(receiver, &receiver->frames_ready);
}

static void drain_frame_results(mln_render_session session, bool* pending) {
  // #region results
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  mln_render_frame_batch_view view = {
    .size = sizeof(mln_render_frame_batch_view)
  };
  if (
    mln_render_session_drain_frame_results(session, &batch, &view, NULL) !=
    MLN_STATUS_OK
  )
    return;
  for (size_t index = 0; index < view.result_count; index++) {
    const char* bytes = (const char*)view.results + index * view.result_size;
    const mln_render_frame_result* result =
      (const mln_render_frame_result*)bytes;
    (void)result;
    *pending = true;
  }
  mln_render_frame_batch_release(batch);
  // #endregion results
}

void run_one_frame(
  frame_receiver* receiver, mln_render_session session, bool* pending
) {
  // #region service
  if (
    atomic_exchange_explicit(
      &receiver->driver_ready, false, memory_order_acq_rel
    )
  ) {
    (void)mln_render_session_service_driver_work(session, NULL);
  }
  // #endregion service
  if (
    atomic_exchange_explicit(
      &receiver->frames_ready, false, memory_order_acq_rel
    )
  ) {
    drain_frame_results(session, pending);
  }

  // #region render
  if (!*pending) return;
  *pending = false;
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_IF_NEEDED | MLN_FRAME_DEMAND_PRESENT |
                 MLN_FRAME_DEMAND_WAIT_FOR_UPDATE;
  demand.token = ++receiver->next_frame_token;
  if (
    mln_render_session_request_frame(session, &demand, NULL) != MLN_STATUS_OK
  ) {
    *pending = true;
  }
  // #endregion render
}
