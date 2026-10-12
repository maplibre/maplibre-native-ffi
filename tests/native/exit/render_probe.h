// The render-session exit program, shared by the backends that render a
// session-owned texture. Each backend's file defines the attach and calls
// probe_exit_after_abandoning() from main.
//
// A render session borrows the host's graphics device, and a graphics driver
// can tear itself down in exit handlers of its own, which no exit handler of
// the library is ordered before. So a host ends each session's graphics calls
// before it exits; abandon does that at once, even mid-frame. Everything else
// stays live through the exit, as in the other programs.

#ifndef MLN_NATIVE_EXIT_RENDER_PROBE_H
#define MLN_NATIVE_EXIT_RENDER_PROBE_H

#include "mln_test_graphics.h"
#include "probe.h"

// Starts attaching a session-owned texture on `map` for the graphics context,
// with the driver that options->driver names.
typedef mln_status (*probe_attach_fn)(
  mln_map map, const mln_test_graphics_context* context,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
);

static const mln_logical_extent probe_extent = {
  .width = 256,
  .height = 256,
  .scale_factor = 1.0,
};

// Counts the wakes that one receiver has been sent. Each signal has one
// waiting thread.
typedef struct probe_signal {
#ifdef _WIN32
  CRITICAL_SECTION lock;
  // Auto-reset, so a wake sent before the wait still ends it.
  HANDLE changed;
#else
  pthread_mutex_t lock;
  pthread_cond_t changed;
#endif
  uint64_t count;
} probe_signal;

static inline void probe_signal_init(probe_signal* signal) {
#ifdef _WIN32
  InitializeCriticalSection(&signal->lock);
  signal->changed = CreateEventW(NULL, FALSE, FALSE, NULL);
#else
  pthread_mutex_init(&signal->lock, NULL);
  pthread_cond_init(&signal->changed, NULL);
#endif
  signal->count = 0;
}

static inline void probe_signal_notify(void* user_data) {
  probe_signal* signal = user_data;
#ifdef _WIN32
  EnterCriticalSection(&signal->lock);
  signal->count += 1;
  SetEvent(signal->changed);
  LeaveCriticalSection(&signal->lock);
#else
  pthread_mutex_lock(&signal->lock);
  signal->count += 1;
  pthread_cond_broadcast(&signal->changed);
  pthread_mutex_unlock(&signal->lock);
#endif
}

// Waits for a wake after the count `seen`, and returns the new count.
static inline uint64_t probe_signal_wait(probe_signal* signal, uint64_t seen) {
#ifdef _WIN32
  EnterCriticalSection(&signal->lock);
  while (signal->count == seen) {
    LeaveCriticalSection(&signal->lock);
    WaitForSingleObject(signal->changed, INFINITE);
    EnterCriticalSection(&signal->lock);
  }
  const uint64_t count = signal->count;
  LeaveCriticalSection(&signal->lock);
#else
  pthread_mutex_lock(&signal->lock);
  while (signal->count == seen) {
    pthread_cond_wait(&signal->changed, &signal->lock);
  }
  const uint64_t count = signal->count;
  pthread_mutex_unlock(&signal->lock);
#endif
  return count;
}

static inline mln_wake probe_signal_wake(probe_signal* signal) {
  return (mln_wake){
    .callback = probe_signal_notify,
    .user_data = signal,
  };
}

// One render session, the receivers of its wakes, and for a caller-driven
// session the host graphics thread that drives it.
typedef struct probe_session {
  mln_render_session session;
  mln_test_graphics* graphics;
  uint32_t graphics_backend;
  probe_signal frames;
  probe_signal driver_work;
  // Written before a driver-work wake that the graphics thread reads after.
  bool stopping;
#ifdef _WIN32
  HANDLE thread;
#else
  pthread_t thread;
#endif
} probe_session;

static inline void probe_request_frame(mln_render_session session) {
  probe_require(
    mln_render_session_request_frame(
      session, &(mln_frame_demand){.size = sizeof(mln_frame_demand)}, NULL
    ),
    "requesting a frame"
  );
}

// Drains the session's frame results and reports whether one was rendered.
static inline bool probe_drain_rendered(mln_render_session session) {
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  const mln_status status =
    mln_render_session_drain_frame_results(session, &batch, NULL);
  if (status == MLN_STATUS_NOT_READY) return false;
  probe_require(status, "draining frame results");
  mln_render_frame_batch_view view = {.size = sizeof(view)};
  probe_require(
    mln_render_frame_batch_get(batch, &view, NULL), "reading frame results"
  );
  bool rendered = false;
  for (size_t index = 0; index < view.result_count; index += 1) {
    const mln_render_frame_result* result =
      (const mln_render_frame_result*)((const char*)view.results +
                                       (index * view.result_size));
    rendered = rendered || result->disposition == MLN_RENDER_RESULT_RENDERED;
  }
  mln_render_frame_batch_release(batch);
  return rendered;
}

