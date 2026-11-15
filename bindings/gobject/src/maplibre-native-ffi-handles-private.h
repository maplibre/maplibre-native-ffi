// Generated from the C headers by tools/bindgen. Do not edit.
#ifndef MAPLIBRE_NATIVE_FFI_HANDLES_PRIVATE_H
#define MAPLIBRE_NATIVE_FFI_HANDLES_PRIVATE_H

#include <maplibre_native_c.h>

#include "maplibre-native-ffi-handles.h"
#include "maplibre-native-ffi-private.h"

G_BEGIN_DECLS

G_GNUC_INTERNAL MaplibreNativeFfiAcquiredFrame*
maplibre_native_ffi_acquired_frame_wrap(
  mln_acquired_frame handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
);
G_GNUC_INTERNAL MaplibreNativeFfiFramePacer*
maplibre_native_ffi_frame_pacer_wrap(
  mln_frame_pacer handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
);
G_GNUC_INTERNAL MaplibreNativeFfiGpuDevice* maplibre_native_ffi_gpu_device_wrap(
  mln_gpu_device handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
);
G_GNUC_INTERNAL MaplibreNativeFfiMap* maplibre_native_ffi_map_wrap(
  mln_map handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
);
G_GNUC_INTERNAL MaplibreNativeFfiMapProjection*
maplibre_native_ffi_map_projection_wrap(
  mln_map_projection handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
);
G_GNUC_INTERNAL MaplibreNativeFfiRenderSession*
maplibre_native_ffi_render_session_wrap(
  mln_render_session handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
);
G_GNUC_INTERNAL MaplibreNativeFfiResourceRequest*
maplibre_native_ffi_resource_request_wrap(
  mln_resource_request handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
);
G_GNUC_INTERNAL MaplibreNativeFfiResourceTransformRequest*
maplibre_native_ffi_resource_transform_request_wrap(
  mln_resource_transform_request handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
);
G_GNUC_INTERNAL MaplibreNativeFfiRuntime* maplibre_native_ffi_runtime_wrap(
  mln_runtime handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
);
G_GNUC_INTERNAL MaplibreNativeFfiStyleExpression*
maplibre_native_ffi_style_expression_wrap(
  mln_style_expression handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
);

G_END_DECLS

#endif
