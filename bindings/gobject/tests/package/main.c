// Creates a runtime and releases it, then releases it again and expects the
// library's error for a released handle, through the installed header and
// pkg-config file.

#include <maplibre-native-ffi.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  GMainLoop* loop;
  int status;
} State;

static void fail(State* state, const char* message, GError* error) {
  fprintf(
    stderr, "%s%s%s\n", message, error ? ": " : "", error ? error->message : ""
  );
  state->status = EXIT_FAILURE;
  g_main_loop_quit(state->loop);
}

static void released_again(
  GObject* source, GAsyncResult* result, gpointer user_data
) {
  State* state = user_data;
  GError* error = NULL;
  if (
    maplibre_native_ffi_runtime_release_finish(
      MAPLIBRE_NATIVE_FFI_RUNTIME(source), result, &error
    )
  ) {
    fail(state, "a second release succeeded", NULL);
    return;
  }
  // The domain's codes are the native statuses, which are negative.
  if (
    !g_error_matches(
      error, MAPLIBRE_NATIVE_FFI_ERROR,
      MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT
    ) ||
    error->code >= 0
  ) {
    fail(state, "a second release failed with an unexpected error", error);
    g_error_free(error);
    return;
  }
  g_error_free(error);
  g_main_loop_quit(state->loop);
}

static void released(
  GObject* source, GAsyncResult* result, gpointer user_data
) {
  State* state = user_data;
  MaplibreNativeFfiRuntime* runtime = MAPLIBRE_NATIVE_FFI_RUNTIME(source);
  GError* error = NULL;
  if (!maplibre_native_ffi_runtime_release_finish(runtime, result, &error)) {
    fail(state, "release failed", error);
    g_error_free(error);
    return;
  }
  maplibre_native_ffi_runtime_release_async(runtime, released_again, state);
}

int main(void) {
  State state = {.loop = g_main_loop_new(NULL, FALSE), .status = EXIT_SUCCESS};
  MaplibreNativeFfiRuntimeOptions* options =
    maplibre_native_ffi_runtime_options_new();
  maplibre_native_ffi_runtime_options_set_asset_path(options, ".");
  GError* error = NULL;
  MaplibreNativeFfiRuntime* runtime =
    maplibre_native_ffi_runtime_create(options, &error);
  maplibre_native_ffi_runtime_options_free(options);
  if (runtime == NULL) {
    fprintf(stderr, "create failed: %s\n", error->message);
    return EXIT_FAILURE;
  }
  maplibre_native_ffi_runtime_release_async(runtime, released, &state);
  g_main_loop_run(state.loop);
  g_object_unref(runtime);

  // A runtime that is never released is disposed with its object.
  runtime = maplibre_native_ffi_runtime_create(NULL, &error);
  if (runtime == NULL) {
    fprintf(stderr, "create failed: %s\n", error->message);
    return EXIT_FAILURE;
  }
  g_object_unref(runtime);
  g_main_loop_unref(state.loop);
  return state.status;
}