// The host graphics thread of a caller-driven session: it services driver
// work on each wake until it is told to stop.
#ifdef _WIN32
static DWORD WINAPI probe_drive(void* user_data) {
#else
static inline void* probe_drive(void* user_data) {
#endif
  probe_session* probe = user_data;
  if (
    probe->graphics_backend == MLN_TEST_GRAPHICS_BACKEND_EGL &&
    !mln_test_graphics_make_current(probe->graphics)
  ) {
    (void)fprintf(
      stderr, "making the context current failed: %s\n",
      mln_test_graphics_last_error()
    );
    exit(1);
  }
  uint64_t seen = 0;
  while (true) {
    seen = probe_signal_wait(&probe->driver_work, seen);
    if (probe->stopping) break;
    size_t serviced = 0;
    probe_require(
      mln_render_session_service_driver_work(
        probe->session, 0, &serviced, NULL
      ),
      "servicing driver work"
    );
  }
  return 0;
}

static inline void probe_start_driving(probe_session* probe) {
#ifdef _WIN32
  probe->thread = CreateThread(NULL, 0, probe_drive, probe, 0, NULL);
  const bool started = probe->thread != NULL;
#else
  const bool started =
    pthread_create(&probe->thread, NULL, probe_drive, probe) == 0;
#endif
  if (!started) {
    (void)fprintf(stderr, "starting the graphics thread failed\n");
    exit(1);
  }
}

// Stops the graphics thread, as a host stops driving a session before it
// abandons the session.
static inline void probe_stop_driving(probe_session* probe) {
  probe->stopping = true;
  probe_signal_notify(&probe->driver_work);
#ifdef _WIN32
  WaitForSingleObject(probe->thread, INFINITE);
  CloseHandle(probe->thread);
#else
  pthread_join(probe->thread, NULL);
#endif
}

// Attaches a session that `driver` drives, and waits until it renders a frame
// of the probe's style.
static inline void probe_attach_rendering(
  probe_session* probe, mln_map map, uint32_t graphics_backend,
  probe_attach_fn attach, uint32_t driver
) {
  // Each session gets its own device, since a core worker and a host graphics
  // thread cannot share a queue.
  probe->graphics_backend = graphics_backend;
  probe->graphics = mln_test_graphics_create(graphics_backend);
  mln_test_graphics_context context = {0};
  if (
    probe->graphics == NULL ||
    !mln_test_graphics_get_context(probe->graphics, &context)
  ) {
    (void)fprintf(
      stderr, "no graphics context: %s\n", mln_test_graphics_last_error()
    );
    exit(1);
  }
  probe_signal_init(&probe->frames);
  probe_signal_init(&probe->driver_work);
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = driver;
  options.frame_wake = probe_signal_wake(&probe->frames);
  if (driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    options.driver_work_wake = probe_signal_wake(&probe->driver_work);
  }

  probe_latch latch;
  const mln_completion completion = probe_latch_completion(&latch);
  probe->session = MLN_HANDLE_NULL;
  probe_require(
    attach(map, &context, &options, &probe->session, &completion),
    "attaching the render session"
  );
  if (driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    probe_start_driving(probe);
  }
  probe_require(probe_latch_wait(&latch), "attaching the render session");

  // A frame can come before the style loads, so ask until one renders it.
  uint64_t seen = 0;
  do {
    probe_request_frame(probe->session);
    seen = probe_signal_wait(&probe->frames, seen);
  } while (!probe_drain_rendered(probe->session));
}

static inline void probe_abandon(mln_render_session session) {
  mln_render_abandon_result result = {.size = sizeof(result)};
  probe_require(
    mln_render_session_abandon(session, &result, NULL), "abandoning a session"
  );
}

// Renders on two maps of one runtime, one session on a core worker and one on
// a host graphics thread. With frames still queued for both, it stops the
// graphics thread, abandons both sessions, and returns the exit status at once,
// leaving the runtime, the maps, the session handles, the wakes, the resource
// provider, and the log callback live.
static inline int probe_exit_after_abandoning(
  uint32_t graphics_backend, probe_attach_fn attach
) {
  probe_start();
  const mln_runtime runtime = probe_create_runtime();
  static probe_session core_worker;
  static probe_session caller_driven;
  probe_attach_rendering(
    &core_worker, probe_create_map(runtime), graphics_backend, attach,
    MLN_RENDER_DRIVER_CORE_WORKER
  );
  probe_attach_rendering(
    &caller_driven, probe_create_map(runtime), graphics_backend, attach,
    MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
  );
  for (int frame = 0; frame < 64; frame += 1) {
    probe_request_frame(core_worker.session);
    probe_request_frame(caller_driven.session);
  }
  // The core worker is mid-frame here; abandon waits out its call.
  probe_abandon(core_worker.session);
  probe_stop_driving(&caller_driven);
  probe_abandon(caller_driven.session);
  return 0;
}

#endif
