// A core-worker session that the browser main thread attaches, abandons, and
// destroys in one turn, while the pre-spawned pthread pool is empty. The
// session's worker then starts only after the main thread yields, so destroy
// must not wait for it.
//
// The suite runs main() on a pthread, so the browser main thread is free to
// run a call proxied to it.

#include <emscripten.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <maplibre_native_c.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "support/test_support.h"

#define CANVAS_ID "mln-main-thread-destroy"

// The transfer path looks the selector up in GL.offscreenCanvases on the
// thread that creates the worker, which here is the main thread. The size block
// is static because the worker can start after the case ends.
static int32_t canvas_shared[3];
EM_JS(void, register_main_thread_canvas, (void* canvas_shared_ptr), {
  const canvas = new OffscreenCanvas(64, 64);
  HEAP32[canvas_shared_ptr >> 2] = 64;
  HEAP32[canvas_shared_ptr + 4 >> 2] = 64;
  HEAPU32[canvas_shared_ptr + 8 >> 2] = 0;
  Module["GL"].offscreenCanvases["#" + "mln-main-thread-destroy"] = {
    canvas : canvas,
    offscreenCanvas : canvas,
    canvasSharedPtr : canvas_shared_ptr,
    id : "mln-main-thread-destroy",
  };
});

EM_JS(int, unused_pool_workers, (void), {
  return PThread.unusedWorkers.length;
});

typedef struct pool_hold {
  pthread_mutex_t mutex;
  pthread_cond_t condition;
  bool released;
  pthread_t* threads;
  int thread_count;
} pool_hold;

static void* hold_pool_worker(void* context) {
  pool_hold* hold = context;
  pthread_mutex_lock(&hold->mutex);
  while (!hold->released) {
    pthread_cond_wait(&hold->condition, &hold->mutex);
  }
  pthread_mutex_unlock(&hold->mutex);
  return NULL;
}

typedef struct main_thread_session {
  mln_map map;
  pool_hold* hold;
  mln_completion attach_completion;
  mln_status attach_status;
  mln_status abandon_status;
  mln_status destroy_status;
} main_thread_session;

static void attach_and_destroy_here(void* context) {
  main_thread_session* run = context;

  // Each pthread_create() here takes a pre-spawned Worker, so the session's
  // worker below needs a Worker that starts only after this call returns.
  run->hold->thread_count = unused_pool_workers();
  run->hold->threads =
    calloc((size_t)run->hold->thread_count + 1, sizeof(pthread_t));
  for (int index = 0; index < run->hold->thread_count; ++index) {
    if (
      pthread_create(
        &run->hold->threads[index], NULL, hold_pool_worker, run->hold
      ) != 0
    ) {
      run->hold->thread_count = index;
      break;
    }
  }

  register_main_thread_canvas(canvas_shared);
  const char* selector = "#" CANVAS_ID;
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
  descriptor.context.platform = MLN_OPENGL_CONTEXT_PLATFORM_WEBGL;
  descriptor.context.ownership = MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED;
  descriptor.context.data.webgl = (mln_webgl_context_descriptor){
    .kind = MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS,
    .canvas_selector = mln_test_buffer_view(selector, strlen(selector)),
  };
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  mln_render_session session = MLN_HANDLE_NULL;
  run->attach_status = mln_opengl_surface_attach(
    run->map, &descriptor, &options, &session, &run->attach_completion, NULL
  );
  if (run->attach_status != MLN_STATUS_OK) return;

  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  run->abandon_status = mln_render_session_abandon(session, &abandoned, NULL);
  run->destroy_status = mln_render_session_destroy(session, NULL);
}

static void destroy_on_main_thread_leaves_unstarted_worker(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  pool_hold hold = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .condition = PTHREAD_COND_INITIALIZER,
  };
  mln_test_completion attach = mln_test_completion_default(0);
  main_thread_session run = {
    .map = map,
    .hold = &hold,
    .attach_completion = attach.descriptor,
    .attach_status = MLN_STATUS_NATIVE_ERROR,
    .abandon_status = MLN_STATUS_NATIVE_ERROR,
    .destroy_status = MLN_STATUS_NATIVE_ERROR,
  };

  TEST_ASSERT_EQUAL_INT(
    1, emscripten_proxy_sync(
         emscripten_proxy_get_system_queue(),
         emscripten_main_runtime_thread_id(), attach_and_destroy_here, &run
       )
  );

  pthread_mutex_lock(&hold.mutex);
  hold.released = true;
  pthread_cond_broadcast(&hold.condition);
  pthread_mutex_unlock(&hold.mutex);
  for (int index = 0; index < hold.thread_count; ++index) {
    pthread_join(hold.threads[index], NULL);
  }
  free(hold.threads);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, run.attach_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, run.abandon_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, run.destroy_status);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_TARGET_LOST, mln_test_completion_settle(&attach)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP { RUN_TEST(destroy_on_main_thread_leaves_unstarted_worker); }
