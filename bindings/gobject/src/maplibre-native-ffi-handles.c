// Generated from the C headers by tools/bindgen. Do not edit.
#include <maplibre_native_c.h>

#include "maplibre-native-ffi-handles.h"

#include "maplibre-native-ffi-private.h"
#include "maplibre-native-ffi-values-private.h"

struct _MaplibreNativeFfiRuntime {
  GObject parent_instance;
  mln_runtime handle;
};

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiRuntime, maplibre_native_ffi_runtime, G_TYPE_OBJECT
)

static void maplibre_native_ffi_runtime_object_dispose(GObject* object) {
  maplibre_native_ffi_runtime_dispose(MAPLIBRE_NATIVE_FFI_RUNTIME(object));
  G_OBJECT_CLASS(maplibre_native_ffi_runtime_parent_class)->dispose(object);
}

static void maplibre_native_ffi_runtime_class_init(
  MaplibreNativeFfiRuntimeClass* klass
) {
  G_OBJECT_CLASS(klass)->dispose = maplibre_native_ffi_runtime_object_dispose;
}

static void maplibre_native_ffi_runtime_init(MaplibreNativeFfiRuntime* self) {
  self->handle = MLN_HANDLE_NULL;
}

MaplibreNativeFfiRuntime* maplibre_native_ffi_runtime_create(
  const MaplibreNativeFfiRuntimeOptions* options, GError** error
) {
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_runtime_options native_options_value;
  const mln_runtime_options* native_options = NULL;
  if (options != NULL) {
    maplibre_native_ffi_runtime_options_to_native(
      options, &native_options_value
    );
    native_options = &native_options_value;
  }
  mln_runtime handle = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic = {.size = sizeof diagnostic};
  mln_status status = mln_runtime_create(native_options, &handle, &diagnostic);
  if (!maplibre_native_ffi_check(status, &diagnostic, error)) return NULL;
  MaplibreNativeFfiRuntime* self =
    g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME, NULL);
  self->handle = handle;
  return self;
}

void maplibre_native_ffi_runtime_dispose(MaplibreNativeFfiRuntime* self) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  mln_runtime handle = self->handle;
  self->handle = MLN_HANDLE_NULL;
  if (handle != MLN_HANDLE_NULL) {
    (void)mln_runtime_dispose(handle, NULL);
  }
}

void maplibre_native_ffi_runtime_release_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  GTask* task = g_task_new(self, NULL, callback, user_data);
  g_task_set_source_tag(task, maplibre_native_ffi_runtime_release_async);
  mln_completion completion;
  GError* error = NULL;
  if (!maplibre_native_ffi_completion_init(&completion, task, &error)) {
    g_task_return_error(task, error);
    g_object_unref(task);
    return;
  }
  mln_diagnostic diagnostic = {.size = sizeof diagnostic};
  mln_status status =
    mln_runtime_release(self->handle, &completion, &diagnostic);
  if (status == MLN_STATUS_OK) {
    self->handle = MLN_HANDLE_NULL;
  } else {
    maplibre_native_ffi_completion_reject(&completion);
    g_task_return_error(
      task, maplibre_native_ffi_error_new(status, &diagnostic)
    );
  }
  g_object_unref(task);
}

gboolean maplibre_native_ffi_runtime_release_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}
