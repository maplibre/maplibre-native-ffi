// Runtime creation on the browser main thread, which cannot block while the
// runtime's worker starts.
//
// The suite runs main() on a pthread, so the browser main thread is free to
// run a call proxied to it.

#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <maplibre_native_c.h>
#include <string.h>

#include "support/test_support.h"

typedef struct main_thread_creation {
  mln_status status;
  mln_runtime runtime;
  mln_diagnostic diagnostic;
} main_thread_creation;

static void create_runtime_here(void* context) {
  main_thread_creation* creation = context;
  const mln_runtime_options options = mln_runtime_options_default();
  creation->status =
    mln_runtime_create(&options, &creation->runtime, &creation->diagnostic);
}

static void runtime_creation_rejects_the_browser_main_thread(void) {
  main_thread_creation creation = {
    .status = MLN_STATUS_OK,
    .runtime = MLN_HANDLE_NULL,
    .diagnostic = {.size = sizeof(creation.diagnostic)},
  };
  TEST_ASSERT_EQUAL_INT(
    1, emscripten_proxy_sync(
         emscripten_proxy_get_system_queue(),
         emscripten_main_runtime_thread_id(), create_runtime_here, &creation
       )
  );

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_WRONG_THREAD, creation.status);
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, creation.runtime);
  TEST_ASSERT_NOT_NULL(strstr(creation.diagnostic.message, "main thread"));
}

MLN_TEST_GROUP { RUN_TEST(runtime_creation_rejects_the_browser_main_thread); }
