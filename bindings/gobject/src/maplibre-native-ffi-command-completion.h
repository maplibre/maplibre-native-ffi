// The result of a map command, which every command's finish function returns.

#ifndef MAPLIBRE_NATIVE_FFI_COMMAND_COMPLETION_H
#define MAPLIBRE_NATIVE_FFI_COMMAND_COMPLETION_H

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * MaplibreNativeFfiCommandCompletion:
 * @generation: the first map snapshot generation that includes the change
 * @render_update_generation: the first render frame result map update
 *   generation that includes the change
 *
 * The result of a command that finished.
 */
typedef struct {
  guint64 generation;
  guint64 render_update_generation;
} MaplibreNativeFfiCommandCompletion;

GType maplibre_native_ffi_command_completion_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_COMMAND_COMPLETION \
  (maplibre_native_ffi_command_completion_get_type())

/**
 * maplibre_native_ffi_command_completion_copy:
 * @self: the value
 *
 * Copies the value.
 *
 * Returns: (transfer full): the copy
 */
MaplibreNativeFfiCommandCompletion* maplibre_native_ffi_command_completion_copy(
  const MaplibreNativeFfiCommandCompletion* self
);

/**
 * maplibre_native_ffi_command_completion_free:
 * @self: (transfer full) (nullable): the value
 *
 * Frees the value.
 */
void maplibre_native_ffi_command_completion_free(
  MaplibreNativeFfiCommandCompletion* self
);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(
  MaplibreNativeFfiCommandCompletion,
  maplibre_native_ffi_command_completion_free
)

G_END_DECLS

#endif
