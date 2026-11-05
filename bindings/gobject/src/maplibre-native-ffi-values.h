// Generated from the C headers by tools/bindgen. Do not edit.
#ifndef MAPLIBRE_NATIVE_FFI_VALUES_H
#define MAPLIBRE_NATIVE_FFI_VALUES_H

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * MaplibreNativeFfiRuntimeOptions:
 *
 * Options for `maplibre_native_ffi_runtime_create()`.
 *
 * See `mln_runtime_options` in the
 * [C API
 * reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef struct _MaplibreNativeFfiRuntimeOptions MaplibreNativeFfiRuntimeOptions;

GType maplibre_native_ffi_runtime_options_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_OPTIONS \
  (maplibre_native_ffi_runtime_options_get_type())

/**
 * maplibre_native_ffi_runtime_options_new: (constructor)
 *
 * Creates a `MaplibreNativeFfiRuntimeOptions` that selects every default.
 *
 * Returns: (transfer full): the new value
 */
MaplibreNativeFfiRuntimeOptions* maplibre_native_ffi_runtime_options_new(void);

/**
 * maplibre_native_ffi_runtime_options_copy:
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiRuntimeOptions* maplibre_native_ffi_runtime_options_copy(
  const MaplibreNativeFfiRuntimeOptions* self
);

/**
 * maplibre_native_ffi_runtime_options_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_runtime_options_free(
  MaplibreNativeFfiRuntimeOptions* self
);

/**
 * maplibre_native_ffi_runtime_options_get_asset_path:
 *
 * The directory that `asset://` URLs load from. Empty uses the working
 * directory, or the app's assets on Android.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_runtime_options_get_asset_path(
  const MaplibreNativeFfiRuntimeOptions* self
);

/**
 * maplibre_native_ffi_runtime_options_set_asset_path:
 * @value: the UTF-8 text
 *
 * The directory that `asset://` URLs load from. Empty uses the working
 * directory, or the app's assets on Android.
 */
void maplibre_native_ffi_runtime_options_set_asset_path(
  MaplibreNativeFfiRuntimeOptions* self, const char* value
);

/**
 * maplibre_native_ffi_runtime_options_get_cache_path:
 *
 * The cache database file, which holds the ambient cache and offline regions.
 * Empty keeps them in memory.
 *
 * Returns: (transfer none): the text
 */
const char* maplibre_native_ffi_runtime_options_get_cache_path(
  const MaplibreNativeFfiRuntimeOptions* self
);

/**
 * maplibre_native_ffi_runtime_options_set_cache_path:
 * @value: the UTF-8 text
 *
 * The cache database file, which holds the ambient cache and offline regions.
 * Empty keeps them in memory.
 */
void maplibre_native_ffi_runtime_options_set_cache_path(
  MaplibreNativeFfiRuntimeOptions* self, const char* value
);

G_END_DECLS

#endif
