// A transition callback already running when exit starts returns after the
// library's exit gate closes. Its release is a separate host callback.

#include "probe.h"

#ifndef _WIN32

static probe_latch callback_entered;
static probe_latch exit_started;
static probe_latch callback_thread_ended;
static bool release_ran_after_exit;

static void signal_latch(probe_latch* latch) {
  const mln_completion_result result = {.status = MLN_STATUS_OK};
  probe_latch_deliver(latch, &result);
}

static pthread_key_t thread_exit_key;

static void record_thread_exit(void* context) {
  (void)context;
  signal_latch(&callback_thread_ended);
}

static void transition_ended(
  void* context, const mln_camera_transition_end* end
) {
  (void)context;
  (void)end;
  pthread_setspecific(thread_exit_key, &callback_thread_ended);
  signal_latch(&callback_entered);
  probe_require(probe_latch_wait(&exit_started), "exit gate");
}

static void release_transition(void* context) {
  (void)context;
  release_ran_after_exit = true;
}

static void after_library_exit_gate(void) {
  signal_latch(&exit_started);
  probe_require(probe_latch_wait(&callback_thread_ended), "worker exit");
  if (release_ran_after_exit) {
    fputs("transition release callback started after the exit gate\n", stderr);
    _Exit(23);
  }
}

int main(void) {
  probe_latch_init(&callback_entered);
  probe_latch_init(&exit_started);
  probe_latch_init(&callback_thread_ended);
  // Registered first, so this runs after the library's exit handler.
  if (atexit(after_library_exit_gate) != 0) return 2;
  if (pthread_key_create(&thread_exit_key, record_thread_exit) != 0) return 2;
  const mln_runtime_options options = mln_runtime_options_default();
  mln_runtime runtime = MLN_HANDLE_NULL;
  probe_require(mln_runtime_create(&options, &runtime, NULL), "create runtime");
  probe_latch creation;
  const mln_completion created = probe_latch_completion(&creation);
  mln_map_options map_options = mln_map_options_default();
  map_options.initial_extent =
    (mln_logical_extent){.width = 256, .height = 256, .scale_factor = 1.0};
  mln_map map = MLN_HANDLE_NULL;
  probe_require(
    mln_runtime_create_map(runtime, &map_options, &map, &created, NULL),
    "create map"
  );
  probe_require(probe_latch_wait(&creation), "map completion");
  mln_camera_update camera = mln_camera_update_default();
  camera.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  camera.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.camera.zoom = 3;
  camera.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  camera.animation.duration_ms = 0;
  camera.animation.end_handler = (mln_camera_transition_handler){
    .callback = transition_ended,
    .release_user_data = release_transition,
  };
  probe_require(
    mln_map_update_camera(map, &camera, &probe_discarding_completion, NULL),
    "camera command"
  );
  probe_require(probe_latch_wait(&callback_entered), "transition callback");
  probe_require(
    mln_map_release(map, &probe_discarding_completion, NULL), "release map"
  );
  probe_require(
    mln_runtime_release(runtime, &probe_discarding_completion, NULL),
    "release runtime"
  );
  return 0;
}

#else

// Windows closes its exit gate after the host's exit handlers finish.
int main(void) { return 0; }

#endif
