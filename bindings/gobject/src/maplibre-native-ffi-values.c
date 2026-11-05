// Generated from the C headers by tools/bindgen. Do not edit.
#include <string.h>

#include "maplibre-native-ffi-values.h"

#include "maplibre-native-ffi-values-private.h"

struct _MaplibreNativeFfiRuntimeOptions {
  char* asset_path;
  char* cache_path;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeOptions, maplibre_native_ffi_runtime_options,
  maplibre_native_ffi_runtime_options_copy,
  maplibre_native_ffi_runtime_options_free
)

MaplibreNativeFfiRuntimeOptions* maplibre_native_ffi_runtime_options_new(void) {
  MaplibreNativeFfiRuntimeOptions* self =
    g_new0(MaplibreNativeFfiRuntimeOptions, 1);
  self->asset_path = g_strdup("");
  self->cache_path = g_strdup("");
  return self;
}

MaplibreNativeFfiRuntimeOptions* maplibre_native_ffi_runtime_options_copy(
  const MaplibreNativeFfiRuntimeOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiRuntimeOptions* copy =
    g_new0(MaplibreNativeFfiRuntimeOptions, 1);
  copy->asset_path = g_strdup(self->asset_path);
  copy->cache_path = g_strdup(self->cache_path);
  return copy;
}

void maplibre_native_ffi_runtime_options_free(
  MaplibreNativeFfiRuntimeOptions* self
) {
  if (self == NULL) return;
  g_free(self->asset_path);
  g_free(self->cache_path);
  g_free(self);
}

const char* maplibre_native_ffi_runtime_options_get_asset_path(
  const MaplibreNativeFfiRuntimeOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->asset_path;
}

void maplibre_native_ffi_runtime_options_set_asset_path(
  MaplibreNativeFfiRuntimeOptions* self, const char* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  char* previous = self->asset_path;
  self->asset_path = g_strdup(value);
  g_free(previous);
}

const char* maplibre_native_ffi_runtime_options_get_cache_path(
  const MaplibreNativeFfiRuntimeOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->cache_path;
}

void maplibre_native_ffi_runtime_options_set_cache_path(
  MaplibreNativeFfiRuntimeOptions* self, const char* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  char* previous = self->cache_path;
  self->cache_path = g_strdup(value);
  g_free(previous);
}

void maplibre_native_ffi_runtime_options_to_native(
  const MaplibreNativeFfiRuntimeOptions* self, mln_runtime_options* out
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->asset_path.data = self->asset_path;
  out->asset_path.size = strlen(self->asset_path);
  out->cache_path.data = self->cache_path;
  out->cache_path.size = strlen(self->cache_path);
}
