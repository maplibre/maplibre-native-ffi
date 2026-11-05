// Generated from the C headers by tools/bindgen. Do not edit.
#ifndef MAPLIBRE_NATIVE_FFI_HANDLES_H
#define MAPLIBRE_NATIVE_FFI_HANDLES_H

#include <gio/gio.h>

#include "maplibre-native-ffi-values.h"

G_BEGIN_DECLS

/**
 * MaplibreNativeFfiRuntime:
 *
 * A runtime owns maps and the network and cache settings that they share.
 *
 * See `mln_runtime` in the
 * [C API
 * reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME \
  (maplibre_native_ffi_runtime_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiRuntime, maplibre_native_ffi_runtime, MAPLIBRE_NATIVE_FFI,
  RUNTIME, GObject
)

/**
 * maplibre_native_ffi_runtime_create: (constructor)
 * @options: (nullable): the options, or %NULL for every default
 *
 * Creates a runtime.
 *
 * A runtime holds maps and the resource loading, cache, and offline regions
 * that they share. In a browser, create runtimes on a Web Worker, not on the
 * main thread.
 *
 * See `mln_runtime_create` in the
 * [C API
 * reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Returns: (transfer full): the new `MaplibreNativeFfiRuntime`, or %NULL with
 * `error` set
 */
MaplibreNativeFfiRuntime* maplibre_native_ffi_runtime_create(
  const MaplibreNativeFfiRuntimeOptions* options, GError** error
);

/**
 * maplibre_native_ffi_runtime_dispose:
 *
 * Releases a runtime in any state, without waiting.
 *
 * The runtime finishes its teardown after its maps are released.
 *
 * The object calls this when it is disposed, unless its handle was released.
 * Later calls do nothing.
 *
 * See `mln_runtime_dispose` in the
 * [C API
 * reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
void maplibre_native_ffi_runtime_dispose(MaplibreNativeFfiRuntime* self);

/**
 * maplibre_native_ffi_runtime_release_async:
 * @callback: (scope async) (nullable): the callback to call when the release
 * finishes
 * @user_data: the data to pass to `callback`
 *
 * Releases a runtime.
 *
 * Release the runtime's maps first.
 *
 * See `mln_runtime_release` in the
 * [C API
 * reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
void maplibre_native_ffi_runtime_release_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_release_finish:
 * @result: the result that the callback received
 *
 * Finishes a release that `maplibre_native_ffi_runtime_release_async` started.
 *
 * Returns: %TRUE when the handle retired, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_runtime_release_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

G_END_DECLS

#endif
