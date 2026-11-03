// Snippets for the docs site's Render to a window page. They compile
// with the native tests, which never run them.

#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static atomic_bool finished;

static void on_done(
  void* user_data, const mln_completion_result* result
) {
  (void)user_data;
  (void)result;
  atomic_store(&finished, true);
}

static void wait_for_done(void) {
  while (!atomic_exchange(&finished, false)) {
  }
}

static const mln_completion done = {
  .size = sizeof(mln_completion), .callback = on_done
};

// Stand-ins for the host's event loop, display timer, and graphics
// thread.
static void run_on_event_loop(void (*task)(void)) { (void)task; }

static void run_at_next_refresh(void (*task)(void)) { (void)task; }

static void run_on_graphics_thread(void (*task)(void)) { (void)task; }

static void replace_session(void) {}

static void destroy_surface(void) {}

// #region attach
static mln_render_session session = MLN_HANDLE_NULL;
// #endregion attach

// #region frame-loop
// Set while the window has no surface, and when it closes.
static bool paused = false;

static void demand_frame(void) {
  const mln_frame_demand demand = {
    .size = sizeof(mln_frame_demand),
    .flags = MLN_FRAME_DEMAND_WAIT_FOR_UPDATE,
  };
  mln_render_session_demand_frame(session, &demand, NULL);
}

static void drain_frame_results(void) {
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  mln_render_frame_batch_view view = {
    .size = sizeof(mln_render_frame_batch_view)
  };
  if (
    mln_render_session_drain_frame_results(
      session, &batch, &view, NULL
    ) != MLN_STATUS_OK ||
    batch == MLN_HANDLE_NULL
  ) {
    return;  // No results yet.
  }

  bool failed = false;
  for (size_t i = 0; i < view.result_count; i++) {
    const mln_render_frame_result* result = MLN_STRIDED_AT(
      mln_render_frame_result, view.results, view.result_size, i
    );
    if (result->disposition == MLN_RENDER_RESULT_FAILED)
      failed = true;
  }
  mln_render_frame_batch_release(batch);

  if (failed) {
    replace_session();  // Dispose the session and attach a new one.
  } else if (!paused) {
    run_at_next_refresh(demand_frame);
  }
}
// #endregion frame-loop

// #region frame-wake
// Runs on a library thread.
static void on_frame_wake(void* user_data) {
  (void)user_data;
  run_on_event_loop(drain_frame_results);
}
// #endregion frame-wake

// #region resize
static void on_window_resized(uint32_t width, uint32_t height) {
  // The window's new size in logical pixels.
  const mln_logical_size size = {.width = width, .height = height};
  mln_render_session_resize(session, size, NULL, NULL);
}
// #endregion resize

// #region service
// Your graphics context is current on this thread.
static void service_session(void) {
  mln_render_session_service_driver_work(session, NULL);
}

// Runs on a library thread.
static void on_driver_work_wake(void* user_data) {
  (void)user_data;
  run_on_graphics_thread(service_session);
}
// #endregion service

void mln_doc_window_surface_lost(void);

void mln_doc_window_surface_lost(void) {
  // #region lost-surface
  paused = true;
  mln_render_session_release_target(session, &done, NULL);
  // #endregion lost-surface
  wait_for_done();
}

void mln_doc_window_surface_restored(
  mln_vulkan_non_dispatchable_handle new_surface
);

void mln_doc_window_surface_restored(
  mln_vulkan_non_dispatchable_handle new_surface
) {
  // #region lost-surface
  const mln_vulkan_surface_replacement replacement = {
    .size = sizeof(mln_vulkan_surface_replacement),
    .logical_size = {.width = 800, .height = 600},
    .surface = new_surface,
  };
  mln_render_session_set_vulkan_surface_target(
    session, &replacement, &done, NULL
  );
  // #endregion lost-surface
  wait_for_done();
  // #region lost-surface
  paused = false;
  demand_frame();
  // #endregion lost-surface
}

void mln_doc_window(
  mln_map map, mln_vulkan_context_descriptor vulkan,
  mln_vulkan_non_dispatchable_handle window_surface
);

void mln_doc_window(
  mln_map map, mln_vulkan_context_descriptor vulkan,
  mln_vulkan_non_dispatchable_handle window_surface
) {
  // #region attach
  const mln_vulkan_surface_descriptor surface = {
    .size = sizeof(mln_vulkan_surface_descriptor),
    // The window's size in logical pixels. The map's scale factor
    // turns it into pixels.
    .logical_size = {.width = 800, .height = 600},
    // Your VkInstance, VkPhysicalDevice,
    // VkDevice, and graphics VkQueue.
    .context = vulkan,
    // Your VkSurfaceKHR.
    .surface = window_surface,
  };
  const mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options),
    .driver = MLN_RENDER_DRIVER_CORE_WORKER,
    .frame_wake = {.callback = on_frame_wake},
  };

  mln_map_attach_vulkan_surface(
    map, &surface, &options, &session, &done, NULL
  );
  // #endregion attach
  wait_for_done();

  demand_frame();
  on_window_resized(1024, 768);

  // #region service
  const mln_render_session_attach_options graphics_thread_options = {
    .size = sizeof(mln_render_session_attach_options),
    .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
    .frame_wake = {.callback = on_frame_wake},
    .driver_work_wake = {.callback = on_driver_work_wake},
  };
  // #endregion service
  (void)graphics_thread_options;

  paused = true;
  mln_render_session_dispose(session, &done, NULL);
  wait_for_done();

  destroy_surface();  // For example, with vkDestroySurfaceKHR.
}
