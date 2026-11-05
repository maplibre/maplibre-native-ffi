// Generated from the C headers by tools/bindgen. Do not edit.
#include <maplibre_native_c.h>
#include <string.h>

#include "maplibre-native-ffi-values.h"

#include "maplibre-native-ffi-handles-private.h"
#include "maplibre-native-ffi-private.h"
#include "maplibre-native-ffi-values-private.h"

G_STATIC_ASSERT(
  MAPLIBRE_NATIVE_FFI_DIAGNOSTIC_MESSAGE_CAPACITY ==
  MLN_DIAGNOSTIC_MESSAGE_CAPACITY
);
G_STATIC_ASSERT(
  MAPLIBRE_NATIVE_FFI_FRAME_DEMAND_TOKEN_STILL_IMAGE ==
  MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE
);
G_STATIC_ASSERT(
  MAPLIBRE_NATIVE_FFI_TEXTURE_RING_MAX_DEPTH == MLN_TEXTURE_RING_MAX_DEPTH
);
G_STATIC_ASSERT(
  MAPLIBRE_NATIVE_FFI_VULKAN_NON_DISPATCHABLE_HANDLE_NULL ==
  MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL
);

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiUnitBezier, maplibre_native_ffi_unit_bezier,
  maplibre_native_ffi_unit_bezier_copy, maplibre_native_ffi_unit_bezier_free
)

MaplibreNativeFfiUnitBezier* maplibre_native_ffi_unit_bezier_copy(
  const MaplibreNativeFfiUnitBezier* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_unit_bezier_free(MaplibreNativeFfiUnitBezier* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_unit_bezier_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_unit_bezier_copy(
    (const MaplibreNativeFfiUnitBezier*)self
  );
}

void maplibre_native_ffi_unit_bezier_to_native(
  const MaplibreNativeFfiUnitBezier* self, mln_unit_bezier* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->x1 = (double)(self->x1);
  out->y1 = (double)(self->y1);
  out->x2 = (double)(self->x2);
  out->y2 = (double)(self->y2);
}

void maplibre_native_ffi_unit_bezier_read(
  MaplibreNativeFfiUnitBezier* out, const mln_unit_bezier* in
) {
  out->x1 = (gdouble)(in->x1);
  out->y1 = (gdouble)(in->y1);
  out->x2 = (gdouble)(in->x2);
  out->y2 = (gdouble)(in->y2);
}

MaplibreNativeFfiUnitBezier* maplibre_native_ffi_unit_bezier_from_native(
  const mln_unit_bezier* in
) {
  MaplibreNativeFfiUnitBezier* self = g_new0(MaplibreNativeFfiUnitBezier, 1);
  maplibre_native_ffi_unit_bezier_read(self, in);
  return self;
}

struct _MaplibreNativeFfiCameraTransitionHandler {
  MaplibreNativeFfiClosure* callback;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCameraTransitionHandler,
  maplibre_native_ffi_camera_transition_handler,
  maplibre_native_ffi_camera_transition_handler_copy,
  maplibre_native_ffi_camera_transition_handler_free
)

MaplibreNativeFfiCameraTransitionHandler*
maplibre_native_ffi_camera_transition_handler_new(void) {
  MaplibreNativeFfiCameraTransitionHandler* self =
    g_new0(MaplibreNativeFfiCameraTransitionHandler, 1);
  return self;
}

MaplibreNativeFfiCameraTransitionHandler*
maplibre_native_ffi_camera_transition_handler_copy(
  const MaplibreNativeFfiCameraTransitionHandler* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiCameraTransitionHandler* copy =
    g_memdup2(self, sizeof *self);
  maplibre_native_ffi_closure_ref(copy->callback);
  return copy;
}

void maplibre_native_ffi_camera_transition_handler_free(
  MaplibreNativeFfiCameraTransitionHandler* self
) {
  if (self == NULL) return;
  maplibre_native_ffi_closure_unref(self->callback);
  g_free(self);
}

void maplibre_native_ffi_camera_transition_handler_set_callback(
  MaplibreNativeFfiCameraTransitionHandler* self,
  MaplibreNativeFfiCameraTransitionEndCallback callback, gpointer user_data,
  GDestroyNotify notify
) {
  g_return_if_fail(self != NULL);
  MaplibreNativeFfiClosure* previous = self->callback;
  self->callback =
    callback != NULL
      ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify)
      : NULL;
  if (callback == NULL && notify != NULL) notify(user_data);
  maplibre_native_ffi_closure_unref(previous);
}

gpointer maplibre_native_ffi_camera_transition_handler_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_camera_transition_handler_copy(
    (const MaplibreNativeFfiCameraTransitionHandler*)self
  );
}

static void camera_transition_handler_callback_trampoline(
  void* user_data, const mln_camera_transition_end* end
) {
  MaplibreNativeFfiClosure* closure =
    maplibre_native_ffi_registration_closure(user_data, 0);
  if (closure == NULL) return;
  MaplibreNativeFfiCameraTransitionEnd* end_ =
    maplibre_native_ffi_camera_transition_end_from_native(end);
  ((MaplibreNativeFfiCameraTransitionEndCallback)(void (*)(void))closure->func)(
    end_, closure->user_data
  );
  maplibre_native_ffi_camera_transition_end_free(end_);
}

void maplibre_native_ffi_camera_transition_handler_to_native(
  const MaplibreNativeFfiCameraTransitionHandler* self,
  mln_camera_transition_handler* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  if (self->callback != NULL) {
    MaplibreNativeFfiClosure* const closures[] = {self->callback};
    out->user_data =
      maplibre_native_ffi_call_register(call, closures, 1, &out->queue);
    out->release_user_data = maplibre_native_ffi_registration_release;
    out->callback = self->callback != NULL
                      ? camera_transition_handler_callback_trampoline
                      : NULL;
  }
}

MaplibreNativeFfiCameraTransitionHandler*
maplibre_native_ffi_camera_transition_handler_from_native(
  const mln_camera_transition_handler* in
) {
  MaplibreNativeFfiCameraTransitionHandler* self =
    maplibre_native_ffi_camera_transition_handler_new();
  (void)in;
  return self;
}

struct _MaplibreNativeFfiAnimationOptions {
  guint64 fields;
  gdouble duration_ms;
  gdouble velocity;
  gdouble min_zoom;
  MaplibreNativeFfiUnitBezier easing;
  guint64 transition_id;
  MaplibreNativeFfiCameraTransitionHandler* end_handler;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiAnimationOptions, maplibre_native_ffi_animation_options,
  maplibre_native_ffi_animation_options_copy,
  maplibre_native_ffi_animation_options_free
)

MaplibreNativeFfiAnimationOptions* maplibre_native_ffi_animation_options_new(
  void
) {
  MaplibreNativeFfiAnimationOptions* self =
    g_new0(MaplibreNativeFfiAnimationOptions, 1);
  self->end_handler = maplibre_native_ffi_camera_transition_handler_new();
  return self;
}

MaplibreNativeFfiAnimationOptions* maplibre_native_ffi_animation_options_copy(
  const MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiAnimationOptions* copy = g_memdup2(self, sizeof *self);
  copy->end_handler =
    self->end_handler != NULL
      ? maplibre_native_ffi_camera_transition_handler_copy(self->end_handler)
      : NULL;
  return copy;
}

void maplibre_native_ffi_animation_options_free(
  MaplibreNativeFfiAnimationOptions* self
) {
  if (self == NULL) return;
  g_clear_pointer(
    &self->end_handler, maplibre_native_ffi_camera_transition_handler_free
  );
  g_free(self);
}

gdouble maplibre_native_ffi_animation_options_get_duration_ms(
  const MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->duration_ms;
}

gboolean maplibre_native_ffi_animation_options_has_duration_ms(
  const MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_ANIMATION_OPTION_DURATION) != 0;
}

void maplibre_native_ffi_animation_options_set_duration_ms(
  MaplibreNativeFfiAnimationOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->duration_ms = value;
  self->fields |= MLN_ANIMATION_OPTION_DURATION;
}

gdouble maplibre_native_ffi_animation_options_get_velocity(
  const MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->velocity;
}

gboolean maplibre_native_ffi_animation_options_has_velocity(
  const MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_ANIMATION_OPTION_VELOCITY) != 0;
}

void maplibre_native_ffi_animation_options_set_velocity(
  MaplibreNativeFfiAnimationOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->velocity = value;
  self->fields |= MLN_ANIMATION_OPTION_VELOCITY;
}

gdouble maplibre_native_ffi_animation_options_get_min_zoom(
  const MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->min_zoom;
}

gboolean maplibre_native_ffi_animation_options_has_min_zoom(
  const MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_ANIMATION_OPTION_MIN_ZOOM) != 0;
}

void maplibre_native_ffi_animation_options_set_min_zoom(
  MaplibreNativeFfiAnimationOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->min_zoom = value;
  self->fields |= MLN_ANIMATION_OPTION_MIN_ZOOM;
}

MaplibreNativeFfiUnitBezier* maplibre_native_ffi_animation_options_get_easing(
  const MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiUnitBezier*)&self->easing;
}

gboolean maplibre_native_ffi_animation_options_has_easing(
  const MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_ANIMATION_OPTION_EASING) != 0;
}

void maplibre_native_ffi_animation_options_set_easing(
  MaplibreNativeFfiAnimationOptions* self,
  const MaplibreNativeFfiUnitBezier* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->easing = *value;
  self->fields |= MLN_ANIMATION_OPTION_EASING;
}

guint64 maplibre_native_ffi_animation_options_get_transition_id(
  const MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->transition_id;
}

gboolean maplibre_native_ffi_animation_options_has_transition_id(
  const MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_ANIMATION_OPTION_TRANSITION_ID) != 0;
}

void maplibre_native_ffi_animation_options_set_transition_id(
  MaplibreNativeFfiAnimationOptions* self, guint64 value
) {
  g_return_if_fail(self != NULL);
  self->transition_id = value;
  self->fields |= MLN_ANIMATION_OPTION_TRANSITION_ID;
}

MaplibreNativeFfiCameraTransitionHandler*
maplibre_native_ffi_animation_options_get_end_handler(
  const MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->end_handler;
}

void maplibre_native_ffi_animation_options_set_end_handler(
  MaplibreNativeFfiAnimationOptions* self,
  const MaplibreNativeFfiCameraTransitionHandler* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiCameraTransitionHandler* previous = self->end_handler;
  self->end_handler = maplibre_native_ffi_camera_transition_handler_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_camera_transition_handler_free(previous);
}

gpointer maplibre_native_ffi_animation_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_animation_options_copy(
    (const MaplibreNativeFfiAnimationOptions*)self
  );
}

void maplibre_native_ffi_animation_options_to_native(
  const MaplibreNativeFfiAnimationOptions* self, mln_animation_options* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->fields = (mln_animation_option_fields)self->fields;
  out->duration_ms = (double)(self->duration_ms);
  out->velocity = (double)(self->velocity);
  out->min_zoom = (double)(self->min_zoom);
  maplibre_native_ffi_unit_bezier_to_native(&self->easing, &out->easing, call);
  out->transition_id = (uint64_t)(self->transition_id);
  if (self->end_handler != NULL)
    maplibre_native_ffi_camera_transition_handler_to_native(
      self->end_handler, &out->end_handler, call
    );
}

MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_animation_options_from_native(
  const mln_animation_options* in
) {
  MaplibreNativeFfiAnimationOptions* self =
    maplibre_native_ffi_animation_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_ANIMATION_OPTION_DURATION) != 0) {
    self->duration_ms = (gdouble)(in->duration_ms);
  }
  if ((in->fields & MLN_ANIMATION_OPTION_VELOCITY) != 0) {
    self->velocity = (gdouble)(in->velocity);
  }
  if ((in->fields & MLN_ANIMATION_OPTION_MIN_ZOOM) != 0) {
    self->min_zoom = (gdouble)(in->min_zoom);
  }
  if ((in->fields & MLN_ANIMATION_OPTION_EASING) != 0) {
    maplibre_native_ffi_unit_bezier_read(&self->easing, &in->easing);
  }
  if ((in->fields & MLN_ANIMATION_OPTION_TRANSITION_ID) != 0) {
    self->transition_id = (guint64)(in->transition_id);
  }
  g_clear_pointer(
    &self->end_handler, maplibre_native_ffi_camera_transition_handler_free
  );
  self->end_handler =
    maplibre_native_ffi_camera_transition_handler_from_native(&in->end_handler);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiLatLng, maplibre_native_ffi_lat_lng,
  maplibre_native_ffi_lat_lng_copy, maplibre_native_ffi_lat_lng_free
)

MaplibreNativeFfiLatLng* maplibre_native_ffi_lat_lng_copy(
  const MaplibreNativeFfiLatLng* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_lat_lng_free(MaplibreNativeFfiLatLng* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_lat_lng_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_lat_lng_copy((const MaplibreNativeFfiLatLng*)self);
}

void maplibre_native_ffi_lat_lng_to_native(
  const MaplibreNativeFfiLatLng* self, mln_lat_lng* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->latitude = (double)(self->latitude);
  out->longitude = (double)(self->longitude);
}

void maplibre_native_ffi_lat_lng_read(
  MaplibreNativeFfiLatLng* out, const mln_lat_lng* in
) {
  out->latitude = (gdouble)(in->latitude);
  out->longitude = (gdouble)(in->longitude);
}

MaplibreNativeFfiLatLng* maplibre_native_ffi_lat_lng_from_native(
  const mln_lat_lng* in
) {
  MaplibreNativeFfiLatLng* self = g_new0(MaplibreNativeFfiLatLng, 1);
  maplibre_native_ffi_lat_lng_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiLatLngBounds, maplibre_native_ffi_lat_lng_bounds,
  maplibre_native_ffi_lat_lng_bounds_copy,
  maplibre_native_ffi_lat_lng_bounds_free
)

MaplibreNativeFfiLatLngBounds* maplibre_native_ffi_lat_lng_bounds_copy(
  const MaplibreNativeFfiLatLngBounds* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_lat_lng_bounds_free(
  MaplibreNativeFfiLatLngBounds* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_lat_lng_bounds_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_lat_lng_bounds_copy(
    (const MaplibreNativeFfiLatLngBounds*)self
  );
}

void maplibre_native_ffi_lat_lng_bounds_to_native(
  const MaplibreNativeFfiLatLngBounds* self, mln_lat_lng_bounds* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  maplibre_native_ffi_lat_lng_to_native(
    &self->southwest, &out->southwest, call
  );
  maplibre_native_ffi_lat_lng_to_native(
    &self->northeast, &out->northeast, call
  );
}

void maplibre_native_ffi_lat_lng_bounds_read(
  MaplibreNativeFfiLatLngBounds* out, const mln_lat_lng_bounds* in
) {
  maplibre_native_ffi_lat_lng_read(&out->southwest, &in->southwest);
  maplibre_native_ffi_lat_lng_read(&out->northeast, &in->northeast);
}

MaplibreNativeFfiLatLngBounds* maplibre_native_ffi_lat_lng_bounds_from_native(
  const mln_lat_lng_bounds* in
) {
  MaplibreNativeFfiLatLngBounds* self =
    g_new0(MaplibreNativeFfiLatLngBounds, 1);
  maplibre_native_ffi_lat_lng_bounds_read(self, in);
  return self;
}

struct _MaplibreNativeFfiBoundOptions {
  guint64 fields;
  MaplibreNativeFfiLatLngBounds bounds;
  gdouble min_zoom;
  gdouble max_zoom;
  gdouble min_pitch;
  gdouble max_pitch;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiBoundOptions, maplibre_native_ffi_bound_options,
  maplibre_native_ffi_bound_options_copy, maplibre_native_ffi_bound_options_free
)

MaplibreNativeFfiBoundOptions* maplibre_native_ffi_bound_options_new(void) {
  MaplibreNativeFfiBoundOptions* self =
    g_new0(MaplibreNativeFfiBoundOptions, 1);
  return self;
}

MaplibreNativeFfiBoundOptions* maplibre_native_ffi_bound_options_copy(
  const MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiBoundOptions* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_bound_options_free(
  MaplibreNativeFfiBoundOptions* self
) {
  if (self == NULL) return;
  g_free(self);
}

MaplibreNativeFfiLatLngBounds* maplibre_native_ffi_bound_options_get_bounds(
  const MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLatLngBounds*)&self->bounds;
}

gboolean maplibre_native_ffi_bound_options_has_bounds(
  const MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_BOUND_OPTION_BOUNDS) != 0;
}

void maplibre_native_ffi_bound_options_set_bounds(
  MaplibreNativeFfiBoundOptions* self,
  const MaplibreNativeFfiLatLngBounds* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->bounds = *value;
  self->fields |= MLN_BOUND_OPTION_BOUNDS;
}

gdouble maplibre_native_ffi_bound_options_get_min_zoom(
  const MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->min_zoom;
}

gboolean maplibre_native_ffi_bound_options_has_min_zoom(
  const MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_BOUND_OPTION_MIN_ZOOM) != 0;
}

void maplibre_native_ffi_bound_options_set_min_zoom(
  MaplibreNativeFfiBoundOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->min_zoom = value;
  self->fields |= MLN_BOUND_OPTION_MIN_ZOOM;
}

gdouble maplibre_native_ffi_bound_options_get_max_zoom(
  const MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->max_zoom;
}

gboolean maplibre_native_ffi_bound_options_has_max_zoom(
  const MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_BOUND_OPTION_MAX_ZOOM) != 0;
}

void maplibre_native_ffi_bound_options_set_max_zoom(
  MaplibreNativeFfiBoundOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->max_zoom = value;
  self->fields |= MLN_BOUND_OPTION_MAX_ZOOM;
}

gdouble maplibre_native_ffi_bound_options_get_min_pitch(
  const MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->min_pitch;
}

gboolean maplibre_native_ffi_bound_options_has_min_pitch(
  const MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_BOUND_OPTION_MIN_PITCH) != 0;
}

void maplibre_native_ffi_bound_options_set_min_pitch(
  MaplibreNativeFfiBoundOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->min_pitch = value;
  self->fields |= MLN_BOUND_OPTION_MIN_PITCH;
}

gdouble maplibre_native_ffi_bound_options_get_max_pitch(
  const MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->max_pitch;
}

gboolean maplibre_native_ffi_bound_options_has_max_pitch(
  const MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_BOUND_OPTION_MAX_PITCH) != 0;
}

void maplibre_native_ffi_bound_options_set_max_pitch(
  MaplibreNativeFfiBoundOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->max_pitch = value;
  self->fields |= MLN_BOUND_OPTION_MAX_PITCH;
}

gpointer maplibre_native_ffi_bound_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_bound_options_copy(
    (const MaplibreNativeFfiBoundOptions*)self
  );
}

void maplibre_native_ffi_bound_options_to_native(
  const MaplibreNativeFfiBoundOptions* self, mln_bound_options* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_bound_option_fields)self->fields;
  maplibre_native_ffi_lat_lng_bounds_to_native(
    &self->bounds, &out->bounds, call
  );
  out->min_zoom = (double)(self->min_zoom);
  out->max_zoom = (double)(self->max_zoom);
  out->min_pitch = (double)(self->min_pitch);
  out->max_pitch = (double)(self->max_pitch);
}

MaplibreNativeFfiBoundOptions* maplibre_native_ffi_bound_options_from_native(
  const mln_bound_options* in
) {
  MaplibreNativeFfiBoundOptions* self = maplibre_native_ffi_bound_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_BOUND_OPTION_BOUNDS) != 0) {
    maplibre_native_ffi_lat_lng_bounds_read(&self->bounds, &in->bounds);
  }
  if ((in->fields & MLN_BOUND_OPTION_MIN_ZOOM) != 0) {
    self->min_zoom = (gdouble)(in->min_zoom);
  }
  if ((in->fields & MLN_BOUND_OPTION_MAX_ZOOM) != 0) {
    self->max_zoom = (gdouble)(in->max_zoom);
  }
  if ((in->fields & MLN_BOUND_OPTION_MIN_PITCH) != 0) {
    self->min_pitch = (gdouble)(in->min_pitch);
  }
  if ((in->fields & MLN_BOUND_OPTION_MAX_PITCH) != 0) {
    self->max_pitch = (gdouble)(in->max_pitch);
  }
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiEdgeInsets, maplibre_native_ffi_edge_insets,
  maplibre_native_ffi_edge_insets_copy, maplibre_native_ffi_edge_insets_free
)

MaplibreNativeFfiEdgeInsets* maplibre_native_ffi_edge_insets_copy(
  const MaplibreNativeFfiEdgeInsets* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_edge_insets_free(MaplibreNativeFfiEdgeInsets* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_edge_insets_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_edge_insets_copy(
    (const MaplibreNativeFfiEdgeInsets*)self
  );
}

void maplibre_native_ffi_edge_insets_to_native(
  const MaplibreNativeFfiEdgeInsets* self, mln_edge_insets* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->top = (double)(self->top);
  out->left = (double)(self->left);
  out->bottom = (double)(self->bottom);
  out->right = (double)(self->right);
}

void maplibre_native_ffi_edge_insets_read(
  MaplibreNativeFfiEdgeInsets* out, const mln_edge_insets* in
) {
  out->top = (gdouble)(in->top);
  out->left = (gdouble)(in->left);
  out->bottom = (gdouble)(in->bottom);
  out->right = (gdouble)(in->right);
}

MaplibreNativeFfiEdgeInsets* maplibre_native_ffi_edge_insets_from_native(
  const mln_edge_insets* in
) {
  MaplibreNativeFfiEdgeInsets* self = g_new0(MaplibreNativeFfiEdgeInsets, 1);
  maplibre_native_ffi_edge_insets_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCamera, maplibre_native_ffi_camera,
  maplibre_native_ffi_camera_copy, maplibre_native_ffi_camera_free
)

MaplibreNativeFfiCamera* maplibre_native_ffi_camera_copy(
  const MaplibreNativeFfiCamera* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_camera_free(MaplibreNativeFfiCamera* self) {
  g_free(self);
}

MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_to_options(
  const MaplibreNativeFfiCamera* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiCameraOptions* options =
    maplibre_native_ffi_camera_options_new();
  maplibre_native_ffi_camera_options_set_center(options, &self->center);
  maplibre_native_ffi_camera_options_set_center_altitude(
    options, self->center_altitude
  );
  maplibre_native_ffi_camera_options_set_padding(options, &self->padding);
  maplibre_native_ffi_camera_options_set_zoom(options, self->zoom);
  maplibre_native_ffi_camera_options_set_bearing(options, self->bearing);
  maplibre_native_ffi_camera_options_set_pitch(options, self->pitch);
  maplibre_native_ffi_camera_options_set_roll(options, self->roll);
  maplibre_native_ffi_camera_options_set_field_of_view(
    options, self->field_of_view
  );
  return options;
}

gpointer maplibre_native_ffi_camera_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_camera_copy((const MaplibreNativeFfiCamera*)self);
}

void maplibre_native_ffi_camera_to_native(
  const MaplibreNativeFfiCamera* self, mln_camera* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  maplibre_native_ffi_lat_lng_to_native(&self->center, &out->center, call);
  out->center_altitude = (double)(self->center_altitude);
  maplibre_native_ffi_edge_insets_to_native(
    &self->padding, &out->padding, call
  );
  out->zoom = (double)(self->zoom);
  out->bearing = (double)(self->bearing);
  out->pitch = (double)(self->pitch);
  out->roll = (double)(self->roll);
  out->field_of_view = (double)(self->field_of_view);
}

void maplibre_native_ffi_camera_read(
  MaplibreNativeFfiCamera* out, const mln_camera* in
) {
  maplibre_native_ffi_lat_lng_read(&out->center, &in->center);
  out->center_altitude = (gdouble)(in->center_altitude);
  maplibre_native_ffi_edge_insets_read(&out->padding, &in->padding);
  out->zoom = (gdouble)(in->zoom);
  out->bearing = (gdouble)(in->bearing);
  out->pitch = (gdouble)(in->pitch);
  out->roll = (gdouble)(in->roll);
  out->field_of_view = (gdouble)(in->field_of_view);
}

MaplibreNativeFfiCamera* maplibre_native_ffi_camera_from_native(
  const mln_camera* in
) {
  MaplibreNativeFfiCamera* self = g_new0(MaplibreNativeFfiCamera, 1);
  maplibre_native_ffi_camera_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCameraBounds, maplibre_native_ffi_camera_bounds,
  maplibre_native_ffi_camera_bounds_copy, maplibre_native_ffi_camera_bounds_free
)

MaplibreNativeFfiCameraBounds* maplibre_native_ffi_camera_bounds_copy(
  const MaplibreNativeFfiCameraBounds* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_camera_bounds_free(
  MaplibreNativeFfiCameraBounds* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_camera_bounds_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_camera_bounds_copy(
    (const MaplibreNativeFfiCameraBounds*)self
  );
}

void maplibre_native_ffi_camera_bounds_to_native(
  const MaplibreNativeFfiCameraBounds* self, mln_camera_bounds* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  maplibre_native_ffi_lat_lng_bounds_to_native(
    &self->bounds, &out->bounds, call
  );
  out->unbounded = (self->unbounded) != FALSE;
  out->min_zoom = (double)(self->min_zoom);
  out->max_zoom = (double)(self->max_zoom);
  out->min_pitch = (double)(self->min_pitch);
  out->max_pitch = (double)(self->max_pitch);
}

void maplibre_native_ffi_camera_bounds_read(
  MaplibreNativeFfiCameraBounds* out, const mln_camera_bounds* in
) {
  maplibre_native_ffi_lat_lng_bounds_read(&out->bounds, &in->bounds);
  out->unbounded = (in->unbounded) ? TRUE : FALSE;
  out->min_zoom = (gdouble)(in->min_zoom);
  out->max_zoom = (gdouble)(in->max_zoom);
  out->min_pitch = (gdouble)(in->min_pitch);
  out->max_pitch = (gdouble)(in->max_pitch);
}

MaplibreNativeFfiCameraBounds* maplibre_native_ffi_camera_bounds_from_native(
  const mln_camera_bounds* in
) {
  MaplibreNativeFfiCameraBounds* self =
    g_new0(MaplibreNativeFfiCameraBounds, 1);
  maplibre_native_ffi_camera_bounds_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiScreenPoint, maplibre_native_ffi_screen_point,
  maplibre_native_ffi_screen_point_copy, maplibre_native_ffi_screen_point_free
)

MaplibreNativeFfiScreenPoint* maplibre_native_ffi_screen_point_copy(
  const MaplibreNativeFfiScreenPoint* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_screen_point_free(MaplibreNativeFfiScreenPoint* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_screen_point_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_screen_point_copy(
    (const MaplibreNativeFfiScreenPoint*)self
  );
}

void maplibre_native_ffi_screen_point_to_native(
  const MaplibreNativeFfiScreenPoint* self, mln_screen_point* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->x = (double)(self->x);
  out->y = (double)(self->y);
}

void maplibre_native_ffi_screen_point_read(
  MaplibreNativeFfiScreenPoint* out, const mln_screen_point* in
) {
  out->x = (gdouble)(in->x);
  out->y = (gdouble)(in->y);
}

MaplibreNativeFfiScreenPoint* maplibre_native_ffi_screen_point_from_native(
  const mln_screen_point* in
) {
  MaplibreNativeFfiScreenPoint* self = g_new0(MaplibreNativeFfiScreenPoint, 1);
  maplibre_native_ffi_screen_point_read(self, in);
  return self;
}

struct _MaplibreNativeFfiCameraDelta {
  guint64 fields;
  MaplibreNativeFfiScreenPoint offset;
  gdouble scale;
  gdouble bearing;
  gdouble pitch;
  MaplibreNativeFfiScreenPoint anchor;
  MaplibreNativeFfiAnimationOptions* animation;
  MaplibreNativeFfiGesturePhase gesture_phase;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCameraDelta, maplibre_native_ffi_camera_delta,
  maplibre_native_ffi_camera_delta_copy, maplibre_native_ffi_camera_delta_free
)

MaplibreNativeFfiCameraDelta* maplibre_native_ffi_camera_delta_new(void) {
  MaplibreNativeFfiCameraDelta* self = g_new0(MaplibreNativeFfiCameraDelta, 1);
  self->animation = maplibre_native_ffi_animation_options_new();
  return self;
}

MaplibreNativeFfiCameraDelta* maplibre_native_ffi_camera_delta_copy(
  const MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiCameraDelta* copy = g_memdup2(self, sizeof *self);
  copy->animation =
    self->animation != NULL
      ? maplibre_native_ffi_animation_options_copy(self->animation)
      : NULL;
  return copy;
}

void maplibre_native_ffi_camera_delta_free(MaplibreNativeFfiCameraDelta* self) {
  if (self == NULL) return;
  g_clear_pointer(&self->animation, maplibre_native_ffi_animation_options_free);
  g_free(self);
}

MaplibreNativeFfiScreenPoint* maplibre_native_ffi_camera_delta_get_offset(
  const MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiScreenPoint*)&self->offset;
}

gboolean maplibre_native_ffi_camera_delta_has_offset(
  const MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_DELTA_OFFSET) != 0;
}

void maplibre_native_ffi_camera_delta_set_offset(
  MaplibreNativeFfiCameraDelta* self, const MaplibreNativeFfiScreenPoint* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->offset = *value;
  self->fields |= MLN_CAMERA_DELTA_OFFSET;
}

gdouble maplibre_native_ffi_camera_delta_get_scale(
  const MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->scale;
}

gboolean maplibre_native_ffi_camera_delta_has_scale(
  const MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_DELTA_SCALE) != 0;
}

void maplibre_native_ffi_camera_delta_set_scale(
  MaplibreNativeFfiCameraDelta* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->scale = value;
  self->fields |= MLN_CAMERA_DELTA_SCALE;
}

gdouble maplibre_native_ffi_camera_delta_get_bearing(
  const MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->bearing;
}

gboolean maplibre_native_ffi_camera_delta_has_bearing(
  const MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_DELTA_BEARING) != 0;
}

void maplibre_native_ffi_camera_delta_set_bearing(
  MaplibreNativeFfiCameraDelta* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->bearing = value;
  self->fields |= MLN_CAMERA_DELTA_BEARING;
}

gdouble maplibre_native_ffi_camera_delta_get_pitch(
  const MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->pitch;
}

gboolean maplibre_native_ffi_camera_delta_has_pitch(
  const MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_DELTA_PITCH) != 0;
}

void maplibre_native_ffi_camera_delta_set_pitch(
  MaplibreNativeFfiCameraDelta* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->pitch = value;
  self->fields |= MLN_CAMERA_DELTA_PITCH;
}

MaplibreNativeFfiScreenPoint* maplibre_native_ffi_camera_delta_get_anchor(
  const MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiScreenPoint*)&self->anchor;
}

gboolean maplibre_native_ffi_camera_delta_has_anchor(
  const MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_DELTA_ANCHOR) != 0;
}

void maplibre_native_ffi_camera_delta_set_anchor(
  MaplibreNativeFfiCameraDelta* self, const MaplibreNativeFfiScreenPoint* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->anchor = *value;
  self->fields |= MLN_CAMERA_DELTA_ANCHOR;
}

MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_camera_delta_get_animation(
  const MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->animation;
}

void maplibre_native_ffi_camera_delta_set_animation(
  MaplibreNativeFfiCameraDelta* self,
  const MaplibreNativeFfiAnimationOptions* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiAnimationOptions* previous = self->animation;
  self->animation = maplibre_native_ffi_animation_options_copy(value);
  if (previous != NULL) maplibre_native_ffi_animation_options_free(previous);
}

MaplibreNativeFfiGesturePhase
maplibre_native_ffi_camera_delta_get_gesture_phase(
  const MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiGesturePhase)0);
  return self->gesture_phase;
}

void maplibre_native_ffi_camera_delta_set_gesture_phase(
  MaplibreNativeFfiCameraDelta* self, MaplibreNativeFfiGesturePhase value
) {
  g_return_if_fail(self != NULL);
  self->gesture_phase = value;
}

gpointer maplibre_native_ffi_camera_delta_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_camera_delta_copy(
    (const MaplibreNativeFfiCameraDelta*)self
  );
}

void maplibre_native_ffi_camera_delta_to_native(
  const MaplibreNativeFfiCameraDelta* self, mln_camera_delta* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_camera_delta_fields)self->fields;
  maplibre_native_ffi_screen_point_to_native(&self->offset, &out->offset, call);
  out->scale = (double)(self->scale);
  out->bearing = (double)(self->bearing);
  out->pitch = (double)(self->pitch);
  maplibre_native_ffi_screen_point_to_native(&self->anchor, &out->anchor, call);
  if (self->animation != NULL)
    maplibre_native_ffi_animation_options_to_native(
      self->animation, &out->animation, call
    );
  out->gesture_phase = (mln_gesture_phase)(self->gesture_phase);
}

MaplibreNativeFfiCameraDelta* maplibre_native_ffi_camera_delta_from_native(
  const mln_camera_delta* in
) {
  MaplibreNativeFfiCameraDelta* self = maplibre_native_ffi_camera_delta_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_CAMERA_DELTA_OFFSET) != 0) {
    maplibre_native_ffi_screen_point_read(&self->offset, &in->offset);
  }
  if ((in->fields & MLN_CAMERA_DELTA_SCALE) != 0) {
    self->scale = (gdouble)(in->scale);
  }
  if ((in->fields & MLN_CAMERA_DELTA_BEARING) != 0) {
    self->bearing = (gdouble)(in->bearing);
  }
  if ((in->fields & MLN_CAMERA_DELTA_PITCH) != 0) {
    self->pitch = (gdouble)(in->pitch);
  }
  if ((in->fields & MLN_CAMERA_DELTA_ANCHOR) != 0) {
    maplibre_native_ffi_screen_point_read(&self->anchor, &in->anchor);
  }
  g_clear_pointer(&self->animation, maplibre_native_ffi_animation_options_free);
  self->animation =
    maplibre_native_ffi_animation_options_from_native(&in->animation);
  self->gesture_phase = (MaplibreNativeFfiGesturePhase)(in->gesture_phase);
  return self;
}

struct _MaplibreNativeFfiCameraFitOptions {
  guint64 fields;
  MaplibreNativeFfiEdgeInsets padding;
  gdouble bearing;
  gdouble pitch;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCameraFitOptions, maplibre_native_ffi_camera_fit_options,
  maplibre_native_ffi_camera_fit_options_copy,
  maplibre_native_ffi_camera_fit_options_free
)

MaplibreNativeFfiCameraFitOptions* maplibre_native_ffi_camera_fit_options_new(
  void
) {
  MaplibreNativeFfiCameraFitOptions* self =
    g_new0(MaplibreNativeFfiCameraFitOptions, 1);
  return self;
}

MaplibreNativeFfiCameraFitOptions* maplibre_native_ffi_camera_fit_options_copy(
  const MaplibreNativeFfiCameraFitOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiCameraFitOptions* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_camera_fit_options_free(
  MaplibreNativeFfiCameraFitOptions* self
) {
  if (self == NULL) return;
  g_free(self);
}

MaplibreNativeFfiEdgeInsets* maplibre_native_ffi_camera_fit_options_get_padding(
  const MaplibreNativeFfiCameraFitOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiEdgeInsets*)&self->padding;
}

gboolean maplibre_native_ffi_camera_fit_options_has_padding(
  const MaplibreNativeFfiCameraFitOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_FIT_OPTION_PADDING) != 0;
}

void maplibre_native_ffi_camera_fit_options_set_padding(
  MaplibreNativeFfiCameraFitOptions* self,
  const MaplibreNativeFfiEdgeInsets* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->padding = *value;
  self->fields |= MLN_CAMERA_FIT_OPTION_PADDING;
}

gdouble maplibre_native_ffi_camera_fit_options_get_bearing(
  const MaplibreNativeFfiCameraFitOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->bearing;
}

gboolean maplibre_native_ffi_camera_fit_options_has_bearing(
  const MaplibreNativeFfiCameraFitOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_FIT_OPTION_BEARING) != 0;
}

void maplibre_native_ffi_camera_fit_options_set_bearing(
  MaplibreNativeFfiCameraFitOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->bearing = value;
  self->fields |= MLN_CAMERA_FIT_OPTION_BEARING;
}

gdouble maplibre_native_ffi_camera_fit_options_get_pitch(
  const MaplibreNativeFfiCameraFitOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->pitch;
}

gboolean maplibre_native_ffi_camera_fit_options_has_pitch(
  const MaplibreNativeFfiCameraFitOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_FIT_OPTION_PITCH) != 0;
}

void maplibre_native_ffi_camera_fit_options_set_pitch(
  MaplibreNativeFfiCameraFitOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->pitch = value;
  self->fields |= MLN_CAMERA_FIT_OPTION_PITCH;
}

gpointer maplibre_native_ffi_camera_fit_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_camera_fit_options_copy(
    (const MaplibreNativeFfiCameraFitOptions*)self
  );
}

void maplibre_native_ffi_camera_fit_options_to_native(
  const MaplibreNativeFfiCameraFitOptions* self, mln_camera_fit_options* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_camera_fit_option_fields)self->fields;
  maplibre_native_ffi_edge_insets_to_native(
    &self->padding, &out->padding, call
  );
  out->bearing = (double)(self->bearing);
  out->pitch = (double)(self->pitch);
}

MaplibreNativeFfiCameraFitOptions*
maplibre_native_ffi_camera_fit_options_from_native(
  const mln_camera_fit_options* in
) {
  MaplibreNativeFfiCameraFitOptions* self =
    maplibre_native_ffi_camera_fit_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_CAMERA_FIT_OPTION_PADDING) != 0) {
    maplibre_native_ffi_edge_insets_read(&self->padding, &in->padding);
  }
  if ((in->fields & MLN_CAMERA_FIT_OPTION_BEARING) != 0) {
    self->bearing = (gdouble)(in->bearing);
  }
  if ((in->fields & MLN_CAMERA_FIT_OPTION_PITCH) != 0) {
    self->pitch = (gdouble)(in->pitch);
  }
  return self;
}

struct _MaplibreNativeFfiCameraFitUpdate {
  MaplibreNativeFfiCameraUpdateMode mode;
  MaplibreNativeFfiCameraFitOptions* fit;
  MaplibreNativeFfiAnimationOptions* animation;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCameraFitUpdate, maplibre_native_ffi_camera_fit_update,
  maplibre_native_ffi_camera_fit_update_copy,
  maplibre_native_ffi_camera_fit_update_free
)

MaplibreNativeFfiCameraFitUpdate* maplibre_native_ffi_camera_fit_update_new(
  void
) {
  MaplibreNativeFfiCameraFitUpdate* self =
    g_new0(MaplibreNativeFfiCameraFitUpdate, 1);
  self->fit = maplibre_native_ffi_camera_fit_options_new();
  self->animation = maplibre_native_ffi_animation_options_new();
  return self;
}

MaplibreNativeFfiCameraFitUpdate* maplibre_native_ffi_camera_fit_update_copy(
  const MaplibreNativeFfiCameraFitUpdate* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiCameraFitUpdate* copy = g_memdup2(self, sizeof *self);
  copy->fit = self->fit != NULL
                ? maplibre_native_ffi_camera_fit_options_copy(self->fit)
                : NULL;
  copy->animation =
    self->animation != NULL
      ? maplibre_native_ffi_animation_options_copy(self->animation)
      : NULL;
  return copy;
}

void maplibre_native_ffi_camera_fit_update_free(
  MaplibreNativeFfiCameraFitUpdate* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->fit, maplibre_native_ffi_camera_fit_options_free);
  g_clear_pointer(&self->animation, maplibre_native_ffi_animation_options_free);
  g_free(self);
}

MaplibreNativeFfiCameraUpdateMode
maplibre_native_ffi_camera_fit_update_get_mode(
  const MaplibreNativeFfiCameraFitUpdate* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiCameraUpdateMode)0);
  return self->mode;
}

void maplibre_native_ffi_camera_fit_update_set_mode(
  MaplibreNativeFfiCameraFitUpdate* self,
  MaplibreNativeFfiCameraUpdateMode value
) {
  g_return_if_fail(self != NULL);
  self->mode = value;
}

MaplibreNativeFfiCameraFitOptions*
maplibre_native_ffi_camera_fit_update_get_fit(
  const MaplibreNativeFfiCameraFitUpdate* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->fit;
}

void maplibre_native_ffi_camera_fit_update_set_fit(
  MaplibreNativeFfiCameraFitUpdate* self,
  const MaplibreNativeFfiCameraFitOptions* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiCameraFitOptions* previous = self->fit;
  self->fit = maplibre_native_ffi_camera_fit_options_copy(value);
  if (previous != NULL) maplibre_native_ffi_camera_fit_options_free(previous);
}

MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_camera_fit_update_get_animation(
  const MaplibreNativeFfiCameraFitUpdate* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->animation;
}

void maplibre_native_ffi_camera_fit_update_set_animation(
  MaplibreNativeFfiCameraFitUpdate* self,
  const MaplibreNativeFfiAnimationOptions* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiAnimationOptions* previous = self->animation;
  self->animation = maplibre_native_ffi_animation_options_copy(value);
  if (previous != NULL) maplibre_native_ffi_animation_options_free(previous);
}

gpointer maplibre_native_ffi_camera_fit_update_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_camera_fit_update_copy(
    (const MaplibreNativeFfiCameraFitUpdate*)self
  );
}

void maplibre_native_ffi_camera_fit_update_to_native(
  const MaplibreNativeFfiCameraFitUpdate* self, mln_camera_fit_update* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->mode = (mln_camera_update_mode)(self->mode);
  if (self->fit != NULL)
    maplibre_native_ffi_camera_fit_options_to_native(
      self->fit, &out->fit, call
    );
  if (self->animation != NULL)
    maplibre_native_ffi_animation_options_to_native(
      self->animation, &out->animation, call
    );
}

MaplibreNativeFfiCameraFitUpdate*
maplibre_native_ffi_camera_fit_update_from_native(
  const mln_camera_fit_update* in
) {
  MaplibreNativeFfiCameraFitUpdate* self =
    maplibre_native_ffi_camera_fit_update_new();
  self->mode = (MaplibreNativeFfiCameraUpdateMode)(in->mode);
  g_clear_pointer(&self->fit, maplibre_native_ffi_camera_fit_options_free);
  self->fit = maplibre_native_ffi_camera_fit_options_from_native(&in->fit);
  g_clear_pointer(&self->animation, maplibre_native_ffi_animation_options_free);
  self->animation =
    maplibre_native_ffi_animation_options_from_native(&in->animation);
  return self;
}

struct _MaplibreNativeFfiCameraOptions {
  guint64 fields;
  MaplibreNativeFfiLatLng center;
  gdouble center_altitude;
  MaplibreNativeFfiEdgeInsets padding;
  MaplibreNativeFfiScreenPoint anchor;
  gdouble zoom;
  gdouble bearing;
  gdouble pitch;
  gdouble roll;
  gdouble field_of_view;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCameraOptions, maplibre_native_ffi_camera_options,
  maplibre_native_ffi_camera_options_copy,
  maplibre_native_ffi_camera_options_free
)

MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_options_new(void) {
  MaplibreNativeFfiCameraOptions* self =
    g_new0(MaplibreNativeFfiCameraOptions, 1);
  return self;
}

MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_options_copy(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiCameraOptions* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_camera_options_free(
  MaplibreNativeFfiCameraOptions* self
) {
  if (self == NULL) return;
  g_free(self);
}

MaplibreNativeFfiLatLng* maplibre_native_ffi_camera_options_get_center(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLatLng*)&self->center;
}

gboolean maplibre_native_ffi_camera_options_has_center(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_OPTION_CENTER) != 0;
}

void maplibre_native_ffi_camera_options_set_center(
  MaplibreNativeFfiCameraOptions* self, const MaplibreNativeFfiLatLng* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->center = *value;
  self->fields |= MLN_CAMERA_OPTION_CENTER;
}

gdouble maplibre_native_ffi_camera_options_get_center_altitude(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->center_altitude;
}

gboolean maplibre_native_ffi_camera_options_has_center_altitude(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0;
}

void maplibre_native_ffi_camera_options_set_center_altitude(
  MaplibreNativeFfiCameraOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->center_altitude = value;
  self->fields |= MLN_CAMERA_OPTION_CENTER_ALTITUDE;
}

MaplibreNativeFfiEdgeInsets* maplibre_native_ffi_camera_options_get_padding(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiEdgeInsets*)&self->padding;
}

gboolean maplibre_native_ffi_camera_options_has_padding(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_OPTION_PADDING) != 0;
}

void maplibre_native_ffi_camera_options_set_padding(
  MaplibreNativeFfiCameraOptions* self, const MaplibreNativeFfiEdgeInsets* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->padding = *value;
  self->fields |= MLN_CAMERA_OPTION_PADDING;
}

MaplibreNativeFfiScreenPoint* maplibre_native_ffi_camera_options_get_anchor(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiScreenPoint*)&self->anchor;
}

gboolean maplibre_native_ffi_camera_options_has_anchor(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_OPTION_ANCHOR) != 0;
}

void maplibre_native_ffi_camera_options_set_anchor(
  MaplibreNativeFfiCameraOptions* self,
  const MaplibreNativeFfiScreenPoint* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->anchor = *value;
  self->fields |= MLN_CAMERA_OPTION_ANCHOR;
}

gdouble maplibre_native_ffi_camera_options_get_zoom(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->zoom;
}

gboolean maplibre_native_ffi_camera_options_has_zoom(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_OPTION_ZOOM) != 0;
}

void maplibre_native_ffi_camera_options_set_zoom(
  MaplibreNativeFfiCameraOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->zoom = value;
  self->fields |= MLN_CAMERA_OPTION_ZOOM;
}

gdouble maplibre_native_ffi_camera_options_get_bearing(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->bearing;
}

gboolean maplibre_native_ffi_camera_options_has_bearing(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_OPTION_BEARING) != 0;
}

void maplibre_native_ffi_camera_options_set_bearing(
  MaplibreNativeFfiCameraOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->bearing = value;
  self->fields |= MLN_CAMERA_OPTION_BEARING;
}

gdouble maplibre_native_ffi_camera_options_get_pitch(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->pitch;
}

gboolean maplibre_native_ffi_camera_options_has_pitch(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_OPTION_PITCH) != 0;
}

void maplibre_native_ffi_camera_options_set_pitch(
  MaplibreNativeFfiCameraOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->pitch = value;
  self->fields |= MLN_CAMERA_OPTION_PITCH;
}

gdouble maplibre_native_ffi_camera_options_get_roll(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->roll;
}

gboolean maplibre_native_ffi_camera_options_has_roll(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_OPTION_ROLL) != 0;
}

void maplibre_native_ffi_camera_options_set_roll(
  MaplibreNativeFfiCameraOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->roll = value;
  self->fields |= MLN_CAMERA_OPTION_ROLL;
}

gdouble maplibre_native_ffi_camera_options_get_field_of_view(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->field_of_view;
}

gboolean maplibre_native_ffi_camera_options_has_field_of_view(
  const MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CAMERA_OPTION_FOV) != 0;
}

void maplibre_native_ffi_camera_options_set_field_of_view(
  MaplibreNativeFfiCameraOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->field_of_view = value;
  self->fields |= MLN_CAMERA_OPTION_FOV;
}

gpointer maplibre_native_ffi_camera_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_camera_options_copy(
    (const MaplibreNativeFfiCameraOptions*)self
  );
}

void maplibre_native_ffi_camera_options_to_native(
  const MaplibreNativeFfiCameraOptions* self, mln_camera_options* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_camera_option_fields)self->fields;
  maplibre_native_ffi_lat_lng_to_native(&self->center, &out->center, call);
  out->center_altitude = (double)(self->center_altitude);
  maplibre_native_ffi_edge_insets_to_native(
    &self->padding, &out->padding, call
  );
  maplibre_native_ffi_screen_point_to_native(&self->anchor, &out->anchor, call);
  out->zoom = (double)(self->zoom);
  out->bearing = (double)(self->bearing);
  out->pitch = (double)(self->pitch);
  out->roll = (double)(self->roll);
  out->field_of_view = (double)(self->field_of_view);
}

MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_options_from_native(
  const mln_camera_options* in
) {
  MaplibreNativeFfiCameraOptions* self =
    maplibre_native_ffi_camera_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_CAMERA_OPTION_CENTER) != 0) {
    maplibre_native_ffi_lat_lng_read(&self->center, &in->center);
  }
  if ((in->fields & MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0) {
    self->center_altitude = (gdouble)(in->center_altitude);
  }
  if ((in->fields & MLN_CAMERA_OPTION_PADDING) != 0) {
    maplibre_native_ffi_edge_insets_read(&self->padding, &in->padding);
  }
  if ((in->fields & MLN_CAMERA_OPTION_ANCHOR) != 0) {
    maplibre_native_ffi_screen_point_read(&self->anchor, &in->anchor);
  }
  if ((in->fields & MLN_CAMERA_OPTION_ZOOM) != 0) {
    self->zoom = (gdouble)(in->zoom);
  }
  if ((in->fields & MLN_CAMERA_OPTION_BEARING) != 0) {
    self->bearing = (gdouble)(in->bearing);
  }
  if ((in->fields & MLN_CAMERA_OPTION_PITCH) != 0) {
    self->pitch = (gdouble)(in->pitch);
  }
  if ((in->fields & MLN_CAMERA_OPTION_ROLL) != 0) {
    self->roll = (gdouble)(in->roll);
  }
  if ((in->fields & MLN_CAMERA_OPTION_FOV) != 0) {
    self->field_of_view = (gdouble)(in->field_of_view);
  }
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCameraQueryResult, maplibre_native_ffi_camera_query_result,
  maplibre_native_ffi_camera_query_result_copy,
  maplibre_native_ffi_camera_query_result_free
)

MaplibreNativeFfiCameraQueryResult*
maplibre_native_ffi_camera_query_result_copy(
  const MaplibreNativeFfiCameraQueryResult* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_camera_query_result_free(
  MaplibreNativeFfiCameraQueryResult* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_camera_query_result_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_camera_query_result_copy(
    (const MaplibreNativeFfiCameraQueryResult*)self
  );
}

void maplibre_native_ffi_camera_query_result_to_native(
  const MaplibreNativeFfiCameraQueryResult* self, mln_camera_query_result* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->generation = (uint64_t)(self->generation);
  maplibre_native_ffi_camera_to_native(&self->camera, &out->camera, call);
}

void maplibre_native_ffi_camera_query_result_read(
  MaplibreNativeFfiCameraQueryResult* out, const mln_camera_query_result* in
) {
  out->generation = (guint64)(in->generation);
  maplibre_native_ffi_camera_read(&out->camera, &in->camera);
}

MaplibreNativeFfiCameraQueryResult*
maplibre_native_ffi_camera_query_result_from_native(
  const mln_camera_query_result* in
) {
  MaplibreNativeFfiCameraQueryResult* self =
    g_new0(MaplibreNativeFfiCameraQueryResult, 1);
  maplibre_native_ffi_camera_query_result_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCameraTransitionEnd,
  maplibre_native_ffi_camera_transition_end,
  maplibre_native_ffi_camera_transition_end_copy,
  maplibre_native_ffi_camera_transition_end_free
)

MaplibreNativeFfiCameraTransitionEnd*
maplibre_native_ffi_camera_transition_end_copy(
  const MaplibreNativeFfiCameraTransitionEnd* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_camera_transition_end_free(
  MaplibreNativeFfiCameraTransitionEnd* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_camera_transition_end_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_camera_transition_end_copy(
    (const MaplibreNativeFfiCameraTransitionEnd*)self
  );
}

void maplibre_native_ffi_camera_transition_end_to_native(
  const MaplibreNativeFfiCameraTransitionEnd* self,
  mln_camera_transition_end* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->outcome = (mln_camera_transition_outcome)(self->outcome);
  out->generation = (uint64_t)(self->generation);
}

void maplibre_native_ffi_camera_transition_end_read(
  MaplibreNativeFfiCameraTransitionEnd* out, const mln_camera_transition_end* in
) {
  out->outcome = (MaplibreNativeFfiCameraTransitionOutcome)(in->outcome);
  out->generation = (guint64)(in->generation);
}

MaplibreNativeFfiCameraTransitionEnd*
maplibre_native_ffi_camera_transition_end_from_native(
  const mln_camera_transition_end* in
) {
  MaplibreNativeFfiCameraTransitionEnd* self =
    g_new0(MaplibreNativeFfiCameraTransitionEnd, 1);
  maplibre_native_ffi_camera_transition_end_read(self, in);
  return self;
}

struct _MaplibreNativeFfiCameraUpdate {
  MaplibreNativeFfiCameraUpdateMode mode;
  MaplibreNativeFfiCameraOptions* camera;
  MaplibreNativeFfiAnimationOptions* animation;
  MaplibreNativeFfiGesturePhase gesture_phase;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCameraUpdate, maplibre_native_ffi_camera_update,
  maplibre_native_ffi_camera_update_copy, maplibre_native_ffi_camera_update_free
)

MaplibreNativeFfiCameraUpdate* maplibre_native_ffi_camera_update_new(void) {
  MaplibreNativeFfiCameraUpdate* self =
    g_new0(MaplibreNativeFfiCameraUpdate, 1);
  self->camera = maplibre_native_ffi_camera_options_new();
  self->animation = maplibre_native_ffi_animation_options_new();
  return self;
}

MaplibreNativeFfiCameraUpdate* maplibre_native_ffi_camera_update_copy(
  const MaplibreNativeFfiCameraUpdate* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiCameraUpdate* copy = g_memdup2(self, sizeof *self);
  copy->camera = self->camera != NULL
                   ? maplibre_native_ffi_camera_options_copy(self->camera)
                   : NULL;
  copy->animation =
    self->animation != NULL
      ? maplibre_native_ffi_animation_options_copy(self->animation)
      : NULL;
  return copy;
}

void maplibre_native_ffi_camera_update_free(
  MaplibreNativeFfiCameraUpdate* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->camera, maplibre_native_ffi_camera_options_free);
  g_clear_pointer(&self->animation, maplibre_native_ffi_animation_options_free);
  g_free(self);
}

MaplibreNativeFfiCameraUpdateMode maplibre_native_ffi_camera_update_get_mode(
  const MaplibreNativeFfiCameraUpdate* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiCameraUpdateMode)0);
  return self->mode;
}

void maplibre_native_ffi_camera_update_set_mode(
  MaplibreNativeFfiCameraUpdate* self, MaplibreNativeFfiCameraUpdateMode value
) {
  g_return_if_fail(self != NULL);
  self->mode = value;
}

MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_update_get_camera(
  const MaplibreNativeFfiCameraUpdate* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->camera;
}

void maplibre_native_ffi_camera_update_set_camera(
  MaplibreNativeFfiCameraUpdate* self,
  const MaplibreNativeFfiCameraOptions* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiCameraOptions* previous = self->camera;
  self->camera = maplibre_native_ffi_camera_options_copy(value);
  if (previous != NULL) maplibre_native_ffi_camera_options_free(previous);
}

MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_camera_update_get_animation(
  const MaplibreNativeFfiCameraUpdate* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->animation;
}

void maplibre_native_ffi_camera_update_set_animation(
  MaplibreNativeFfiCameraUpdate* self,
  const MaplibreNativeFfiAnimationOptions* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiAnimationOptions* previous = self->animation;
  self->animation = maplibre_native_ffi_animation_options_copy(value);
  if (previous != NULL) maplibre_native_ffi_animation_options_free(previous);
}

MaplibreNativeFfiGesturePhase
maplibre_native_ffi_camera_update_get_gesture_phase(
  const MaplibreNativeFfiCameraUpdate* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiGesturePhase)0);
  return self->gesture_phase;
}

void maplibre_native_ffi_camera_update_set_gesture_phase(
  MaplibreNativeFfiCameraUpdate* self, MaplibreNativeFfiGesturePhase value
) {
  g_return_if_fail(self != NULL);
  self->gesture_phase = value;
}

gpointer maplibre_native_ffi_camera_update_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_camera_update_copy(
    (const MaplibreNativeFfiCameraUpdate*)self
  );
}

void maplibre_native_ffi_camera_update_to_native(
  const MaplibreNativeFfiCameraUpdate* self, mln_camera_update* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->mode = (mln_camera_update_mode)(self->mode);
  if (self->camera != NULL)
    maplibre_native_ffi_camera_options_to_native(
      self->camera, &out->camera, call
    );
  if (self->animation != NULL)
    maplibre_native_ffi_animation_options_to_native(
      self->animation, &out->animation, call
    );
  out->gesture_phase = (mln_gesture_phase)(self->gesture_phase);
}

MaplibreNativeFfiCameraUpdate* maplibre_native_ffi_camera_update_from_native(
  const mln_camera_update* in
) {
  MaplibreNativeFfiCameraUpdate* self = maplibre_native_ffi_camera_update_new();
  self->mode = (MaplibreNativeFfiCameraUpdateMode)(in->mode);
  g_clear_pointer(&self->camera, maplibre_native_ffi_camera_options_free);
  self->camera = maplibre_native_ffi_camera_options_from_native(&in->camera);
  g_clear_pointer(&self->animation, maplibre_native_ffi_animation_options_free);
  self->animation =
    maplibre_native_ffi_animation_options_from_native(&in->animation);
  self->gesture_phase = (MaplibreNativeFfiGesturePhase)(in->gesture_phase);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCanonicalTileId, maplibre_native_ffi_canonical_tile_id,
  maplibre_native_ffi_canonical_tile_id_copy,
  maplibre_native_ffi_canonical_tile_id_free
)

MaplibreNativeFfiCanonicalTileId* maplibre_native_ffi_canonical_tile_id_copy(
  const MaplibreNativeFfiCanonicalTileId* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_canonical_tile_id_free(
  MaplibreNativeFfiCanonicalTileId* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_canonical_tile_id_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_canonical_tile_id_copy(
    (const MaplibreNativeFfiCanonicalTileId*)self
  );
}

void maplibre_native_ffi_canonical_tile_id_to_native(
  const MaplibreNativeFfiCanonicalTileId* self, mln_canonical_tile_id* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->z = (uint32_t)(self->z);
  out->x = (uint32_t)(self->x);
  out->y = (uint32_t)(self->y);
}

void maplibre_native_ffi_canonical_tile_id_read(
  MaplibreNativeFfiCanonicalTileId* out, const mln_canonical_tile_id* in
) {
  out->z = (guint32)(in->z);
  out->x = (guint32)(in->x);
  out->y = (guint32)(in->y);
}

MaplibreNativeFfiCanonicalTileId*
maplibre_native_ffi_canonical_tile_id_from_native(
  const mln_canonical_tile_id* in
) {
  MaplibreNativeFfiCanonicalTileId* self =
    g_new0(MaplibreNativeFfiCanonicalTileId, 1);
  maplibre_native_ffi_canonical_tile_id_read(self, in);
  return self;
}

struct _MaplibreNativeFfiCustomGeometrySourceOptions {
  guint64 fields;
  gdouble min_zoom;
  gdouble max_zoom;
  gdouble tolerance;
  guint32 tile_size;
  guint32 buffer;
  gboolean clip;
  gboolean wrap;
  MaplibreNativeFfiClosure* fetch_tile;
  MaplibreNativeFfiClosure* cancel_tile;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCustomGeometrySourceOptions,
  maplibre_native_ffi_custom_geometry_source_options,
  maplibre_native_ffi_custom_geometry_source_options_copy,
  maplibre_native_ffi_custom_geometry_source_options_free
)

MaplibreNativeFfiCustomGeometrySourceOptions*
maplibre_native_ffi_custom_geometry_source_options_new(void) {
  MaplibreNativeFfiCustomGeometrySourceOptions* self =
    g_new0(MaplibreNativeFfiCustomGeometrySourceOptions, 1);
  return self;
}

MaplibreNativeFfiCustomGeometrySourceOptions*
maplibre_native_ffi_custom_geometry_source_options_copy(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiCustomGeometrySourceOptions* copy =
    g_memdup2(self, sizeof *self);
  maplibre_native_ffi_closure_ref(copy->fetch_tile);
  maplibre_native_ffi_closure_ref(copy->cancel_tile);
  return copy;
}

void maplibre_native_ffi_custom_geometry_source_options_free(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  if (self == NULL) return;
  maplibre_native_ffi_closure_unref(self->fetch_tile);
  maplibre_native_ffi_closure_unref(self->cancel_tile);
  g_free(self);
}

gdouble maplibre_native_ffi_custom_geometry_source_options_get_min_zoom(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->min_zoom;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_min_zoom(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_min_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->min_zoom = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
}

gdouble maplibre_native_ffi_custom_geometry_source_options_get_max_zoom(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->max_zoom;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_max_zoom(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_max_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->max_zoom = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
}

gdouble maplibre_native_ffi_custom_geometry_source_options_get_tolerance(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->tolerance;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_tolerance(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_tolerance(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->tolerance = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
}

guint32 maplibre_native_ffi_custom_geometry_source_options_get_tile_size(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->tile_size;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_tile_size(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_tile_size(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->tile_size = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
}

guint32 maplibre_native_ffi_custom_geometry_source_options_get_buffer(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->buffer;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_buffer(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_buffer(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->buffer = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_get_clip(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->clip;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_clip(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_clip(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->clip = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_get_wrap(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->wrap;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_wrap(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_wrap(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->wrap = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
}

void maplibre_native_ffi_custom_geometry_source_options_set_fetch_tile(
  MaplibreNativeFfiCustomGeometrySourceOptions* self,
  MaplibreNativeFfiCustomSourceFetchTileCallback callback, gpointer user_data,
  GDestroyNotify notify
) {
  g_return_if_fail(self != NULL);
  MaplibreNativeFfiClosure* previous = self->fetch_tile;
  self->fetch_tile =
    callback != NULL
      ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify)
      : NULL;
  if (callback == NULL && notify != NULL) notify(user_data);
  maplibre_native_ffi_closure_unref(previous);
}

void maplibre_native_ffi_custom_geometry_source_options_set_cancel_tile(
  MaplibreNativeFfiCustomGeometrySourceOptions* self,
  MaplibreNativeFfiCustomSourceCancelTileCallback callback, gpointer user_data,
  GDestroyNotify notify
) {
  g_return_if_fail(self != NULL);
  MaplibreNativeFfiClosure* previous = self->cancel_tile;
  self->cancel_tile =
    callback != NULL
      ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify)
      : NULL;
  if (callback == NULL && notify != NULL) notify(user_data);
  maplibre_native_ffi_closure_unref(previous);
}

gpointer maplibre_native_ffi_custom_geometry_source_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_custom_geometry_source_options_copy(
    (const MaplibreNativeFfiCustomGeometrySourceOptions*)self
  );
}

static void custom_geometry_source_options_fetch_tile_trampoline(
  void* user_data, mln_map map, mln_buffer_view source_id,
  mln_canonical_tile_id tile_id
) {
  MaplibreNativeFfiClosure* closure =
    maplibre_native_ffi_registration_closure(user_data, 0);
  if (closure == NULL) return;
  MaplibreNativeFfiMap* map_ =
    maplibre_native_ffi_map_wrap(map, NULL, TRUE, NULL);
  char* source_id_ = maplibre_native_ffi_text_new(source_id);
  MaplibreNativeFfiCanonicalTileId* tile_id_ =
    maplibre_native_ffi_canonical_tile_id_from_native(&tile_id);
  ((MaplibreNativeFfiCustomSourceFetchTileCallback)(void (*)(void))
     closure->func)(map_, source_id_, tile_id_, closure->user_data);
  g_object_unref(map_);
  g_free(source_id_);
  maplibre_native_ffi_canonical_tile_id_free(tile_id_);
}

static void custom_geometry_source_options_cancel_tile_trampoline(
  void* user_data, mln_map map, mln_buffer_view source_id,
  mln_canonical_tile_id tile_id
) {
  MaplibreNativeFfiClosure* closure =
    maplibre_native_ffi_registration_closure(user_data, 1);
  if (closure == NULL) return;
  MaplibreNativeFfiMap* map_ =
    maplibre_native_ffi_map_wrap(map, NULL, TRUE, NULL);
  char* source_id_ = maplibre_native_ffi_text_new(source_id);
  MaplibreNativeFfiCanonicalTileId* tile_id_ =
    maplibre_native_ffi_canonical_tile_id_from_native(&tile_id);
  ((MaplibreNativeFfiCustomSourceCancelTileCallback)(void (*)(void))
     closure->func)(map_, source_id_, tile_id_, closure->user_data);
  g_object_unref(map_);
  g_free(source_id_);
  maplibre_native_ffi_canonical_tile_id_free(tile_id_);
}

void maplibre_native_ffi_custom_geometry_source_options_to_native(
  const MaplibreNativeFfiCustomGeometrySourceOptions* self,
  mln_custom_geometry_source_options* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_custom_geometry_source_option_fields)self->fields;
  out->min_zoom = (double)(self->min_zoom);
  out->max_zoom = (double)(self->max_zoom);
  out->tolerance = (double)(self->tolerance);
  out->tile_size = (uint32_t)(self->tile_size);
  out->buffer = (uint32_t)(self->buffer);
  out->clip = (self->clip) != FALSE;
  out->wrap = (self->wrap) != FALSE;
  if (self->fetch_tile != NULL || self->cancel_tile != NULL) {
    MaplibreNativeFfiClosure* const closures[] = {
      self->fetch_tile, self->cancel_tile
    };
    out->user_data =
      maplibre_native_ffi_call_register(call, closures, 2, &out->queue);
    out->release_user_data = maplibre_native_ffi_registration_release;
    out->fetch_tile = self->fetch_tile != NULL
                        ? custom_geometry_source_options_fetch_tile_trampoline
                        : NULL;
    out->cancel_tile = self->cancel_tile != NULL
                         ? custom_geometry_source_options_cancel_tile_trampoline
                         : NULL;
  }
}

MaplibreNativeFfiCustomGeometrySourceOptions*
maplibre_native_ffi_custom_geometry_source_options_from_native(
  const mln_custom_geometry_source_options* in
) {
  MaplibreNativeFfiCustomGeometrySourceOptions* self =
    maplibre_native_ffi_custom_geometry_source_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM) != 0) {
    self->min_zoom = (gdouble)(in->min_zoom);
  }
  if ((in->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM) != 0) {
    self->max_zoom = (gdouble)(in->max_zoom);
  }
  if ((in->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE) != 0) {
    self->tolerance = (gdouble)(in->tolerance);
  }
  if ((in->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE) != 0) {
    self->tile_size = (guint32)(in->tile_size);
  }
  if ((in->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER) != 0) {
    self->buffer = (guint32)(in->buffer);
  }
  if ((in->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP) != 0) {
    self->clip = (in->clip) ? TRUE : FALSE;
  }
  if ((in->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP) != 0) {
    self->wrap = (in->wrap) ? TRUE : FALSE;
  }
  return self;
}

struct _MaplibreNativeFfiCustomVectorSourceOptions {
  guint64 fields;
  gdouble min_zoom;
  gdouble max_zoom;
  MaplibreNativeFfiClosure* fetch_tile;
  MaplibreNativeFfiClosure* cancel_tile;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCustomVectorSourceOptions,
  maplibre_native_ffi_custom_vector_source_options,
  maplibre_native_ffi_custom_vector_source_options_copy,
  maplibre_native_ffi_custom_vector_source_options_free
)

MaplibreNativeFfiCustomVectorSourceOptions*
maplibre_native_ffi_custom_vector_source_options_new(void) {
  MaplibreNativeFfiCustomVectorSourceOptions* self =
    g_new0(MaplibreNativeFfiCustomVectorSourceOptions, 1);
  return self;
}

MaplibreNativeFfiCustomVectorSourceOptions*
maplibre_native_ffi_custom_vector_source_options_copy(
  const MaplibreNativeFfiCustomVectorSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiCustomVectorSourceOptions* copy =
    g_memdup2(self, sizeof *self);
  maplibre_native_ffi_closure_ref(copy->fetch_tile);
  maplibre_native_ffi_closure_ref(copy->cancel_tile);
  return copy;
}

void maplibre_native_ffi_custom_vector_source_options_free(
  MaplibreNativeFfiCustomVectorSourceOptions* self
) {
  if (self == NULL) return;
  maplibre_native_ffi_closure_unref(self->fetch_tile);
  maplibre_native_ffi_closure_unref(self->cancel_tile);
  g_free(self);
}

gdouble maplibre_native_ffi_custom_vector_source_options_get_min_zoom(
  const MaplibreNativeFfiCustomVectorSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->min_zoom;
}

gboolean maplibre_native_ffi_custom_vector_source_options_has_min_zoom(
  const MaplibreNativeFfiCustomVectorSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM) != 0;
}

void maplibre_native_ffi_custom_vector_source_options_set_min_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->min_zoom = value;
  self->fields |= MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM;
}

gdouble maplibre_native_ffi_custom_vector_source_options_get_max_zoom(
  const MaplibreNativeFfiCustomVectorSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->max_zoom;
}

gboolean maplibre_native_ffi_custom_vector_source_options_has_max_zoom(
  const MaplibreNativeFfiCustomVectorSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM) != 0;
}

void maplibre_native_ffi_custom_vector_source_options_set_max_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->max_zoom = value;
  self->fields |= MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM;
}

void maplibre_native_ffi_custom_vector_source_options_set_fetch_tile(
  MaplibreNativeFfiCustomVectorSourceOptions* self,
  MaplibreNativeFfiCustomSourceFetchTileCallback callback, gpointer user_data,
  GDestroyNotify notify
) {
  g_return_if_fail(self != NULL);
  MaplibreNativeFfiClosure* previous = self->fetch_tile;
  self->fetch_tile =
    callback != NULL
      ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify)
      : NULL;
  if (callback == NULL && notify != NULL) notify(user_data);
  maplibre_native_ffi_closure_unref(previous);
}

void maplibre_native_ffi_custom_vector_source_options_set_cancel_tile(
  MaplibreNativeFfiCustomVectorSourceOptions* self,
  MaplibreNativeFfiCustomSourceCancelTileCallback callback, gpointer user_data,
  GDestroyNotify notify
) {
  g_return_if_fail(self != NULL);
  MaplibreNativeFfiClosure* previous = self->cancel_tile;
  self->cancel_tile =
    callback != NULL
      ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify)
      : NULL;
  if (callback == NULL && notify != NULL) notify(user_data);
  maplibre_native_ffi_closure_unref(previous);
}

gpointer maplibre_native_ffi_custom_vector_source_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_custom_vector_source_options_copy(
    (const MaplibreNativeFfiCustomVectorSourceOptions*)self
  );
}

static void custom_vector_source_options_fetch_tile_trampoline(
  void* user_data, mln_map map, mln_buffer_view source_id,
  mln_canonical_tile_id tile_id
) {
  MaplibreNativeFfiClosure* closure =
    maplibre_native_ffi_registration_closure(user_data, 0);
  if (closure == NULL) return;
  MaplibreNativeFfiMap* map_ =
    maplibre_native_ffi_map_wrap(map, NULL, TRUE, NULL);
  char* source_id_ = maplibre_native_ffi_text_new(source_id);
  MaplibreNativeFfiCanonicalTileId* tile_id_ =
    maplibre_native_ffi_canonical_tile_id_from_native(&tile_id);
  ((MaplibreNativeFfiCustomSourceFetchTileCallback)(void (*)(void))
     closure->func)(map_, source_id_, tile_id_, closure->user_data);
  g_object_unref(map_);
  g_free(source_id_);
  maplibre_native_ffi_canonical_tile_id_free(tile_id_);
}

static void custom_vector_source_options_cancel_tile_trampoline(
  void* user_data, mln_map map, mln_buffer_view source_id,
  mln_canonical_tile_id tile_id
) {
  MaplibreNativeFfiClosure* closure =
    maplibre_native_ffi_registration_closure(user_data, 1);
  if (closure == NULL) return;
  MaplibreNativeFfiMap* map_ =
    maplibre_native_ffi_map_wrap(map, NULL, TRUE, NULL);
  char* source_id_ = maplibre_native_ffi_text_new(source_id);
  MaplibreNativeFfiCanonicalTileId* tile_id_ =
    maplibre_native_ffi_canonical_tile_id_from_native(&tile_id);
  ((MaplibreNativeFfiCustomSourceCancelTileCallback)(void (*)(void))
     closure->func)(map_, source_id_, tile_id_, closure->user_data);
  g_object_unref(map_);
  g_free(source_id_);
  maplibre_native_ffi_canonical_tile_id_free(tile_id_);
}

void maplibre_native_ffi_custom_vector_source_options_to_native(
  const MaplibreNativeFfiCustomVectorSourceOptions* self,
  mln_custom_vector_source_options* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_custom_vector_source_option_fields)self->fields;
  out->min_zoom = (double)(self->min_zoom);
  out->max_zoom = (double)(self->max_zoom);
  if (self->fetch_tile != NULL || self->cancel_tile != NULL) {
    MaplibreNativeFfiClosure* const closures[] = {
      self->fetch_tile, self->cancel_tile
    };
    out->user_data =
      maplibre_native_ffi_call_register(call, closures, 2, &out->queue);
    out->release_user_data = maplibre_native_ffi_registration_release;
    out->fetch_tile = self->fetch_tile != NULL
                        ? custom_vector_source_options_fetch_tile_trampoline
                        : NULL;
    out->cancel_tile = self->cancel_tile != NULL
                         ? custom_vector_source_options_cancel_tile_trampoline
                         : NULL;
  }
}

MaplibreNativeFfiCustomVectorSourceOptions*
maplibre_native_ffi_custom_vector_source_options_from_native(
  const mln_custom_vector_source_options* in
) {
  MaplibreNativeFfiCustomVectorSourceOptions* self =
    maplibre_native_ffi_custom_vector_source_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM) != 0) {
    self->min_zoom = (gdouble)(in->min_zoom);
  }
  if ((in->fields & MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM) != 0) {
    self->max_zoom = (gdouble)(in->max_zoom);
  }
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiDmaBufFormat, maplibre_native_ffi_dma_buf_format,
  maplibre_native_ffi_dma_buf_format_copy,
  maplibre_native_ffi_dma_buf_format_free
)

MaplibreNativeFfiDmaBufFormat* maplibre_native_ffi_dma_buf_format_copy(
  const MaplibreNativeFfiDmaBufFormat* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_dma_buf_format_free(
  MaplibreNativeFfiDmaBufFormat* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_dma_buf_format_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_dma_buf_format_copy(
    (const MaplibreNativeFfiDmaBufFormat*)self
  );
}

void maplibre_native_ffi_dma_buf_format_to_native(
  const MaplibreNativeFfiDmaBufFormat* self, mln_dma_buf_format* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->fourcc = (uint32_t)(self->fourcc);
  out->modifier = (uint64_t)(self->modifier);
}

void maplibre_native_ffi_dma_buf_format_read(
  MaplibreNativeFfiDmaBufFormat* out, const mln_dma_buf_format* in
) {
  out->fourcc = (guint32)(in->fourcc);
  out->modifier = (guint64)(in->modifier);
}

MaplibreNativeFfiDmaBufFormat* maplibre_native_ffi_dma_buf_format_from_native(
  const mln_dma_buf_format* in
) {
  MaplibreNativeFfiDmaBufFormat* self =
    g_new0(MaplibreNativeFfiDmaBufFormat, 1);
  maplibre_native_ffi_dma_buf_format_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiDmaBufPlane, maplibre_native_ffi_dma_buf_plane,
  maplibre_native_ffi_dma_buf_plane_copy, maplibre_native_ffi_dma_buf_plane_free
)

MaplibreNativeFfiDmaBufPlane* maplibre_native_ffi_dma_buf_plane_copy(
  const MaplibreNativeFfiDmaBufPlane* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_dma_buf_plane_free(
  MaplibreNativeFfiDmaBufPlane* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_dma_buf_plane_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_dma_buf_plane_copy(
    (const MaplibreNativeFfiDmaBufPlane*)self
  );
}

void maplibre_native_ffi_dma_buf_plane_to_native(
  const MaplibreNativeFfiDmaBufPlane* self, mln_dma_buf_plane* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->fd = (int32_t)(self->fd);
  out->offset = (uint32_t)(self->offset);
  out->stride = (uint32_t)(self->stride);
}

void maplibre_native_ffi_dma_buf_plane_read(
  MaplibreNativeFfiDmaBufPlane* out, const mln_dma_buf_plane* in
) {
  out->fd = (gint32)(in->fd);
  out->offset = (guint32)(in->offset);
  out->stride = (guint32)(in->stride);
}

MaplibreNativeFfiDmaBufPlane* maplibre_native_ffi_dma_buf_plane_from_native(
  const mln_dma_buf_plane* in
) {
  MaplibreNativeFfiDmaBufPlane* self = g_new0(MaplibreNativeFfiDmaBufPlane, 1);
  maplibre_native_ffi_dma_buf_plane_read(self, in);
  return self;
}

struct _MaplibreNativeFfiDmaBufFrame {
  guint32 width;
  guint32 height;
  gdouble scale_factor;
  guint32 slot;
  guint32 fourcc;
  guint64 modifier;
  GPtrArray* planes;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiDmaBufFrame, maplibre_native_ffi_dma_buf_frame,
  maplibre_native_ffi_dma_buf_frame_copy, maplibre_native_ffi_dma_buf_frame_free
)

MaplibreNativeFfiDmaBufFrame* maplibre_native_ffi_dma_buf_frame_new(void) {
  MaplibreNativeFfiDmaBufFrame* self = g_new0(MaplibreNativeFfiDmaBufFrame, 1);
  self->planes = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_dma_buf_plane_free
  );
  return self;
}

MaplibreNativeFfiDmaBufFrame* maplibre_native_ffi_dma_buf_frame_copy(
  const MaplibreNativeFfiDmaBufFrame* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiDmaBufFrame* copy = g_memdup2(self, sizeof *self);
  copy->planes =
    self->planes != NULL
      ? g_ptr_array_copy(
          self->planes, maplibre_native_ffi_dma_buf_plane_copy_func, NULL
        )
      : NULL;
  if (copy->planes != NULL)
    g_ptr_array_set_free_func(
      copy->planes, (GDestroyNotify)maplibre_native_ffi_dma_buf_plane_free
    );
  return copy;
}

void maplibre_native_ffi_dma_buf_frame_free(
  MaplibreNativeFfiDmaBufFrame* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->planes, g_ptr_array_unref);
  g_free(self);
}

guint32 maplibre_native_ffi_dma_buf_frame_get_width(
  const MaplibreNativeFfiDmaBufFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->width;
}

guint32 maplibre_native_ffi_dma_buf_frame_get_height(
  const MaplibreNativeFfiDmaBufFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->height;
}

gdouble maplibre_native_ffi_dma_buf_frame_get_scale_factor(
  const MaplibreNativeFfiDmaBufFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->scale_factor;
}

guint32 maplibre_native_ffi_dma_buf_frame_get_slot(
  const MaplibreNativeFfiDmaBufFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->slot;
}

guint32 maplibre_native_ffi_dma_buf_frame_get_fourcc(
  const MaplibreNativeFfiDmaBufFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->fourcc;
}

guint64 maplibre_native_ffi_dma_buf_frame_get_modifier(
  const MaplibreNativeFfiDmaBufFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->modifier;
}

GPtrArray* maplibre_native_ffi_dma_buf_frame_get_planes(
  const MaplibreNativeFfiDmaBufFrame* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->planes;
}

gpointer maplibre_native_ffi_dma_buf_frame_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_dma_buf_frame_copy(
    (const MaplibreNativeFfiDmaBufFrame*)self
  );
}

void maplibre_native_ffi_dma_buf_frame_to_native(
  const MaplibreNativeFfiDmaBufFrame* self, mln_dma_buf_frame* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->width = (uint32_t)(self->width);
  out->height = (uint32_t)(self->height);
  out->scale_factor = (double)(self->scale_factor);
  out->slot = (uint32_t)(self->slot);
  out->fourcc = (uint32_t)(self->fourcc);
  out->modifier = (uint64_t)(self->modifier);
  if (self->planes != NULL && self->planes->len > 0) {
    mln_dma_buf_plane* items =
      maplibre_native_ffi_call_alloc(call, self->planes->len, sizeof *items);
    for (guint index = 0; index < self->planes->len; index++) {
      maplibre_native_ffi_dma_buf_plane_to_native(
        self->planes->pdata[index], &items[index], call
      );
    }
    out->planes = items;
    out->plane_count = self->planes->len;
  }
}

MaplibreNativeFfiDmaBufFrame* maplibre_native_ffi_dma_buf_frame_from_native(
  const mln_dma_buf_frame* in
) {
  MaplibreNativeFfiDmaBufFrame* self = maplibre_native_ffi_dma_buf_frame_new();
  self->width = (guint32)(in->width);
  self->height = (guint32)(in->height);
  self->scale_factor = (gdouble)(in->scale_factor);
  self->slot = (guint32)(in->slot);
  self->fourcc = (guint32)(in->fourcc);
  self->modifier = (guint64)(in->modifier);
  g_clear_pointer(&self->planes, g_ptr_array_unref);
  self->planes = g_ptr_array_new_full(
    (guint)in->plane_count,
    (GDestroyNotify)maplibre_native_ffi_dma_buf_plane_free
  );
  for (size_t index = 0; index < in->plane_count; index++) {
    g_ptr_array_add(
      self->planes,
      maplibre_native_ffi_dma_buf_plane_from_native(&in->planes[index])
    );
  }
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiLogicalSize, maplibre_native_ffi_logical_size,
  maplibre_native_ffi_logical_size_copy, maplibre_native_ffi_logical_size_free
)

MaplibreNativeFfiLogicalSize* maplibre_native_ffi_logical_size_copy(
  const MaplibreNativeFfiLogicalSize* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_logical_size_free(MaplibreNativeFfiLogicalSize* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_logical_size_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_logical_size_copy(
    (const MaplibreNativeFfiLogicalSize*)self
  );
}

void maplibre_native_ffi_logical_size_to_native(
  const MaplibreNativeFfiLogicalSize* self, mln_logical_size* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->width = (uint32_t)(self->width);
  out->height = (uint32_t)(self->height);
}

void maplibre_native_ffi_logical_size_read(
  MaplibreNativeFfiLogicalSize* out, const mln_logical_size* in
) {
  out->width = (guint32)(in->width);
  out->height = (guint32)(in->height);
}

MaplibreNativeFfiLogicalSize* maplibre_native_ffi_logical_size_from_native(
  const mln_logical_size* in
) {
  MaplibreNativeFfiLogicalSize* self = g_new0(MaplibreNativeFfiLogicalSize, 1);
  maplibre_native_ffi_logical_size_read(self, in);
  return self;
}

struct _MaplibreNativeFfiDmaBufRingDescriptor {
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 buffer_count;
  GPtrArray* formats;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiDmaBufRingDescriptor,
  maplibre_native_ffi_dma_buf_ring_descriptor,
  maplibre_native_ffi_dma_buf_ring_descriptor_copy,
  maplibre_native_ffi_dma_buf_ring_descriptor_free
)

MaplibreNativeFfiDmaBufRingDescriptor*
maplibre_native_ffi_dma_buf_ring_descriptor_new(void) {
  MaplibreNativeFfiDmaBufRingDescriptor* self =
    g_new0(MaplibreNativeFfiDmaBufRingDescriptor, 1);
  self->formats = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_dma_buf_format_free
  );
  return self;
}

MaplibreNativeFfiDmaBufRingDescriptor*
maplibre_native_ffi_dma_buf_ring_descriptor_copy(
  const MaplibreNativeFfiDmaBufRingDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiDmaBufRingDescriptor* copy = g_memdup2(self, sizeof *self);
  copy->formats =
    self->formats != NULL
      ? g_ptr_array_copy(
          self->formats, maplibre_native_ffi_dma_buf_format_copy_func, NULL
        )
      : NULL;
  if (copy->formats != NULL)
    g_ptr_array_set_free_func(
      copy->formats, (GDestroyNotify)maplibre_native_ffi_dma_buf_format_free
    );
  return copy;
}

void maplibre_native_ffi_dma_buf_ring_descriptor_free(
  MaplibreNativeFfiDmaBufRingDescriptor* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->formats, g_ptr_array_unref);
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_dma_buf_ring_descriptor_get_logical_size(
  const MaplibreNativeFfiDmaBufRingDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_dma_buf_ring_descriptor_set_logical_size(
  MaplibreNativeFfiDmaBufRingDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

guint32 maplibre_native_ffi_dma_buf_ring_descriptor_get_buffer_count(
  const MaplibreNativeFfiDmaBufRingDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->buffer_count;
}

void maplibre_native_ffi_dma_buf_ring_descriptor_set_buffer_count(
  MaplibreNativeFfiDmaBufRingDescriptor* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->buffer_count = value;
}

GPtrArray* maplibre_native_ffi_dma_buf_ring_descriptor_get_formats(
  const MaplibreNativeFfiDmaBufRingDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->formats;
}

void maplibre_native_ffi_dma_buf_ring_descriptor_set_formats(
  MaplibreNativeFfiDmaBufRingDescriptor* self,
  const MaplibreNativeFfiDmaBufFormat* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->formats;
  self->formats = g_ptr_array_new_full(
    (guint)value_count, (GDestroyNotify)maplibre_native_ffi_dma_buf_format_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->formats, maplibre_native_ffi_dma_buf_format_copy(&value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

gpointer maplibre_native_ffi_dma_buf_ring_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_dma_buf_ring_descriptor_copy(
    (const MaplibreNativeFfiDmaBufRingDescriptor*)self
  );
}

void maplibre_native_ffi_dma_buf_ring_descriptor_to_native(
  const MaplibreNativeFfiDmaBufRingDescriptor* self,
  mln_dma_buf_ring_descriptor* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  out->buffer_count = (uint32_t)(self->buffer_count);
  if (self->formats != NULL && self->formats->len > 0) {
    mln_dma_buf_format* items =
      maplibre_native_ffi_call_alloc(call, self->formats->len, sizeof *items);
    for (guint index = 0; index < self->formats->len; index++) {
      maplibre_native_ffi_dma_buf_format_to_native(
        self->formats->pdata[index], &items[index], call
      );
    }
    out->formats = items;
    out->format_count = self->formats->len;
  }
}

MaplibreNativeFfiDmaBufRingDescriptor*
maplibre_native_ffi_dma_buf_ring_descriptor_from_native(
  const mln_dma_buf_ring_descriptor* in
) {
  MaplibreNativeFfiDmaBufRingDescriptor* self =
    maplibre_native_ffi_dma_buf_ring_descriptor_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  self->buffer_count = (guint32)(in->buffer_count);
  g_clear_pointer(&self->formats, g_ptr_array_unref);
  self->formats = g_ptr_array_new_full(
    (guint)in->format_count,
    (GDestroyNotify)maplibre_native_ffi_dma_buf_format_free
  );
  for (size_t index = 0; index < in->format_count; index++) {
    g_ptr_array_add(
      self->formats,
      maplibre_native_ffi_dma_buf_format_from_native(&in->formats[index])
    );
  }
  return self;
}

struct _MaplibreNativeFfiEglContextDescriptor {
  gpointer display;
  gpointer config;
  gpointer share_context;
  MaplibreNativeFfiOpenGlClientApi client_api;
  gpointer get_proc_address;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiEglContextDescriptor,
  maplibre_native_ffi_egl_context_descriptor,
  maplibre_native_ffi_egl_context_descriptor_copy,
  maplibre_native_ffi_egl_context_descriptor_free
)

MaplibreNativeFfiEglContextDescriptor*
maplibre_native_ffi_egl_context_descriptor_new(void) {
  MaplibreNativeFfiEglContextDescriptor* self =
    g_new0(MaplibreNativeFfiEglContextDescriptor, 1);
  return self;
}

MaplibreNativeFfiEglContextDescriptor*
maplibre_native_ffi_egl_context_descriptor_copy(
  const MaplibreNativeFfiEglContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiEglContextDescriptor* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_egl_context_descriptor_free(
  MaplibreNativeFfiEglContextDescriptor* self
) {
  if (self == NULL) return;
  g_free(self);
}

gpointer maplibre_native_ffi_egl_context_descriptor_get_display(
  const MaplibreNativeFfiEglContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->display;
}

void maplibre_native_ffi_egl_context_descriptor_set_display(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->display = value;
}

gpointer maplibre_native_ffi_egl_context_descriptor_get_config(
  const MaplibreNativeFfiEglContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->config;
}

void maplibre_native_ffi_egl_context_descriptor_set_config(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->config = value;
}

gpointer maplibre_native_ffi_egl_context_descriptor_get_share_context(
  const MaplibreNativeFfiEglContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->share_context;
}

void maplibre_native_ffi_egl_context_descriptor_set_share_context(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->share_context = value;
}

MaplibreNativeFfiOpenGlClientApi
maplibre_native_ffi_egl_context_descriptor_get_client_api(
  const MaplibreNativeFfiEglContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiOpenGlClientApi)0);
  return self->client_api;
}

void maplibre_native_ffi_egl_context_descriptor_set_client_api(
  MaplibreNativeFfiEglContextDescriptor* self,
  MaplibreNativeFfiOpenGlClientApi value
) {
  g_return_if_fail(self != NULL);
  self->client_api = value;
}

gpointer maplibre_native_ffi_egl_context_descriptor_get_get_proc_address(
  const MaplibreNativeFfiEglContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->get_proc_address;
}

void maplibre_native_ffi_egl_context_descriptor_set_get_proc_address(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->get_proc_address = value;
}

gpointer maplibre_native_ffi_egl_context_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_egl_context_descriptor_copy(
    (const MaplibreNativeFfiEglContextDescriptor*)self
  );
}

void maplibre_native_ffi_egl_context_descriptor_to_native(
  const MaplibreNativeFfiEglContextDescriptor* self,
  mln_egl_context_descriptor* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->display = (void*)(self->display);
  out->config = (void*)(self->config);
  out->share_context = (void*)(self->share_context);
  out->client_api = (mln_open_gl_client_api)(self->client_api);
  out->get_proc_address = (void*)(self->get_proc_address);
}

MaplibreNativeFfiEglContextDescriptor*
maplibre_native_ffi_egl_context_descriptor_from_native(
  const mln_egl_context_descriptor* in
) {
  MaplibreNativeFfiEglContextDescriptor* self =
    maplibre_native_ffi_egl_context_descriptor_new();
  self->display = (gpointer)(in->display);
  self->config = (gpointer)(in->config);
  self->share_context = (gpointer)(in->share_context);
  self->client_api = (MaplibreNativeFfiOpenGlClientApi)(in->client_api);
  self->get_proc_address = (gpointer)(in->get_proc_address);
  return self;
}

struct _MaplibreNativeFfiFeatureId {
  gint64 tag;
  char* string_value;
  guint64 uint_value;
  gint64 int_value;
  gdouble double_value;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiFeatureId, maplibre_native_ffi_feature_id,
  maplibre_native_ffi_feature_id_copy, maplibre_native_ffi_feature_id_free
)

MaplibreNativeFfiFeatureId* maplibre_native_ffi_feature_id_copy(
  const MaplibreNativeFfiFeatureId* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiFeatureId* copy = g_memdup2(self, sizeof *self);
  copy->string_value = g_strdup(self->string_value);
  return copy;
}

void maplibre_native_ffi_feature_id_free(MaplibreNativeFfiFeatureId* self) {
  if (self == NULL) return;
  g_free(self->string_value);
  g_free(self);
}

MaplibreNativeFfiFeatureId* maplibre_native_ffi_feature_id_new_string_value(
  const char* value
) {
  MaplibreNativeFfiFeatureId* self = g_new0(MaplibreNativeFfiFeatureId, 1);
  self->tag = MLN_FEATURE_ID_TYPE_STRING;
  self->string_value = g_strdup("");
  char* previous = self->string_value;
  self->string_value = g_strdup(value != NULL ? value : "");
  g_free(previous);
  return self;
}

gboolean maplibre_native_ffi_feature_id_is_string_value(
  const MaplibreNativeFfiFeatureId* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_FEATURE_ID_TYPE_STRING;
}

const char* maplibre_native_ffi_feature_id_get_string_value(
  const MaplibreNativeFfiFeatureId* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(self->tag == MLN_FEATURE_ID_TYPE_STRING, NULL);
  return self->string_value;
}

MaplibreNativeFfiFeatureId* maplibre_native_ffi_feature_id_new_uint_value(
  guint64 value
) {
  MaplibreNativeFfiFeatureId* self = g_new0(MaplibreNativeFfiFeatureId, 1);
  self->tag = MLN_FEATURE_ID_TYPE_UINT;
  self->uint_value = value;
  return self;
}

gboolean maplibre_native_ffi_feature_id_is_uint_value(
  const MaplibreNativeFfiFeatureId* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_FEATURE_ID_TYPE_UINT;
}

guint64 maplibre_native_ffi_feature_id_get_uint_value(
  const MaplibreNativeFfiFeatureId* self
) {
  g_return_val_if_fail(self != NULL, 0);
  g_return_val_if_fail(self->tag == MLN_FEATURE_ID_TYPE_UINT, 0);
  return self->uint_value;
}

MaplibreNativeFfiFeatureId* maplibre_native_ffi_feature_id_new_int_value(
  gint64 value
) {
  MaplibreNativeFfiFeatureId* self = g_new0(MaplibreNativeFfiFeatureId, 1);
  self->tag = MLN_FEATURE_ID_TYPE_INT;
  self->int_value = value;
  return self;
}

gboolean maplibre_native_ffi_feature_id_is_int_value(
  const MaplibreNativeFfiFeatureId* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_FEATURE_ID_TYPE_INT;
}

gint64 maplibre_native_ffi_feature_id_get_int_value(
  const MaplibreNativeFfiFeatureId* self
) {
  g_return_val_if_fail(self != NULL, 0);
  g_return_val_if_fail(self->tag == MLN_FEATURE_ID_TYPE_INT, 0);
  return self->int_value;
}

MaplibreNativeFfiFeatureId* maplibre_native_ffi_feature_id_new_double_value(
  gdouble value
) {
  MaplibreNativeFfiFeatureId* self = g_new0(MaplibreNativeFfiFeatureId, 1);
  self->tag = MLN_FEATURE_ID_TYPE_DOUBLE;
  self->double_value = value;
  return self;
}

gboolean maplibre_native_ffi_feature_id_is_double_value(
  const MaplibreNativeFfiFeatureId* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_FEATURE_ID_TYPE_DOUBLE;
}

gdouble maplibre_native_ffi_feature_id_get_double_value(
  const MaplibreNativeFfiFeatureId* self
) {
  g_return_val_if_fail(self != NULL, 0);
  g_return_val_if_fail(self->tag == MLN_FEATURE_ID_TYPE_DOUBLE, 0);
  return self->double_value;
}

void maplibre_native_ffi_feature_id_to_native(
  const MaplibreNativeFfiFeatureId* self, mln_feature_id* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->type = (mln_feature_id_type)self->tag;
  mln_feature_id_data* data = &out->data;
  switch (self->tag) {
    case MLN_FEATURE_ID_TYPE_STRING:
      data->string_value = maplibre_native_ffi_text_view(self->string_value);
      break;
    case MLN_FEATURE_ID_TYPE_UINT:
      data->uint_value = (uint64_t)(self->uint_value);
      break;
    case MLN_FEATURE_ID_TYPE_INT:
      data->int_value = (int64_t)(self->int_value);
      break;
    case MLN_FEATURE_ID_TYPE_DOUBLE:
      data->double_value = (double)(self->double_value);
      break;
    default:
      break;
  }
}

MaplibreNativeFfiFeatureId* maplibre_native_ffi_feature_id_from_native(
  const mln_feature_id* in
) {
  gint64 tag = (gint64)in->type;
  const mln_feature_id_data* data = &in->data;
  MaplibreNativeFfiFeatureId* self = g_new0(MaplibreNativeFfiFeatureId, 1);
  self->tag = tag;
  switch (tag) {
    case MLN_FEATURE_ID_TYPE_STRING:
      self->string_value = maplibre_native_ffi_text_new(data->string_value);
      break;
    case MLN_FEATURE_ID_TYPE_UINT:
      self->uint_value = (guint64)(data->uint_value);
      break;
    case MLN_FEATURE_ID_TYPE_INT:
      self->int_value = (gint64)(data->int_value);
      break;
    case MLN_FEATURE_ID_TYPE_DOUBLE:
      self->double_value = (gdouble)(data->double_value);
      break;
    default:
      break;
  }
  return self;
}

gpointer maplibre_native_ffi_feature_id_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_feature_id_copy(
    (const MaplibreNativeFfiFeatureId*)self
  );
}

struct _MaplibreNativeFfiFeatureStateSelector {
  guint64 fields;
  char* source_id;
  char* source_layer_id;
  MaplibreNativeFfiFeatureId* feature_id;
  char* state_key;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiFeatureStateSelector,
  maplibre_native_ffi_feature_state_selector,
  maplibre_native_ffi_feature_state_selector_copy,
  maplibre_native_ffi_feature_state_selector_free
)

MaplibreNativeFfiFeatureStateSelector*
maplibre_native_ffi_feature_state_selector_new(void) {
  MaplibreNativeFfiFeatureStateSelector* self =
    g_new0(MaplibreNativeFfiFeatureStateSelector, 1);
  self->source_id = g_strdup("");
  self->source_layer_id = g_strdup("");
  self->state_key = g_strdup("");
  return self;
}

MaplibreNativeFfiFeatureStateSelector*
maplibre_native_ffi_feature_state_selector_copy(
  const MaplibreNativeFfiFeatureStateSelector* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiFeatureStateSelector* copy = g_memdup2(self, sizeof *self);
  copy->source_id = g_strdup(self->source_id);
  copy->source_layer_id = g_strdup(self->source_layer_id);
  copy->feature_id = self->feature_id != NULL
                       ? maplibre_native_ffi_feature_id_copy(self->feature_id)
                       : NULL;
  copy->state_key = g_strdup(self->state_key);
  return copy;
}

void maplibre_native_ffi_feature_state_selector_free(
  MaplibreNativeFfiFeatureStateSelector* self
) {
  if (self == NULL) return;
  g_free(self->source_id);
  g_free(self->source_layer_id);
  g_clear_pointer(&self->feature_id, maplibre_native_ffi_feature_id_free);
  g_free(self->state_key);
  g_free(self);
}

const char* maplibre_native_ffi_feature_state_selector_get_source_id(
  const MaplibreNativeFfiFeatureStateSelector* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->source_id;
}

void maplibre_native_ffi_feature_state_selector_set_source_id(
  MaplibreNativeFfiFeatureStateSelector* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->source_id;
  self->source_id = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

const char* maplibre_native_ffi_feature_state_selector_get_source_layer_id(
  const MaplibreNativeFfiFeatureStateSelector* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->source_layer_id;
}

void maplibre_native_ffi_feature_state_selector_set_source_layer_id(
  MaplibreNativeFfiFeatureStateSelector* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->source_layer_id;
  self->source_layer_id = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

MaplibreNativeFfiFeatureId*
maplibre_native_ffi_feature_state_selector_get_feature_id(
  const MaplibreNativeFfiFeatureStateSelector* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->feature_id;
}

gboolean maplibre_native_ffi_feature_state_selector_has_feature_id(
  const MaplibreNativeFfiFeatureStateSelector* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_FEATURE_STATE_SELECTOR_FEATURE_ID) != 0;
}

void maplibre_native_ffi_feature_state_selector_set_feature_id(
  MaplibreNativeFfiFeatureStateSelector* self,
  const MaplibreNativeFfiFeatureId* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiFeatureId* previous = self->feature_id;
  self->feature_id = maplibre_native_ffi_feature_id_copy(value);
  if (previous != NULL) maplibre_native_ffi_feature_id_free(previous);
  self->fields |= MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
}

const char* maplibre_native_ffi_feature_state_selector_get_state_key(
  const MaplibreNativeFfiFeatureStateSelector* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->state_key;
}

void maplibre_native_ffi_feature_state_selector_set_state_key(
  MaplibreNativeFfiFeatureStateSelector* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->state_key;
  self->state_key = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

gpointer maplibre_native_ffi_feature_state_selector_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_feature_state_selector_copy(
    (const MaplibreNativeFfiFeatureStateSelector*)self
  );
}

void maplibre_native_ffi_feature_state_selector_to_native(
  const MaplibreNativeFfiFeatureStateSelector* self,
  mln_feature_state_selector* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_feature_state_selector_fields)self->fields;
  out->source_id = maplibre_native_ffi_text_view(self->source_id);
  out->source_layer_id = maplibre_native_ffi_text_view(self->source_layer_id);
  if (self->feature_id != NULL)
    maplibre_native_ffi_feature_id_to_native(
      self->feature_id, &out->feature_id, call
    );
  out->state_key = maplibre_native_ffi_text_view(self->state_key);
}

MaplibreNativeFfiFeatureStateSelector*
maplibre_native_ffi_feature_state_selector_from_native(
  const mln_feature_state_selector* in
) {
  MaplibreNativeFfiFeatureStateSelector* self =
    maplibre_native_ffi_feature_state_selector_new();
  self->fields = (guint64)in->fields;
  g_free(self->source_id);
  self->source_id = maplibre_native_ffi_text_new(in->source_id);
  g_free(self->source_layer_id);
  self->source_layer_id = maplibre_native_ffi_text_new(in->source_layer_id);
  if ((in->fields & MLN_FEATURE_STATE_SELECTOR_FEATURE_ID) != 0) {
    g_clear_pointer(&self->feature_id, maplibre_native_ffi_feature_id_free);
    self->feature_id =
      maplibre_native_ffi_feature_id_from_native(&in->feature_id);
  }
  g_free(self->state_key);
  self->state_key = maplibre_native_ffi_text_new(in->state_key);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiFrameDemand, maplibre_native_ffi_frame_demand,
  maplibre_native_ffi_frame_demand_copy, maplibre_native_ffi_frame_demand_free
)

MaplibreNativeFfiFrameDemand* maplibre_native_ffi_frame_demand_copy(
  const MaplibreNativeFfiFrameDemand* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_frame_demand_free(MaplibreNativeFfiFrameDemand* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_frame_demand_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_frame_demand_copy(
    (const MaplibreNativeFfiFrameDemand*)self
  );
}

void maplibre_native_ffi_frame_demand_to_native(
  const MaplibreNativeFfiFrameDemand* self, mln_frame_demand* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->flags = (mln_frame_demand_flags)(self->flags);
  out->token = (uint64_t)(self->token);
}

void maplibre_native_ffi_frame_demand_read(
  MaplibreNativeFfiFrameDemand* out, const mln_frame_demand* in
) {
  out->flags = (MaplibreNativeFfiFrameDemandFlag)(in->flags);
  out->token = (guint64)(in->token);
}

MaplibreNativeFfiFrameDemand* maplibre_native_ffi_frame_demand_from_native(
  const mln_frame_demand* in
) {
  MaplibreNativeFfiFrameDemand* self = g_new0(MaplibreNativeFfiFrameDemand, 1);
  maplibre_native_ffi_frame_demand_read(self, in);
  return self;
}

struct _MaplibreNativeFfiFramePacerOptions {
  guint64 fields;
  guint32 maximum_frames_per_second;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiFramePacerOptions, maplibre_native_ffi_frame_pacer_options,
  maplibre_native_ffi_frame_pacer_options_copy,
  maplibre_native_ffi_frame_pacer_options_free
)

MaplibreNativeFfiFramePacerOptions* maplibre_native_ffi_frame_pacer_options_new(
  void
) {
  MaplibreNativeFfiFramePacerOptions* self =
    g_new0(MaplibreNativeFfiFramePacerOptions, 1);
  return self;
}

MaplibreNativeFfiFramePacerOptions*
maplibre_native_ffi_frame_pacer_options_copy(
  const MaplibreNativeFfiFramePacerOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiFramePacerOptions* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_frame_pacer_options_free(
  MaplibreNativeFfiFramePacerOptions* self
) {
  if (self == NULL) return;
  g_free(self);
}

guint32 maplibre_native_ffi_frame_pacer_options_get_maximum_frames_per_second(
  const MaplibreNativeFfiFramePacerOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->maximum_frames_per_second;
}

gboolean maplibre_native_ffi_frame_pacer_options_has_maximum_frames_per_second(
  const MaplibreNativeFfiFramePacerOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND) != 0;
}

void maplibre_native_ffi_frame_pacer_options_set_maximum_frames_per_second(
  MaplibreNativeFfiFramePacerOptions* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->maximum_frames_per_second = value;
  self->fields |= MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND;
}

gpointer maplibre_native_ffi_frame_pacer_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_frame_pacer_options_copy(
    (const MaplibreNativeFfiFramePacerOptions*)self
  );
}

void maplibre_native_ffi_frame_pacer_options_to_native(
  const MaplibreNativeFfiFramePacerOptions* self, mln_frame_pacer_options* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_frame_pacer_option_fields)self->fields;
  out->maximum_frames_per_second = (uint32_t)(self->maximum_frames_per_second);
}

MaplibreNativeFfiFramePacerOptions*
maplibre_native_ffi_frame_pacer_options_from_native(
  const mln_frame_pacer_options* in
) {
  MaplibreNativeFfiFramePacerOptions* self =
    maplibre_native_ffi_frame_pacer_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND) != 0) {
    self->maximum_frames_per_second = (guint32)(in->maximum_frames_per_second);
  }
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiVec3, maplibre_native_ffi_vec3,
  maplibre_native_ffi_vec3_copy, maplibre_native_ffi_vec3_free
)

MaplibreNativeFfiVec3* maplibre_native_ffi_vec3_copy(
  const MaplibreNativeFfiVec3* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_vec3_free(MaplibreNativeFfiVec3* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_vec3_copy_func(gconstpointer self, gpointer data) {
  (void)data;
  return maplibre_native_ffi_vec3_copy((const MaplibreNativeFfiVec3*)self);
}

void maplibre_native_ffi_vec3_to_native(
  const MaplibreNativeFfiVec3* self, mln_vec3* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->x = (double)(self->x);
  out->y = (double)(self->y);
  out->z = (double)(self->z);
}

void maplibre_native_ffi_vec3_read(
  MaplibreNativeFfiVec3* out, const mln_vec3* in
) {
  out->x = (gdouble)(in->x);
  out->y = (gdouble)(in->y);
  out->z = (gdouble)(in->z);
}

MaplibreNativeFfiVec3* maplibre_native_ffi_vec3_from_native(
  const mln_vec3* in
) {
  MaplibreNativeFfiVec3* self = g_new0(MaplibreNativeFfiVec3, 1);
  maplibre_native_ffi_vec3_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiQuaternion, maplibre_native_ffi_quaternion,
  maplibre_native_ffi_quaternion_copy, maplibre_native_ffi_quaternion_free
)

MaplibreNativeFfiQuaternion* maplibre_native_ffi_quaternion_copy(
  const MaplibreNativeFfiQuaternion* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_quaternion_free(MaplibreNativeFfiQuaternion* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_quaternion_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_quaternion_copy(
    (const MaplibreNativeFfiQuaternion*)self
  );
}

void maplibre_native_ffi_quaternion_to_native(
  const MaplibreNativeFfiQuaternion* self, mln_quaternion* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->x = (double)(self->x);
  out->y = (double)(self->y);
  out->z = (double)(self->z);
  out->w = (double)(self->w);
}

void maplibre_native_ffi_quaternion_read(
  MaplibreNativeFfiQuaternion* out, const mln_quaternion* in
) {
  out->x = (gdouble)(in->x);
  out->y = (gdouble)(in->y);
  out->z = (gdouble)(in->z);
  out->w = (gdouble)(in->w);
}

MaplibreNativeFfiQuaternion* maplibre_native_ffi_quaternion_from_native(
  const mln_quaternion* in
) {
  MaplibreNativeFfiQuaternion* self = g_new0(MaplibreNativeFfiQuaternion, 1);
  maplibre_native_ffi_quaternion_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiFreeCamera, maplibre_native_ffi_free_camera,
  maplibre_native_ffi_free_camera_copy, maplibre_native_ffi_free_camera_free
)

MaplibreNativeFfiFreeCamera* maplibre_native_ffi_free_camera_copy(
  const MaplibreNativeFfiFreeCamera* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_free_camera_free(MaplibreNativeFfiFreeCamera* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_free_camera_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_free_camera_copy(
    (const MaplibreNativeFfiFreeCamera*)self
  );
}

void maplibre_native_ffi_free_camera_to_native(
  const MaplibreNativeFfiFreeCamera* self, mln_free_camera* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  maplibre_native_ffi_vec3_to_native(&self->position, &out->position, call);
  maplibre_native_ffi_quaternion_to_native(
    &self->orientation, &out->orientation, call
  );
}

void maplibre_native_ffi_free_camera_read(
  MaplibreNativeFfiFreeCamera* out, const mln_free_camera* in
) {
  maplibre_native_ffi_vec3_read(&out->position, &in->position);
  maplibre_native_ffi_quaternion_read(&out->orientation, &in->orientation);
}

MaplibreNativeFfiFreeCamera* maplibre_native_ffi_free_camera_from_native(
  const mln_free_camera* in
) {
  MaplibreNativeFfiFreeCamera* self = g_new0(MaplibreNativeFfiFreeCamera, 1);
  maplibre_native_ffi_free_camera_read(self, in);
  return self;
}

struct _MaplibreNativeFfiFreeCameraOptions {
  guint64 fields;
  MaplibreNativeFfiVec3 position;
  MaplibreNativeFfiQuaternion orientation;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiFreeCameraOptions, maplibre_native_ffi_free_camera_options,
  maplibre_native_ffi_free_camera_options_copy,
  maplibre_native_ffi_free_camera_options_free
)

MaplibreNativeFfiFreeCameraOptions* maplibre_native_ffi_free_camera_options_new(
  void
) {
  MaplibreNativeFfiFreeCameraOptions* self =
    g_new0(MaplibreNativeFfiFreeCameraOptions, 1);
  return self;
}

MaplibreNativeFfiFreeCameraOptions*
maplibre_native_ffi_free_camera_options_copy(
  const MaplibreNativeFfiFreeCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiFreeCameraOptions* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_free_camera_options_free(
  MaplibreNativeFfiFreeCameraOptions* self
) {
  if (self == NULL) return;
  g_free(self);
}

MaplibreNativeFfiVec3* maplibre_native_ffi_free_camera_options_get_position(
  const MaplibreNativeFfiFreeCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiVec3*)&self->position;
}

gboolean maplibre_native_ffi_free_camera_options_has_position(
  const MaplibreNativeFfiFreeCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_FREE_CAMERA_OPTION_POSITION) != 0;
}

void maplibre_native_ffi_free_camera_options_set_position(
  MaplibreNativeFfiFreeCameraOptions* self, const MaplibreNativeFfiVec3* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->position = *value;
  self->fields |= MLN_FREE_CAMERA_OPTION_POSITION;
}

MaplibreNativeFfiQuaternion*
maplibre_native_ffi_free_camera_options_get_orientation(
  const MaplibreNativeFfiFreeCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiQuaternion*)&self->orientation;
}

gboolean maplibre_native_ffi_free_camera_options_has_orientation(
  const MaplibreNativeFfiFreeCameraOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0;
}

void maplibre_native_ffi_free_camera_options_set_orientation(
  MaplibreNativeFfiFreeCameraOptions* self,
  const MaplibreNativeFfiQuaternion* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->orientation = *value;
  self->fields |= MLN_FREE_CAMERA_OPTION_ORIENTATION;
}

gpointer maplibre_native_ffi_free_camera_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_free_camera_options_copy(
    (const MaplibreNativeFfiFreeCameraOptions*)self
  );
}

void maplibre_native_ffi_free_camera_options_to_native(
  const MaplibreNativeFfiFreeCameraOptions* self, mln_free_camera_options* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_free_camera_option_fields)self->fields;
  maplibre_native_ffi_vec3_to_native(&self->position, &out->position, call);
  maplibre_native_ffi_quaternion_to_native(
    &self->orientation, &out->orientation, call
  );
}

MaplibreNativeFfiFreeCameraOptions*
maplibre_native_ffi_free_camera_options_from_native(
  const mln_free_camera_options* in
) {
  MaplibreNativeFfiFreeCameraOptions* self =
    maplibre_native_ffi_free_camera_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_FREE_CAMERA_OPTION_POSITION) != 0) {
    maplibre_native_ffi_vec3_read(&self->position, &in->position);
  }
  if ((in->fields & MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0) {
    maplibre_native_ffi_quaternion_read(&self->orientation, &in->orientation);
  }
  return self;
}

struct _MaplibreNativeFfiGeoJsonSourceOptions {
  guint64 fields;
  gdouble min_zoom;
  gdouble max_zoom;
  gdouble tolerance;
  gdouble cluster_max_zoom;
  char* cluster_properties;
  guint32 buffer;
  guint32 cluster_radius;
  guint64 cluster_min_points;
  gboolean line_metrics;
  gboolean cluster;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiGeoJsonSourceOptions,
  maplibre_native_ffi_geo_json_source_options,
  maplibre_native_ffi_geo_json_source_options_copy,
  maplibre_native_ffi_geo_json_source_options_free
)

MaplibreNativeFfiGeoJsonSourceOptions*
maplibre_native_ffi_geo_json_source_options_new(void) {
  MaplibreNativeFfiGeoJsonSourceOptions* self =
    g_new0(MaplibreNativeFfiGeoJsonSourceOptions, 1);
  self->cluster_properties = g_strdup("");
  return self;
}

MaplibreNativeFfiGeoJsonSourceOptions*
maplibre_native_ffi_geo_json_source_options_copy(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiGeoJsonSourceOptions* copy = g_memdup2(self, sizeof *self);
  copy->cluster_properties = g_strdup(self->cluster_properties);
  return copy;
}

void maplibre_native_ffi_geo_json_source_options_free(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  if (self == NULL) return;
  g_free(self->cluster_properties);
  g_free(self);
}

gdouble maplibre_native_ffi_geo_json_source_options_get_min_zoom(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->min_zoom;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_min_zoom(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_min_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->min_zoom = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM;
}

gdouble maplibre_native_ffi_geo_json_source_options_get_max_zoom(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->max_zoom;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_max_zoom(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->max_zoom = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM;
}

gdouble maplibre_native_ffi_geo_json_source_options_get_tolerance(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->tolerance;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_tolerance(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_tolerance(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->tolerance = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE;
}

gdouble maplibre_native_ffi_geo_json_source_options_get_cluster_max_zoom(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->cluster_max_zoom;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_cluster_max_zoom(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_cluster_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->cluster_max_zoom = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
}

const char* maplibre_native_ffi_geo_json_source_options_get_cluster_properties(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->cluster_properties;
}

void maplibre_native_ffi_geo_json_source_options_set_cluster_properties(
  MaplibreNativeFfiGeoJsonSourceOptions* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->cluster_properties;
  self->cluster_properties = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

guint32 maplibre_native_ffi_geo_json_source_options_get_buffer(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->buffer;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_buffer(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_BUFFER) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_buffer(
  MaplibreNativeFfiGeoJsonSourceOptions* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->buffer = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_BUFFER;
}

guint32 maplibre_native_ffi_geo_json_source_options_get_cluster_radius(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->cluster_radius;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_cluster_radius(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_cluster_radius(
  MaplibreNativeFfiGeoJsonSourceOptions* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->cluster_radius = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS;
}

guint64 maplibre_native_ffi_geo_json_source_options_get_cluster_min_points(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->cluster_min_points;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_cluster_min_points(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_cluster_min_points(
  MaplibreNativeFfiGeoJsonSourceOptions* self, guint64 value
) {
  g_return_if_fail(self != NULL);
  self->cluster_min_points = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
}

gboolean maplibre_native_ffi_geo_json_source_options_get_line_metrics(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->line_metrics;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_line_metrics(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_line_metrics(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->line_metrics = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS;
}

gboolean maplibre_native_ffi_geo_json_source_options_get_cluster(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->cluster;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_cluster(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_cluster(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->cluster = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_CLUSTER;
}

gpointer maplibre_native_ffi_geo_json_source_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_geo_json_source_options_copy(
    (const MaplibreNativeFfiGeoJsonSourceOptions*)self
  );
}

void maplibre_native_ffi_geo_json_source_options_to_native(
  const MaplibreNativeFfiGeoJsonSourceOptions* self,
  mln_geo_json_source_options* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_geo_json_source_option_fields)self->fields;
  out->min_zoom = (double)(self->min_zoom);
  out->max_zoom = (double)(self->max_zoom);
  out->tolerance = (double)(self->tolerance);
  out->cluster_max_zoom = (double)(self->cluster_max_zoom);
  out->cluster_properties =
    maplibre_native_ffi_text_view(self->cluster_properties);
  out->buffer = (uint32_t)(self->buffer);
  out->cluster_radius = (uint32_t)(self->cluster_radius);
  out->cluster_min_points = (uint64_t)(self->cluster_min_points);
  out->line_metrics = (self->line_metrics) != FALSE;
  out->cluster = (self->cluster) != FALSE;
}

MaplibreNativeFfiGeoJsonSourceOptions*
maplibre_native_ffi_geo_json_source_options_from_native(
  const mln_geo_json_source_options* in
) {
  MaplibreNativeFfiGeoJsonSourceOptions* self =
    maplibre_native_ffi_geo_json_source_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM) != 0) {
    self->min_zoom = (gdouble)(in->min_zoom);
  }
  if ((in->fields & MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM) != 0) {
    self->max_zoom = (gdouble)(in->max_zoom);
  }
  if ((in->fields & MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE) != 0) {
    self->tolerance = (gdouble)(in->tolerance);
  }
  if ((in->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM) != 0) {
    self->cluster_max_zoom = (gdouble)(in->cluster_max_zoom);
  }
  g_free(self->cluster_properties);
  self->cluster_properties =
    maplibre_native_ffi_text_new(in->cluster_properties);
  if ((in->fields & MLN_GEO_JSON_SOURCE_OPTION_BUFFER) != 0) {
    self->buffer = (guint32)(in->buffer);
  }
  if ((in->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS) != 0) {
    self->cluster_radius = (guint32)(in->cluster_radius);
  }
  if ((in->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS) != 0) {
    self->cluster_min_points = (guint64)(in->cluster_min_points);
  }
  if ((in->fields & MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS) != 0) {
    self->line_metrics = (in->line_metrics) ? TRUE : FALSE;
  }
  if ((in->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER) != 0) {
    self->cluster = (in->cluster) ? TRUE : FALSE;
  }
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiGpuSync, maplibre_native_ffi_gpu_sync,
  maplibre_native_ffi_gpu_sync_copy, maplibre_native_ffi_gpu_sync_free
)

MaplibreNativeFfiGpuSync* maplibre_native_ffi_gpu_sync_copy(
  const MaplibreNativeFfiGpuSync* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_gpu_sync_free(MaplibreNativeFfiGpuSync* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_gpu_sync_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_gpu_sync_copy(
    (const MaplibreNativeFfiGpuSync*)self
  );
}

void maplibre_native_ffi_gpu_sync_to_native(
  const MaplibreNativeFfiGpuSync* self, mln_gpu_sync* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->kind = (mln_gpu_sync_kind)(self->kind);
  out->handle = (uint64_t)(self->handle);
  out->value = (uint64_t)(self->value);
}

void maplibre_native_ffi_gpu_sync_read(
  MaplibreNativeFfiGpuSync* out, const mln_gpu_sync* in
) {
  out->kind = (MaplibreNativeFfiGpuSyncKind)(in->kind);
  out->handle = (guint64)(in->handle);
  out->value = (guint64)(in->value);
}

MaplibreNativeFfiGpuSync* maplibre_native_ffi_gpu_sync_from_native(
  const mln_gpu_sync* in
) {
  MaplibreNativeFfiGpuSync* self = g_new0(MaplibreNativeFfiGpuSync, 1);
  maplibre_native_ffi_gpu_sync_read(self, in);
  return self;
}

struct _MaplibreNativeFfiHttpHeader {
  char* name;
  char* value;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiHttpHeader, maplibre_native_ffi_http_header,
  maplibre_native_ffi_http_header_copy, maplibre_native_ffi_http_header_free
)

MaplibreNativeFfiHttpHeader* maplibre_native_ffi_http_header_new(void) {
  MaplibreNativeFfiHttpHeader* self = g_new0(MaplibreNativeFfiHttpHeader, 1);
  self->name = g_strdup("");
  self->value = g_strdup("");
  return self;
}

MaplibreNativeFfiHttpHeader* maplibre_native_ffi_http_header_copy(
  const MaplibreNativeFfiHttpHeader* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiHttpHeader* copy = g_memdup2(self, sizeof *self);
  copy->name = g_strdup(self->name);
  copy->value = g_strdup(self->value);
  return copy;
}

void maplibre_native_ffi_http_header_free(MaplibreNativeFfiHttpHeader* self) {
  if (self == NULL) return;
  g_free(self->name);
  g_free(self->value);
  g_free(self);
}

const char* maplibre_native_ffi_http_header_get_name(
  const MaplibreNativeFfiHttpHeader* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->name;
}

void maplibre_native_ffi_http_header_set_name(
  MaplibreNativeFfiHttpHeader* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->name;
  self->name = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

const char* maplibre_native_ffi_http_header_get_value(
  const MaplibreNativeFfiHttpHeader* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->value;
}

void maplibre_native_ffi_http_header_set_value(
  MaplibreNativeFfiHttpHeader* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->value;
  self->value = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

gpointer maplibre_native_ffi_http_header_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_http_header_copy(
    (const MaplibreNativeFfiHttpHeader*)self
  );
}

void maplibre_native_ffi_http_header_to_native(
  const MaplibreNativeFfiHttpHeader* self, mln_http_header* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->name = maplibre_native_ffi_text_view(self->name);
  out->value = maplibre_native_ffi_text_view(self->value);
}

MaplibreNativeFfiHttpHeader* maplibre_native_ffi_http_header_from_native(
  const mln_http_header* in
) {
  MaplibreNativeFfiHttpHeader* self = maplibre_native_ffi_http_header_new();
  g_free(self->name);
  self->name = maplibre_native_ffi_text_new(in->name);
  g_free(self->value);
  self->value = maplibre_native_ffi_text_new(in->value);
  return self;
}

struct _MaplibreNativeFfiHttpHeaderRule {
  MaplibreNativeFfiResourceKindFlag kinds;
  MaplibreNativeFfiUrlMatchFlag flags;
  char* url;
  GPtrArray* headers;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiHttpHeaderRule, maplibre_native_ffi_http_header_rule,
  maplibre_native_ffi_http_header_rule_copy,
  maplibre_native_ffi_http_header_rule_free
)

MaplibreNativeFfiHttpHeaderRule* maplibre_native_ffi_http_header_rule_new(
  void
) {
  MaplibreNativeFfiHttpHeaderRule* self =
    g_new0(MaplibreNativeFfiHttpHeaderRule, 1);
  self->url = g_strdup("");
  self->headers = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_http_header_free
  );
  return self;
}

MaplibreNativeFfiHttpHeaderRule* maplibre_native_ffi_http_header_rule_copy(
  const MaplibreNativeFfiHttpHeaderRule* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiHttpHeaderRule* copy = g_memdup2(self, sizeof *self);
  copy->url = g_strdup(self->url);
  copy->headers =
    self->headers != NULL
      ? g_ptr_array_copy(
          self->headers, maplibre_native_ffi_http_header_copy_func, NULL
        )
      : NULL;
  if (copy->headers != NULL)
    g_ptr_array_set_free_func(
      copy->headers, (GDestroyNotify)maplibre_native_ffi_http_header_free
    );
  return copy;
}

void maplibre_native_ffi_http_header_rule_free(
  MaplibreNativeFfiHttpHeaderRule* self
) {
  if (self == NULL) return;
  g_free(self->url);
  g_clear_pointer(&self->headers, g_ptr_array_unref);
  g_free(self);
}

MaplibreNativeFfiResourceKindFlag
maplibre_native_ffi_http_header_rule_get_kinds(
  const MaplibreNativeFfiHttpHeaderRule* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiResourceKindFlag)0);
  return self->kinds;
}

void maplibre_native_ffi_http_header_rule_set_kinds(
  MaplibreNativeFfiHttpHeaderRule* self, MaplibreNativeFfiResourceKindFlag value
) {
  g_return_if_fail(self != NULL);
  self->kinds = value;
}

MaplibreNativeFfiUrlMatchFlag maplibre_native_ffi_http_header_rule_get_flags(
  const MaplibreNativeFfiHttpHeaderRule* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiUrlMatchFlag)0);
  return self->flags;
}

void maplibre_native_ffi_http_header_rule_set_flags(
  MaplibreNativeFfiHttpHeaderRule* self, MaplibreNativeFfiUrlMatchFlag value
) {
  g_return_if_fail(self != NULL);
  self->flags = value;
}

const char* maplibre_native_ffi_http_header_rule_get_url(
  const MaplibreNativeFfiHttpHeaderRule* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->url;
}

void maplibre_native_ffi_http_header_rule_set_url(
  MaplibreNativeFfiHttpHeaderRule* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->url;
  self->url = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

GPtrArray* maplibre_native_ffi_http_header_rule_get_headers(
  const MaplibreNativeFfiHttpHeaderRule* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->headers;
}

void maplibre_native_ffi_http_header_rule_set_headers(
  MaplibreNativeFfiHttpHeaderRule* self,
  MaplibreNativeFfiHttpHeader* const* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->headers;
  self->headers = g_ptr_array_new_full(
    (guint)value_count, (GDestroyNotify)maplibre_native_ffi_http_header_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->headers, maplibre_native_ffi_http_header_copy(value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

gpointer maplibre_native_ffi_http_header_rule_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_http_header_rule_copy(
    (const MaplibreNativeFfiHttpHeaderRule*)self
  );
}

void maplibre_native_ffi_http_header_rule_to_native(
  const MaplibreNativeFfiHttpHeaderRule* self, mln_http_header_rule* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->kinds = (mln_resource_kind_flags)(self->kinds);
  out->flags = (mln_url_match_flags)(self->flags);
  out->url = maplibre_native_ffi_text_view(self->url);
  if (self->headers != NULL && self->headers->len > 0) {
    mln_http_header* items =
      maplibre_native_ffi_call_alloc(call, self->headers->len, sizeof *items);
    for (guint index = 0; index < self->headers->len; index++) {
      maplibre_native_ffi_http_header_to_native(
        self->headers->pdata[index], &items[index], call
      );
    }
    out->headers = items;
    out->header_count = self->headers->len;
  }
}

MaplibreNativeFfiHttpHeaderRule*
maplibre_native_ffi_http_header_rule_from_native(
  const mln_http_header_rule* in
) {
  MaplibreNativeFfiHttpHeaderRule* self =
    maplibre_native_ffi_http_header_rule_new();
  self->kinds = (MaplibreNativeFfiResourceKindFlag)(in->kinds);
  self->flags = (MaplibreNativeFfiUrlMatchFlag)(in->flags);
  g_free(self->url);
  self->url = maplibre_native_ffi_text_new(in->url);
  g_clear_pointer(&self->headers, g_ptr_array_unref);
  self->headers = g_ptr_array_new_full(
    (guint)in->header_count,
    (GDestroyNotify)maplibre_native_ffi_http_header_free
  );
  for (size_t index = 0; index < in->header_count; index++) {
    g_ptr_array_add(
      self->headers,
      maplibre_native_ffi_http_header_from_native(&in->headers[index])
    );
  }
  return self;
}

struct _MaplibreNativeFfiHttpHeaderTransform {
  GPtrArray* rules;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiHttpHeaderTransform,
  maplibre_native_ffi_http_header_transform,
  maplibre_native_ffi_http_header_transform_copy,
  maplibre_native_ffi_http_header_transform_free
)

MaplibreNativeFfiHttpHeaderTransform*
maplibre_native_ffi_http_header_transform_new(void) {
  MaplibreNativeFfiHttpHeaderTransform* self =
    g_new0(MaplibreNativeFfiHttpHeaderTransform, 1);
  self->rules = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_http_header_rule_free
  );
  return self;
}

MaplibreNativeFfiHttpHeaderTransform*
maplibre_native_ffi_http_header_transform_copy(
  const MaplibreNativeFfiHttpHeaderTransform* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiHttpHeaderTransform* copy = g_memdup2(self, sizeof *self);
  copy->rules =
    self->rules != NULL
      ? g_ptr_array_copy(
          self->rules, maplibre_native_ffi_http_header_rule_copy_func, NULL
        )
      : NULL;
  if (copy->rules != NULL)
    g_ptr_array_set_free_func(
      copy->rules, (GDestroyNotify)maplibre_native_ffi_http_header_rule_free
    );
  return copy;
}

void maplibre_native_ffi_http_header_transform_free(
  MaplibreNativeFfiHttpHeaderTransform* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->rules, g_ptr_array_unref);
  g_free(self);
}

GPtrArray* maplibre_native_ffi_http_header_transform_get_rules(
  const MaplibreNativeFfiHttpHeaderTransform* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->rules;
}

void maplibre_native_ffi_http_header_transform_set_rules(
  MaplibreNativeFfiHttpHeaderTransform* self,
  MaplibreNativeFfiHttpHeaderRule* const* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->rules;
  self->rules = g_ptr_array_new_full(
    (guint)value_count,
    (GDestroyNotify)maplibre_native_ffi_http_header_rule_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->rules, maplibre_native_ffi_http_header_rule_copy(value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

gpointer maplibre_native_ffi_http_header_transform_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_http_header_transform_copy(
    (const MaplibreNativeFfiHttpHeaderTransform*)self
  );
}

void maplibre_native_ffi_http_header_transform_to_native(
  const MaplibreNativeFfiHttpHeaderTransform* self,
  mln_http_header_transform* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  if (self->rules != NULL && self->rules->len > 0) {
    mln_http_header_rule* items =
      maplibre_native_ffi_call_alloc(call, self->rules->len, sizeof *items);
    for (guint index = 0; index < self->rules->len; index++) {
      maplibre_native_ffi_http_header_rule_to_native(
        self->rules->pdata[index], &items[index], call
      );
    }
    out->rules = items;
    out->rule_count = self->rules->len;
  }
}

MaplibreNativeFfiHttpHeaderTransform*
maplibre_native_ffi_http_header_transform_from_native(
  const mln_http_header_transform* in
) {
  MaplibreNativeFfiHttpHeaderTransform* self =
    maplibre_native_ffi_http_header_transform_new();
  g_clear_pointer(&self->rules, g_ptr_array_unref);
  self->rules = g_ptr_array_new_full(
    (guint)in->rule_count,
    (GDestroyNotify)maplibre_native_ffi_http_header_rule_free
  );
  for (size_t index = 0; index < in->rule_count; index++) {
    g_ptr_array_add(
      self->rules,
      maplibre_native_ffi_http_header_rule_from_native(&in->rules[index])
    );
  }
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiImageContent, maplibre_native_ffi_image_content,
  maplibre_native_ffi_image_content_copy, maplibre_native_ffi_image_content_free
)

MaplibreNativeFfiImageContent* maplibre_native_ffi_image_content_copy(
  const MaplibreNativeFfiImageContent* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_image_content_free(
  MaplibreNativeFfiImageContent* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_image_content_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_image_content_copy(
    (const MaplibreNativeFfiImageContent*)self
  );
}

void maplibre_native_ffi_image_content_to_native(
  const MaplibreNativeFfiImageContent* self, mln_image_content* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->left = (float)(self->left);
  out->top = (float)(self->top);
  out->right = (float)(self->right);
  out->bottom = (float)(self->bottom);
}

void maplibre_native_ffi_image_content_read(
  MaplibreNativeFfiImageContent* out, const mln_image_content* in
) {
  out->left = (gfloat)(in->left);
  out->top = (gfloat)(in->top);
  out->right = (gfloat)(in->right);
  out->bottom = (gfloat)(in->bottom);
}

MaplibreNativeFfiImageContent* maplibre_native_ffi_image_content_from_native(
  const mln_image_content* in
) {
  MaplibreNativeFfiImageContent* self =
    g_new0(MaplibreNativeFfiImageContent, 1);
  maplibre_native_ffi_image_content_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiImageStretch, maplibre_native_ffi_image_stretch,
  maplibre_native_ffi_image_stretch_copy, maplibre_native_ffi_image_stretch_free
)

MaplibreNativeFfiImageStretch* maplibre_native_ffi_image_stretch_copy(
  const MaplibreNativeFfiImageStretch* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_image_stretch_free(
  MaplibreNativeFfiImageStretch* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_image_stretch_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_image_stretch_copy(
    (const MaplibreNativeFfiImageStretch*)self
  );
}

void maplibre_native_ffi_image_stretch_to_native(
  const MaplibreNativeFfiImageStretch* self, mln_image_stretch* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->from = (float)(self->from);
  out->to = (float)(self->to);
}

void maplibre_native_ffi_image_stretch_read(
  MaplibreNativeFfiImageStretch* out, const mln_image_stretch* in
) {
  out->from = (gfloat)(in->from);
  out->to = (gfloat)(in->to);
}

MaplibreNativeFfiImageStretch* maplibre_native_ffi_image_stretch_from_native(
  const mln_image_stretch* in
) {
  MaplibreNativeFfiImageStretch* self =
    g_new0(MaplibreNativeFfiImageStretch, 1);
  maplibre_native_ffi_image_stretch_read(self, in);
  return self;
}

struct _MaplibreNativeFfiIoSurfaceFrame {
  guint32 width;
  guint32 height;
  gdouble scale_factor;
  guint32 slot;
  gpointer surface;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiIoSurfaceFrame, maplibre_native_ffi_io_surface_frame,
  maplibre_native_ffi_io_surface_frame_copy,
  maplibre_native_ffi_io_surface_frame_free
)

MaplibreNativeFfiIoSurfaceFrame* maplibre_native_ffi_io_surface_frame_new(
  void
) {
  MaplibreNativeFfiIoSurfaceFrame* self =
    g_new0(MaplibreNativeFfiIoSurfaceFrame, 1);
  return self;
}

MaplibreNativeFfiIoSurfaceFrame* maplibre_native_ffi_io_surface_frame_copy(
  const MaplibreNativeFfiIoSurfaceFrame* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiIoSurfaceFrame* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_io_surface_frame_free(
  MaplibreNativeFfiIoSurfaceFrame* self
) {
  if (self == NULL) return;
  g_free(self);
}

guint32 maplibre_native_ffi_io_surface_frame_get_width(
  const MaplibreNativeFfiIoSurfaceFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->width;
}

guint32 maplibre_native_ffi_io_surface_frame_get_height(
  const MaplibreNativeFfiIoSurfaceFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->height;
}

gdouble maplibre_native_ffi_io_surface_frame_get_scale_factor(
  const MaplibreNativeFfiIoSurfaceFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->scale_factor;
}

guint32 maplibre_native_ffi_io_surface_frame_get_slot(
  const MaplibreNativeFfiIoSurfaceFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->slot;
}

gpointer maplibre_native_ffi_io_surface_frame_get_surface(
  const MaplibreNativeFfiIoSurfaceFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->surface;
}

gpointer maplibre_native_ffi_io_surface_frame_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_io_surface_frame_copy(
    (const MaplibreNativeFfiIoSurfaceFrame*)self
  );
}

void maplibre_native_ffi_io_surface_frame_to_native(
  const MaplibreNativeFfiIoSurfaceFrame* self, mln_io_surface_frame* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->width = (uint32_t)(self->width);
  out->height = (uint32_t)(self->height);
  out->scale_factor = (double)(self->scale_factor);
  out->slot = (uint32_t)(self->slot);
  out->surface = (void*)(self->surface);
}

MaplibreNativeFfiIoSurfaceFrame*
maplibre_native_ffi_io_surface_frame_from_native(
  const mln_io_surface_frame* in
) {
  MaplibreNativeFfiIoSurfaceFrame* self =
    maplibre_native_ffi_io_surface_frame_new();
  self->width = (guint32)(in->width);
  self->height = (guint32)(in->height);
  self->scale_factor = (gdouble)(in->scale_factor);
  self->slot = (guint32)(in->slot);
  self->surface = (gpointer)(in->surface);
  return self;
}

struct _MaplibreNativeFfiMetalContextDescriptor {
  gpointer device;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMetalContextDescriptor,
  maplibre_native_ffi_metal_context_descriptor,
  maplibre_native_ffi_metal_context_descriptor_copy,
  maplibre_native_ffi_metal_context_descriptor_free
)

MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_metal_context_descriptor_new(void) {
  MaplibreNativeFfiMetalContextDescriptor* self =
    g_new0(MaplibreNativeFfiMetalContextDescriptor, 1);
  return self;
}

MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_metal_context_descriptor_copy(
  const MaplibreNativeFfiMetalContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiMetalContextDescriptor* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_metal_context_descriptor_free(
  MaplibreNativeFfiMetalContextDescriptor* self
) {
  if (self == NULL) return;
  g_free(self);
}

gpointer maplibre_native_ffi_metal_context_descriptor_get_device(
  const MaplibreNativeFfiMetalContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->device;
}

void maplibre_native_ffi_metal_context_descriptor_set_device(
  MaplibreNativeFfiMetalContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->device = value;
}

gpointer maplibre_native_ffi_metal_context_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_metal_context_descriptor_copy(
    (const MaplibreNativeFfiMetalContextDescriptor*)self
  );
}

void maplibre_native_ffi_metal_context_descriptor_to_native(
  const MaplibreNativeFfiMetalContextDescriptor* self,
  mln_metal_context_descriptor* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->device = (void*)(self->device);
}

MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_metal_context_descriptor_from_native(
  const mln_metal_context_descriptor* in
) {
  MaplibreNativeFfiMetalContextDescriptor* self =
    maplibre_native_ffi_metal_context_descriptor_new();
  self->device = (gpointer)(in->device);
  return self;
}

struct _MaplibreNativeFfiIoSurfaceRingDescriptor {
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 buffer_count;
  MaplibreNativeFfiMetalContextDescriptor* context;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiIoSurfaceRingDescriptor,
  maplibre_native_ffi_io_surface_ring_descriptor,
  maplibre_native_ffi_io_surface_ring_descriptor_copy,
  maplibre_native_ffi_io_surface_ring_descriptor_free
)

MaplibreNativeFfiIoSurfaceRingDescriptor*
maplibre_native_ffi_io_surface_ring_descriptor_new(void) {
  MaplibreNativeFfiIoSurfaceRingDescriptor* self =
    g_new0(MaplibreNativeFfiIoSurfaceRingDescriptor, 1);
  self->context = maplibre_native_ffi_metal_context_descriptor_new();
  return self;
}

MaplibreNativeFfiIoSurfaceRingDescriptor*
maplibre_native_ffi_io_surface_ring_descriptor_copy(
  const MaplibreNativeFfiIoSurfaceRingDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiIoSurfaceRingDescriptor* copy =
    g_memdup2(self, sizeof *self);
  copy->context =
    self->context != NULL
      ? maplibre_native_ffi_metal_context_descriptor_copy(self->context)
      : NULL;
  return copy;
}

void maplibre_native_ffi_io_surface_ring_descriptor_free(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self
) {
  if (self == NULL) return;
  g_clear_pointer(
    &self->context, maplibre_native_ffi_metal_context_descriptor_free
  );
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_io_surface_ring_descriptor_get_logical_size(
  const MaplibreNativeFfiIoSurfaceRingDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_io_surface_ring_descriptor_set_logical_size(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

guint32 maplibre_native_ffi_io_surface_ring_descriptor_get_buffer_count(
  const MaplibreNativeFfiIoSurfaceRingDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->buffer_count;
}

void maplibre_native_ffi_io_surface_ring_descriptor_set_buffer_count(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->buffer_count = value;
}

MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_io_surface_ring_descriptor_get_context(
  const MaplibreNativeFfiIoSurfaceRingDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->context;
}

void maplibre_native_ffi_io_surface_ring_descriptor_set_context(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self,
  const MaplibreNativeFfiMetalContextDescriptor* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiMetalContextDescriptor* previous = self->context;
  self->context = maplibre_native_ffi_metal_context_descriptor_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_metal_context_descriptor_free(previous);
}

gpointer maplibre_native_ffi_io_surface_ring_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_io_surface_ring_descriptor_copy(
    (const MaplibreNativeFfiIoSurfaceRingDescriptor*)self
  );
}

void maplibre_native_ffi_io_surface_ring_descriptor_to_native(
  const MaplibreNativeFfiIoSurfaceRingDescriptor* self,
  mln_io_surface_ring_descriptor* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  out->buffer_count = (uint32_t)(self->buffer_count);
  if (self->context != NULL)
    maplibre_native_ffi_metal_context_descriptor_to_native(
      self->context, &out->context, call
    );
}

MaplibreNativeFfiIoSurfaceRingDescriptor*
maplibre_native_ffi_io_surface_ring_descriptor_from_native(
  const mln_io_surface_ring_descriptor* in
) {
  MaplibreNativeFfiIoSurfaceRingDescriptor* self =
    maplibre_native_ffi_io_surface_ring_descriptor_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  self->buffer_count = (guint32)(in->buffer_count);
  g_clear_pointer(
    &self->context, maplibre_native_ffi_metal_context_descriptor_free
  );
  self->context =
    maplibre_native_ffi_metal_context_descriptor_from_native(&in->context);
  return self;
}

struct _MaplibreNativeFfiLogHandler {
  MaplibreNativeFfiLogSeverityFlag platform_severities;
  MaplibreNativeFfiClosure* callback;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiLogHandler, maplibre_native_ffi_log_handler,
  maplibre_native_ffi_log_handler_copy, maplibre_native_ffi_log_handler_free
)

MaplibreNativeFfiLogHandler* maplibre_native_ffi_log_handler_new(void) {
  MaplibreNativeFfiLogHandler* self = g_new0(MaplibreNativeFfiLogHandler, 1);
  return self;
}

MaplibreNativeFfiLogHandler* maplibre_native_ffi_log_handler_copy(
  const MaplibreNativeFfiLogHandler* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiLogHandler* copy = g_memdup2(self, sizeof *self);
  maplibre_native_ffi_closure_ref(copy->callback);
  return copy;
}

void maplibre_native_ffi_log_handler_free(MaplibreNativeFfiLogHandler* self) {
  if (self == NULL) return;
  maplibre_native_ffi_closure_unref(self->callback);
  g_free(self);
}

MaplibreNativeFfiLogSeverityFlag
maplibre_native_ffi_log_handler_get_platform_severities(
  const MaplibreNativeFfiLogHandler* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiLogSeverityFlag)0);
  return self->platform_severities;
}

void maplibre_native_ffi_log_handler_set_platform_severities(
  MaplibreNativeFfiLogHandler* self, MaplibreNativeFfiLogSeverityFlag value
) {
  g_return_if_fail(self != NULL);
  self->platform_severities = value;
}

void maplibre_native_ffi_log_handler_set_callback(
  MaplibreNativeFfiLogHandler* self, MaplibreNativeFfiLogCallback callback,
  gpointer user_data, GDestroyNotify notify
) {
  g_return_if_fail(self != NULL);
  MaplibreNativeFfiClosure* previous = self->callback;
  self->callback =
    callback != NULL
      ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify)
      : NULL;
  if (callback == NULL && notify != NULL) notify(user_data);
  maplibre_native_ffi_closure_unref(previous);
}

gpointer maplibre_native_ffi_log_handler_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_log_handler_copy(
    (const MaplibreNativeFfiLogHandler*)self
  );
}

static void log_handler_callback_trampoline(
  void* user_data, mln_log_severity severity, mln_log_event event, int64_t code,
  mln_buffer_view message
) {
  MaplibreNativeFfiClosure* closure =
    maplibre_native_ffi_registration_closure(user_data, 0);
  if (closure == NULL) return;
  char* message_ = maplibre_native_ffi_text_new(message);
  ((MaplibreNativeFfiLogCallback)(void (*)(void))closure->func)(
    (MaplibreNativeFfiLogSeverity)(severity),
    (MaplibreNativeFfiLogEvent)(event), (gint64)(code), message_,
    closure->user_data
  );
  g_free(message_);
}

void maplibre_native_ffi_log_handler_to_native(
  const MaplibreNativeFfiLogHandler* self, mln_log_handler* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->platform_severities =
    (mln_log_severity_flags)(self->platform_severities);
  if (self->callback != NULL) {
    MaplibreNativeFfiClosure* const closures[] = {self->callback};
    out->user_data =
      maplibre_native_ffi_call_register(call, closures, 1, &out->queue);
    out->release_user_data = maplibre_native_ffi_registration_release;
    out->callback =
      self->callback != NULL ? log_handler_callback_trampoline : NULL;
  }
}

MaplibreNativeFfiLogHandler* maplibre_native_ffi_log_handler_from_native(
  const mln_log_handler* in
) {
  MaplibreNativeFfiLogHandler* self = maplibre_native_ffi_log_handler_new();
  self->platform_severities =
    (MaplibreNativeFfiLogSeverityFlag)(in->platform_severities);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiLogicalExtent, maplibre_native_ffi_logical_extent,
  maplibre_native_ffi_logical_extent_copy,
  maplibre_native_ffi_logical_extent_free
)

MaplibreNativeFfiLogicalExtent* maplibre_native_ffi_logical_extent_copy(
  const MaplibreNativeFfiLogicalExtent* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_logical_extent_free(
  MaplibreNativeFfiLogicalExtent* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_logical_extent_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_logical_extent_copy(
    (const MaplibreNativeFfiLogicalExtent*)self
  );
}

void maplibre_native_ffi_logical_extent_to_native(
  const MaplibreNativeFfiLogicalExtent* self, mln_logical_extent* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->width = (uint32_t)(self->width);
  out->height = (uint32_t)(self->height);
  out->scale_factor = (double)(self->scale_factor);
}

void maplibre_native_ffi_logical_extent_read(
  MaplibreNativeFfiLogicalExtent* out, const mln_logical_extent* in
) {
  out->width = (guint32)(in->width);
  out->height = (guint32)(in->height);
  out->scale_factor = (gdouble)(in->scale_factor);
}

MaplibreNativeFfiLogicalExtent* maplibre_native_ffi_logical_extent_from_native(
  const mln_logical_extent* in
) {
  MaplibreNativeFfiLogicalExtent* self =
    g_new0(MaplibreNativeFfiLogicalExtent, 1);
  maplibre_native_ffi_logical_extent_read(self, in);
  return self;
}

struct _MaplibreNativeFfiMapOptions {
  guint64 fields;
  MaplibreNativeFfiLogicalExtent initial_extent;
  MaplibreNativeFfiMapMode map_mode;
  gboolean fast_pfor_enabled;
  MaplibreNativeFfiRuntimeEventFlag event_mask;
  gboolean tile_cache_enabled;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMapOptions, maplibre_native_ffi_map_options,
  maplibre_native_ffi_map_options_copy, maplibre_native_ffi_map_options_free
)

MaplibreNativeFfiMapOptions* maplibre_native_ffi_map_options_new(void) {
  MaplibreNativeFfiMapOptions* self = g_new0(MaplibreNativeFfiMapOptions, 1);
  return self;
}

MaplibreNativeFfiMapOptions* maplibre_native_ffi_map_options_copy(
  const MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiMapOptions* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_map_options_free(MaplibreNativeFfiMapOptions* self) {
  if (self == NULL) return;
  g_free(self);
}

MaplibreNativeFfiLogicalExtent*
maplibre_native_ffi_map_options_get_initial_extent(
  const MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalExtent*)&self->initial_extent;
}

void maplibre_native_ffi_map_options_set_initial_extent(
  MaplibreNativeFfiMapOptions* self, const MaplibreNativeFfiLogicalExtent* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->initial_extent = *value;
}

MaplibreNativeFfiMapMode maplibre_native_ffi_map_options_get_map_mode(
  const MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiMapMode)0);
  return self->map_mode;
}

void maplibre_native_ffi_map_options_set_map_mode(
  MaplibreNativeFfiMapOptions* self, MaplibreNativeFfiMapMode value
) {
  g_return_if_fail(self != NULL);
  self->map_mode = value;
}

gboolean maplibre_native_ffi_map_options_get_fast_pfor_enabled(
  const MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->fast_pfor_enabled;
}

void maplibre_native_ffi_map_options_set_fast_pfor_enabled(
  MaplibreNativeFfiMapOptions* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->fast_pfor_enabled = value;
}

MaplibreNativeFfiRuntimeEventFlag
maplibre_native_ffi_map_options_get_event_mask(
  const MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiRuntimeEventFlag)0);
  return self->event_mask;
}

gboolean maplibre_native_ffi_map_options_has_event_mask(
  const MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_MAP_OPTION_EVENT_MASK) != 0;
}

void maplibre_native_ffi_map_options_set_event_mask(
  MaplibreNativeFfiMapOptions* self, MaplibreNativeFfiRuntimeEventFlag value
) {
  g_return_if_fail(self != NULL);
  self->event_mask = value;
  self->fields |= MLN_MAP_OPTION_EVENT_MASK;
}

gboolean maplibre_native_ffi_map_options_get_tile_cache_enabled(
  const MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tile_cache_enabled;
}

gboolean maplibre_native_ffi_map_options_has_tile_cache_enabled(
  const MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_MAP_OPTION_TILE_CACHE_ENABLED) != 0;
}

void maplibre_native_ffi_map_options_set_tile_cache_enabled(
  MaplibreNativeFfiMapOptions* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->tile_cache_enabled = value;
  self->fields |= MLN_MAP_OPTION_TILE_CACHE_ENABLED;
}

gpointer maplibre_native_ffi_map_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_map_options_copy(
    (const MaplibreNativeFfiMapOptions*)self
  );
}

void maplibre_native_ffi_map_options_to_native(
  const MaplibreNativeFfiMapOptions* self, mln_map_options* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_map_option_fields)self->fields;
  maplibre_native_ffi_logical_extent_to_native(
    &self->initial_extent, &out->initial_extent, call
  );
  out->map_mode = (mln_map_mode)(self->map_mode);
  out->fast_pfor_enabled = (self->fast_pfor_enabled) != FALSE;
  out->event_mask = (mln_runtime_event_flags)(self->event_mask);
  out->tile_cache_enabled = (self->tile_cache_enabled) != FALSE;
}

MaplibreNativeFfiMapOptions* maplibre_native_ffi_map_options_from_native(
  const mln_map_options* in
) {
  MaplibreNativeFfiMapOptions* self = maplibre_native_ffi_map_options_new();
  self->fields = (guint64)in->fields;
  maplibre_native_ffi_logical_extent_read(
    &self->initial_extent, &in->initial_extent
  );
  self->map_mode = (MaplibreNativeFfiMapMode)(in->map_mode);
  self->fast_pfor_enabled = (in->fast_pfor_enabled) ? TRUE : FALSE;
  if ((in->fields & MLN_MAP_OPTION_EVENT_MASK) != 0) {
    self->event_mask = (MaplibreNativeFfiRuntimeEventFlag)(in->event_mask);
  }
  if ((in->fields & MLN_MAP_OPTION_TILE_CACHE_ENABLED) != 0) {
    self->tile_cache_enabled = (in->tile_cache_enabled) ? TRUE : FALSE;
  }
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMapProjectionCameraResult,
  maplibre_native_ffi_map_projection_camera_result,
  maplibre_native_ffi_map_projection_camera_result_copy,
  maplibre_native_ffi_map_projection_camera_result_free
)

MaplibreNativeFfiMapProjectionCameraResult*
maplibre_native_ffi_map_projection_camera_result_copy(
  const MaplibreNativeFfiMapProjectionCameraResult* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_map_projection_camera_result_free(
  MaplibreNativeFfiMapProjectionCameraResult* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_map_projection_camera_result_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_map_projection_camera_result_copy(
    (const MaplibreNativeFfiMapProjectionCameraResult*)self
  );
}

void maplibre_native_ffi_map_projection_camera_result_to_native(
  const MaplibreNativeFfiMapProjectionCameraResult* self,
  mln_map_projection_camera_result* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_camera_to_native(&self->camera, &out->camera, call);
}

void maplibre_native_ffi_map_projection_camera_result_read(
  MaplibreNativeFfiMapProjectionCameraResult* out,
  const mln_map_projection_camera_result* in
) {
  maplibre_native_ffi_camera_read(&out->camera, &in->camera);
}

MaplibreNativeFfiMapProjectionCameraResult*
maplibre_native_ffi_map_projection_camera_result_from_native(
  const mln_map_projection_camera_result* in
) {
  MaplibreNativeFfiMapProjectionCameraResult* self =
    g_new0(MaplibreNativeFfiMapProjectionCameraResult, 1);
  maplibre_native_ffi_map_projection_camera_result_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMapTileSettings, maplibre_native_ffi_map_tile_settings,
  maplibre_native_ffi_map_tile_settings_copy,
  maplibre_native_ffi_map_tile_settings_free
)

MaplibreNativeFfiMapTileSettings* maplibre_native_ffi_map_tile_settings_copy(
  const MaplibreNativeFfiMapTileSettings* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_map_tile_settings_free(
  MaplibreNativeFfiMapTileSettings* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_map_tile_settings_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_map_tile_settings_copy(
    (const MaplibreNativeFfiMapTileSettings*)self
  );
}

void maplibre_native_ffi_map_tile_settings_to_native(
  const MaplibreNativeFfiMapTileSettings* self, mln_map_tile_settings* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->prefetch_zoom_delta = (uint32_t)(self->prefetch_zoom_delta);
  out->lod_min_radius = (double)(self->lod_min_radius);
  out->lod_scale = (double)(self->lod_scale);
  out->lod_pitch_threshold = (double)(self->lod_pitch_threshold);
  out->lod_zoom_shift = (double)(self->lod_zoom_shift);
  out->lod_mode = (mln_tile_lod_mode)(self->lod_mode);
}

void maplibre_native_ffi_map_tile_settings_read(
  MaplibreNativeFfiMapTileSettings* out, const mln_map_tile_settings* in
) {
  out->prefetch_zoom_delta = (guint32)(in->prefetch_zoom_delta);
  out->lod_min_radius = (gdouble)(in->lod_min_radius);
  out->lod_scale = (gdouble)(in->lod_scale);
  out->lod_pitch_threshold = (gdouble)(in->lod_pitch_threshold);
  out->lod_zoom_shift = (gdouble)(in->lod_zoom_shift);
  out->lod_mode = (MaplibreNativeFfiTileLodMode)(in->lod_mode);
}

MaplibreNativeFfiMapTileSettings*
maplibre_native_ffi_map_tile_settings_from_native(
  const mln_map_tile_settings* in
) {
  MaplibreNativeFfiMapTileSettings* self =
    g_new0(MaplibreNativeFfiMapTileSettings, 1);
  maplibre_native_ffi_map_tile_settings_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiProjectionMode, maplibre_native_ffi_projection_mode,
  maplibre_native_ffi_projection_mode_copy,
  maplibre_native_ffi_projection_mode_free
)

MaplibreNativeFfiProjectionMode* maplibre_native_ffi_projection_mode_copy(
  const MaplibreNativeFfiProjectionMode* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_projection_mode_free(
  MaplibreNativeFfiProjectionMode* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_projection_mode_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_projection_mode_copy(
    (const MaplibreNativeFfiProjectionMode*)self
  );
}

void maplibre_native_ffi_projection_mode_to_native(
  const MaplibreNativeFfiProjectionMode* self, mln_projection_mode* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->axonometric = (self->axonometric) != FALSE;
  out->x_skew = (double)(self->x_skew);
  out->y_skew = (double)(self->y_skew);
}

void maplibre_native_ffi_projection_mode_read(
  MaplibreNativeFfiProjectionMode* out, const mln_projection_mode* in
) {
  out->axonometric = (in->axonometric) ? TRUE : FALSE;
  out->x_skew = (gdouble)(in->x_skew);
  out->y_skew = (gdouble)(in->y_skew);
}

MaplibreNativeFfiProjectionMode*
maplibre_native_ffi_projection_mode_from_native(const mln_projection_mode* in) {
  MaplibreNativeFfiProjectionMode* self =
    g_new0(MaplibreNativeFfiProjectionMode, 1);
  maplibre_native_ffi_projection_mode_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMapViewport, maplibre_native_ffi_map_viewport,
  maplibre_native_ffi_map_viewport_copy, maplibre_native_ffi_map_viewport_free
)

MaplibreNativeFfiMapViewport* maplibre_native_ffi_map_viewport_copy(
  const MaplibreNativeFfiMapViewport* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_map_viewport_free(MaplibreNativeFfiMapViewport* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_map_viewport_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_map_viewport_copy(
    (const MaplibreNativeFfiMapViewport*)self
  );
}

void maplibre_native_ffi_map_viewport_to_native(
  const MaplibreNativeFfiMapViewport* self, mln_map_viewport* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->north_orientation = (mln_north_orientation)(self->north_orientation);
  out->constrain_mode = (mln_constrain_mode)(self->constrain_mode);
  out->viewport_mode = (mln_viewport_mode)(self->viewport_mode);
  maplibre_native_ffi_edge_insets_to_native(
    &self->frustum_offset, &out->frustum_offset, call
  );
}

void maplibre_native_ffi_map_viewport_read(
  MaplibreNativeFfiMapViewport* out, const mln_map_viewport* in
) {
  out->north_orientation =
    (MaplibreNativeFfiNorthOrientation)(in->north_orientation);
  out->constrain_mode = (MaplibreNativeFfiConstrainMode)(in->constrain_mode);
  out->viewport_mode = (MaplibreNativeFfiViewportMode)(in->viewport_mode);
  maplibre_native_ffi_edge_insets_read(
    &out->frustum_offset, &in->frustum_offset
  );
}

MaplibreNativeFfiMapViewport* maplibre_native_ffi_map_viewport_from_native(
  const mln_map_viewport* in
) {
  MaplibreNativeFfiMapViewport* self = g_new0(MaplibreNativeFfiMapViewport, 1);
  maplibre_native_ffi_map_viewport_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMapView, maplibre_native_ffi_map_view,
  maplibre_native_ffi_map_view_copy, maplibre_native_ffi_map_view_free
)

MaplibreNativeFfiMapView* maplibre_native_ffi_map_view_copy(
  const MaplibreNativeFfiMapView* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_map_view_free(MaplibreNativeFfiMapView* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_map_view_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_map_view_copy(
    (const MaplibreNativeFfiMapView*)self
  );
}

void maplibre_native_ffi_map_view_to_native(
  const MaplibreNativeFfiMapView* self, mln_map_view* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_camera_to_native(&self->camera, &out->camera, call);
  maplibre_native_ffi_logical_extent_to_native(
    &self->extent, &out->extent, call
  );
  maplibre_native_ffi_projection_mode_to_native(
    &self->projection_mode, &out->projection_mode, call
  );
  maplibre_native_ffi_map_viewport_to_native(
    &self->viewport, &out->viewport, call
  );
}

void maplibre_native_ffi_map_view_read(
  MaplibreNativeFfiMapView* out, const mln_map_view* in
) {
  maplibre_native_ffi_camera_read(&out->camera, &in->camera);
  maplibre_native_ffi_logical_extent_read(&out->extent, &in->extent);
  maplibre_native_ffi_projection_mode_read(
    &out->projection_mode, &in->projection_mode
  );
  maplibre_native_ffi_map_viewport_read(&out->viewport, &in->viewport);
}

MaplibreNativeFfiMapView* maplibre_native_ffi_map_view_from_native(
  const mln_map_view* in
) {
  MaplibreNativeFfiMapView* self = g_new0(MaplibreNativeFfiMapView, 1);
  maplibre_native_ffi_map_view_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMapSnapshot, maplibre_native_ffi_map_snapshot,
  maplibre_native_ffi_map_snapshot_copy, maplibre_native_ffi_map_snapshot_free
)

MaplibreNativeFfiMapSnapshot* maplibre_native_ffi_map_snapshot_copy(
  const MaplibreNativeFfiMapSnapshot* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_map_snapshot_free(MaplibreNativeFfiMapSnapshot* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_map_snapshot_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_map_snapshot_copy(
    (const MaplibreNativeFfiMapSnapshot*)self
  );
}

void maplibre_native_ffi_map_snapshot_to_native(
  const MaplibreNativeFfiMapSnapshot* self, mln_map_snapshot* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->debug_flags = (mln_map_debug_flags)(self->debug_flags);
  out->generation = (uint64_t)(self->generation);
  out->fully_loaded = (self->fully_loaded) != FALSE;
  out->rendering_stats_view_enabled =
    (self->rendering_stats_view_enabled) != FALSE;
  out->gesture_in_progress = (self->gesture_in_progress) != FALSE;
  out->transition_in_progress = (self->transition_in_progress) != FALSE;
  out->event_mask = (mln_runtime_event_flags)(self->event_mask);
  out->latest_render_update_generation =
    (uint64_t)(self->latest_render_update_generation);
  maplibre_native_ffi_map_tile_settings_to_native(
    &self->tile, &out->tile, call
  );
  maplibre_native_ffi_camera_bounds_to_native(
    &self->bounds, &out->bounds, call
  );
  maplibre_native_ffi_free_camera_to_native(
    &self->free_camera, &out->free_camera, call
  );
  maplibre_native_ffi_map_view_to_native(&self->view, &out->view, call);
}

void maplibre_native_ffi_map_snapshot_read(
  MaplibreNativeFfiMapSnapshot* out, const mln_map_snapshot* in
) {
  out->debug_flags = (MaplibreNativeFfiMapDebugFlag)(in->debug_flags);
  out->generation = (guint64)(in->generation);
  out->fully_loaded = (in->fully_loaded) ? TRUE : FALSE;
  out->rendering_stats_view_enabled =
    (in->rendering_stats_view_enabled) ? TRUE : FALSE;
  out->gesture_in_progress = (in->gesture_in_progress) ? TRUE : FALSE;
  out->transition_in_progress = (in->transition_in_progress) ? TRUE : FALSE;
  out->event_mask = (MaplibreNativeFfiRuntimeEventFlag)(in->event_mask);
  out->latest_render_update_generation =
    (guint64)(in->latest_render_update_generation);
  maplibre_native_ffi_map_tile_settings_read(&out->tile, &in->tile);
  maplibre_native_ffi_camera_bounds_read(&out->bounds, &in->bounds);
  maplibre_native_ffi_free_camera_read(&out->free_camera, &in->free_camera);
  maplibre_native_ffi_map_view_read(&out->view, &in->view);
}

MaplibreNativeFfiMapSnapshot* maplibre_native_ffi_map_snapshot_from_native(
  const mln_map_snapshot* in
) {
  MaplibreNativeFfiMapSnapshot* self = g_new0(MaplibreNativeFfiMapSnapshot, 1);
  maplibre_native_ffi_map_snapshot_read(self, in);
  return self;
}

struct _MaplibreNativeFfiMapTileOptions {
  guint64 fields;
  guint32 prefetch_zoom_delta;
  gdouble lod_min_radius;
  gdouble lod_scale;
  gdouble lod_pitch_threshold;
  gdouble lod_zoom_shift;
  MaplibreNativeFfiTileLodMode lod_mode;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMapTileOptions, maplibre_native_ffi_map_tile_options,
  maplibre_native_ffi_map_tile_options_copy,
  maplibre_native_ffi_map_tile_options_free
)

MaplibreNativeFfiMapTileOptions* maplibre_native_ffi_map_tile_options_new(
  void
) {
  MaplibreNativeFfiMapTileOptions* self =
    g_new0(MaplibreNativeFfiMapTileOptions, 1);
  return self;
}

MaplibreNativeFfiMapTileOptions* maplibre_native_ffi_map_tile_options_copy(
  const MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiMapTileOptions* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_map_tile_options_free(
  MaplibreNativeFfiMapTileOptions* self
) {
  if (self == NULL) return;
  g_free(self);
}

guint32 maplibre_native_ffi_map_tile_options_get_prefetch_zoom_delta(
  const MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->prefetch_zoom_delta;
}

gboolean maplibre_native_ffi_map_tile_options_has_prefetch_zoom_delta(
  const MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0;
}

void maplibre_native_ffi_map_tile_options_set_prefetch_zoom_delta(
  MaplibreNativeFfiMapTileOptions* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->prefetch_zoom_delta = value;
  self->fields |= MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
}

gdouble maplibre_native_ffi_map_tile_options_get_lod_min_radius(
  const MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->lod_min_radius;
}

gboolean maplibre_native_ffi_map_tile_options_has_lod_min_radius(
  const MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0;
}

void maplibre_native_ffi_map_tile_options_set_lod_min_radius(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->lod_min_radius = value;
  self->fields |= MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
}

gdouble maplibre_native_ffi_map_tile_options_get_lod_scale(
  const MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->lod_scale;
}

gboolean maplibre_native_ffi_map_tile_options_has_lod_scale(
  const MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_MAP_TILE_OPTION_LOD_SCALE) != 0;
}

void maplibre_native_ffi_map_tile_options_set_lod_scale(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->lod_scale = value;
  self->fields |= MLN_MAP_TILE_OPTION_LOD_SCALE;
}

gdouble maplibre_native_ffi_map_tile_options_get_lod_pitch_threshold(
  const MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->lod_pitch_threshold;
}

gboolean maplibre_native_ffi_map_tile_options_has_lod_pitch_threshold(
  const MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0;
}

void maplibre_native_ffi_map_tile_options_set_lod_pitch_threshold(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->lod_pitch_threshold = value;
  self->fields |= MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
}

gdouble maplibre_native_ffi_map_tile_options_get_lod_zoom_shift(
  const MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->lod_zoom_shift;
}

gboolean maplibre_native_ffi_map_tile_options_has_lod_zoom_shift(
  const MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0;
}

void maplibre_native_ffi_map_tile_options_set_lod_zoom_shift(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->lod_zoom_shift = value;
  self->fields |= MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
}

MaplibreNativeFfiTileLodMode maplibre_native_ffi_map_tile_options_get_lod_mode(
  const MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiTileLodMode)0);
  return self->lod_mode;
}

gboolean maplibre_native_ffi_map_tile_options_has_lod_mode(
  const MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_MAP_TILE_OPTION_LOD_MODE) != 0;
}

void maplibre_native_ffi_map_tile_options_set_lod_mode(
  MaplibreNativeFfiMapTileOptions* self, MaplibreNativeFfiTileLodMode value
) {
  g_return_if_fail(self != NULL);
  self->lod_mode = value;
  self->fields |= MLN_MAP_TILE_OPTION_LOD_MODE;
}

gpointer maplibre_native_ffi_map_tile_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_map_tile_options_copy(
    (const MaplibreNativeFfiMapTileOptions*)self
  );
}

void maplibre_native_ffi_map_tile_options_to_native(
  const MaplibreNativeFfiMapTileOptions* self, mln_map_tile_options* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_map_tile_option_fields)self->fields;
  out->prefetch_zoom_delta = (uint32_t)(self->prefetch_zoom_delta);
  out->lod_min_radius = (double)(self->lod_min_radius);
  out->lod_scale = (double)(self->lod_scale);
  out->lod_pitch_threshold = (double)(self->lod_pitch_threshold);
  out->lod_zoom_shift = (double)(self->lod_zoom_shift);
  out->lod_mode = (mln_tile_lod_mode)(self->lod_mode);
}

MaplibreNativeFfiMapTileOptions*
maplibre_native_ffi_map_tile_options_from_native(
  const mln_map_tile_options* in
) {
  MaplibreNativeFfiMapTileOptions* self =
    maplibre_native_ffi_map_tile_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0) {
    self->prefetch_zoom_delta = (guint32)(in->prefetch_zoom_delta);
  }
  if ((in->fields & MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0) {
    self->lod_min_radius = (gdouble)(in->lod_min_radius);
  }
  if ((in->fields & MLN_MAP_TILE_OPTION_LOD_SCALE) != 0) {
    self->lod_scale = (gdouble)(in->lod_scale);
  }
  if ((in->fields & MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0) {
    self->lod_pitch_threshold = (gdouble)(in->lod_pitch_threshold);
  }
  if ((in->fields & MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0) {
    self->lod_zoom_shift = (gdouble)(in->lod_zoom_shift);
  }
  if ((in->fields & MLN_MAP_TILE_OPTION_LOD_MODE) != 0) {
    self->lod_mode = (MaplibreNativeFfiTileLodMode)(in->lod_mode);
  }
  return self;
}

struct _MaplibreNativeFfiMapViewportOptions {
  guint64 fields;
  MaplibreNativeFfiNorthOrientation north_orientation;
  MaplibreNativeFfiConstrainMode constrain_mode;
  MaplibreNativeFfiViewportMode viewport_mode;
  MaplibreNativeFfiEdgeInsets frustum_offset;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMapViewportOptions, maplibre_native_ffi_map_viewport_options,
  maplibre_native_ffi_map_viewport_options_copy,
  maplibre_native_ffi_map_viewport_options_free
)

MaplibreNativeFfiMapViewportOptions*
maplibre_native_ffi_map_viewport_options_new(void) {
  MaplibreNativeFfiMapViewportOptions* self =
    g_new0(MaplibreNativeFfiMapViewportOptions, 1);
  return self;
}

MaplibreNativeFfiMapViewportOptions*
maplibre_native_ffi_map_viewport_options_copy(
  const MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiMapViewportOptions* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_map_viewport_options_free(
  MaplibreNativeFfiMapViewportOptions* self
) {
  if (self == NULL) return;
  g_free(self);
}

MaplibreNativeFfiNorthOrientation
maplibre_native_ffi_map_viewport_options_get_north_orientation(
  const MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiNorthOrientation)0);
  return self->north_orientation;
}

gboolean maplibre_native_ffi_map_viewport_options_has_north_orientation(
  const MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0;
}

void maplibre_native_ffi_map_viewport_options_set_north_orientation(
  MaplibreNativeFfiMapViewportOptions* self,
  MaplibreNativeFfiNorthOrientation value
) {
  g_return_if_fail(self != NULL);
  self->north_orientation = value;
  self->fields |= MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
}

MaplibreNativeFfiConstrainMode
maplibre_native_ffi_map_viewport_options_get_constrain_mode(
  const MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiConstrainMode)0);
  return self->constrain_mode;
}

gboolean maplibre_native_ffi_map_viewport_options_has_constrain_mode(
  const MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0;
}

void maplibre_native_ffi_map_viewport_options_set_constrain_mode(
  MaplibreNativeFfiMapViewportOptions* self,
  MaplibreNativeFfiConstrainMode value
) {
  g_return_if_fail(self != NULL);
  self->constrain_mode = value;
  self->fields |= MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
}

MaplibreNativeFfiViewportMode
maplibre_native_ffi_map_viewport_options_get_viewport_mode(
  const MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiViewportMode)0);
  return self->viewport_mode;
}

gboolean maplibre_native_ffi_map_viewport_options_has_viewport_mode(
  const MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0;
}

void maplibre_native_ffi_map_viewport_options_set_viewport_mode(
  MaplibreNativeFfiMapViewportOptions* self, MaplibreNativeFfiViewportMode value
) {
  g_return_if_fail(self != NULL);
  self->viewport_mode = value;
  self->fields |= MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
}

MaplibreNativeFfiEdgeInsets*
maplibre_native_ffi_map_viewport_options_get_frustum_offset(
  const MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiEdgeInsets*)&self->frustum_offset;
}

gboolean maplibre_native_ffi_map_viewport_options_has_frustum_offset(
  const MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0;
}

void maplibre_native_ffi_map_viewport_options_set_frustum_offset(
  MaplibreNativeFfiMapViewportOptions* self,
  const MaplibreNativeFfiEdgeInsets* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->frustum_offset = *value;
  self->fields |= MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
}

gpointer maplibre_native_ffi_map_viewport_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_map_viewport_options_copy(
    (const MaplibreNativeFfiMapViewportOptions*)self
  );
}

void maplibre_native_ffi_map_viewport_options_to_native(
  const MaplibreNativeFfiMapViewportOptions* self,
  mln_map_viewport_options* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_map_viewport_option_fields)self->fields;
  out->north_orientation = (mln_north_orientation)(self->north_orientation);
  out->constrain_mode = (mln_constrain_mode)(self->constrain_mode);
  out->viewport_mode = (mln_viewport_mode)(self->viewport_mode);
  maplibre_native_ffi_edge_insets_to_native(
    &self->frustum_offset, &out->frustum_offset, call
  );
}

MaplibreNativeFfiMapViewportOptions*
maplibre_native_ffi_map_viewport_options_from_native(
  const mln_map_viewport_options* in
) {
  MaplibreNativeFfiMapViewportOptions* self =
    maplibre_native_ffi_map_viewport_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0) {
    self->north_orientation =
      (MaplibreNativeFfiNorthOrientation)(in->north_orientation);
  }
  if ((in->fields & MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0) {
    self->constrain_mode = (MaplibreNativeFfiConstrainMode)(in->constrain_mode);
  }
  if ((in->fields & MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0) {
    self->viewport_mode = (MaplibreNativeFfiViewportMode)(in->viewport_mode);
  }
  if ((in->fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0) {
    maplibre_native_ffi_edge_insets_read(
      &self->frustum_offset, &in->frustum_offset
    );
  }
  return self;
}

struct _MaplibreNativeFfiMetalBorrowedTexture {
  gpointer texture;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMetalBorrowedTexture,
  maplibre_native_ffi_metal_borrowed_texture,
  maplibre_native_ffi_metal_borrowed_texture_copy,
  maplibre_native_ffi_metal_borrowed_texture_free
)

MaplibreNativeFfiMetalBorrowedTexture*
maplibre_native_ffi_metal_borrowed_texture_new(void) {
  MaplibreNativeFfiMetalBorrowedTexture* self =
    g_new0(MaplibreNativeFfiMetalBorrowedTexture, 1);
  return self;
}

MaplibreNativeFfiMetalBorrowedTexture*
maplibre_native_ffi_metal_borrowed_texture_copy(
  const MaplibreNativeFfiMetalBorrowedTexture* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiMetalBorrowedTexture* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_metal_borrowed_texture_free(
  MaplibreNativeFfiMetalBorrowedTexture* self
) {
  if (self == NULL) return;
  g_free(self);
}

gpointer maplibre_native_ffi_metal_borrowed_texture_get_texture(
  const MaplibreNativeFfiMetalBorrowedTexture* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->texture;
}

void maplibre_native_ffi_metal_borrowed_texture_set_texture(
  MaplibreNativeFfiMetalBorrowedTexture* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->texture = value;
}

gpointer maplibre_native_ffi_metal_borrowed_texture_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_metal_borrowed_texture_copy(
    (const MaplibreNativeFfiMetalBorrowedTexture*)self
  );
}

void maplibre_native_ffi_metal_borrowed_texture_to_native(
  const MaplibreNativeFfiMetalBorrowedTexture* self,
  mln_metal_borrowed_texture* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->texture = (void*)(self->texture);
}

MaplibreNativeFfiMetalBorrowedTexture*
maplibre_native_ffi_metal_borrowed_texture_from_native(
  const mln_metal_borrowed_texture* in
) {
  MaplibreNativeFfiMetalBorrowedTexture* self =
    maplibre_native_ffi_metal_borrowed_texture_new();
  self->texture = (gpointer)(in->texture);
  return self;
}

struct _MaplibreNativeFfiMetalBorrowedTextureDescriptor {
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 physical_width;
  guint32 physical_height;
  GPtrArray* textures;
  gpointer command_queue;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor,
  maplibre_native_ffi_metal_borrowed_texture_descriptor,
  maplibre_native_ffi_metal_borrowed_texture_descriptor_copy,
  maplibre_native_ffi_metal_borrowed_texture_descriptor_free
)

MaplibreNativeFfiMetalBorrowedTextureDescriptor*
maplibre_native_ffi_metal_borrowed_texture_descriptor_new(void) {
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self =
    g_new0(MaplibreNativeFfiMetalBorrowedTextureDescriptor, 1);
  self->textures = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_metal_borrowed_texture_free
  );
  return self;
}

MaplibreNativeFfiMetalBorrowedTextureDescriptor*
maplibre_native_ffi_metal_borrowed_texture_descriptor_copy(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* copy =
    g_memdup2(self, sizeof *self);
  copy->textures =
    self->textures != NULL
      ? g_ptr_array_copy(
          self->textures, maplibre_native_ffi_metal_borrowed_texture_copy_func,
          NULL
        )
      : NULL;
  if (copy->textures != NULL)
    g_ptr_array_set_free_func(
      copy->textures,
      (GDestroyNotify)maplibre_native_ffi_metal_borrowed_texture_free
    );
  return copy;
}

void maplibre_native_ffi_metal_borrowed_texture_descriptor_free(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_logical_size(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_logical_size(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

guint32
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_physical_width(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->physical_width;
}

void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_width(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->physical_width = value;
}

guint32
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_physical_height(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->physical_height;
}

void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_height(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->physical_height = value;
}

GPtrArray* maplibre_native_ffi_metal_borrowed_texture_descriptor_get_textures(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->textures;
}

void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_textures(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self,
  MaplibreNativeFfiMetalBorrowedTexture* const* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->textures;
  self->textures = g_ptr_array_new_full(
    (guint)value_count,
    (GDestroyNotify)maplibre_native_ffi_metal_borrowed_texture_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->textures,
      maplibre_native_ffi_metal_borrowed_texture_copy(value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

gpointer
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_command_queue(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->command_queue;
}

void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_command_queue(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->command_queue = value;
}

gpointer maplibre_native_ffi_metal_borrowed_texture_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_metal_borrowed_texture_descriptor_copy(
    (const MaplibreNativeFfiMetalBorrowedTextureDescriptor*)self
  );
}

void maplibre_native_ffi_metal_borrowed_texture_descriptor_to_native(
  const MaplibreNativeFfiMetalBorrowedTextureDescriptor* self,
  mln_metal_borrowed_texture_descriptor* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  out->physical_width = (uint32_t)(self->physical_width);
  out->physical_height = (uint32_t)(self->physical_height);
  if (self->textures != NULL && self->textures->len > 0) {
    mln_metal_borrowed_texture* items =
      maplibre_native_ffi_call_alloc(call, self->textures->len, sizeof *items);
    for (guint index = 0; index < self->textures->len; index++) {
      maplibre_native_ffi_metal_borrowed_texture_to_native(
        self->textures->pdata[index], &items[index], call
      );
    }
    out->textures = items;
    out->texture_count = self->textures->len;
  }
  out->command_queue = (void*)(self->command_queue);
}

MaplibreNativeFfiMetalBorrowedTextureDescriptor*
maplibre_native_ffi_metal_borrowed_texture_descriptor_from_native(
  const mln_metal_borrowed_texture_descriptor* in
) {
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self =
    maplibre_native_ffi_metal_borrowed_texture_descriptor_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  self->physical_width = (guint32)(in->physical_width);
  self->physical_height = (guint32)(in->physical_height);
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  self->textures = g_ptr_array_new_full(
    (guint)in->texture_count,
    (GDestroyNotify)maplibre_native_ffi_metal_borrowed_texture_free
  );
  for (size_t index = 0; index < in->texture_count; index++) {
    g_ptr_array_add(
      self->textures, maplibre_native_ffi_metal_borrowed_texture_from_native(
                        &in->textures[index]
                      )
    );
  }
  self->command_queue = (gpointer)(in->command_queue);
  return self;
}

struct _MaplibreNativeFfiMetalBorrowedTextureReplacement {
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 physical_width;
  guint32 physical_height;
  GPtrArray* textures;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMetalBorrowedTextureReplacement,
  maplibre_native_ffi_metal_borrowed_texture_replacement,
  maplibre_native_ffi_metal_borrowed_texture_replacement_copy,
  maplibre_native_ffi_metal_borrowed_texture_replacement_free
)

MaplibreNativeFfiMetalBorrowedTextureReplacement*
maplibre_native_ffi_metal_borrowed_texture_replacement_new(void) {
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self =
    g_new0(MaplibreNativeFfiMetalBorrowedTextureReplacement, 1);
  self->textures = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_metal_borrowed_texture_free
  );
  return self;
}

MaplibreNativeFfiMetalBorrowedTextureReplacement*
maplibre_native_ffi_metal_borrowed_texture_replacement_copy(
  const MaplibreNativeFfiMetalBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiMetalBorrowedTextureReplacement* copy =
    g_memdup2(self, sizeof *self);
  copy->textures =
    self->textures != NULL
      ? g_ptr_array_copy(
          self->textures, maplibre_native_ffi_metal_borrowed_texture_copy_func,
          NULL
        )
      : NULL;
  if (copy->textures != NULL)
    g_ptr_array_set_free_func(
      copy->textures,
      (GDestroyNotify)maplibre_native_ffi_metal_borrowed_texture_free
    );
  return copy;
}

void maplibre_native_ffi_metal_borrowed_texture_replacement_free(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_borrowed_texture_replacement_get_logical_size(
  const MaplibreNativeFfiMetalBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_metal_borrowed_texture_replacement_set_logical_size(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

guint32
maplibre_native_ffi_metal_borrowed_texture_replacement_get_physical_width(
  const MaplibreNativeFfiMetalBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->physical_width;
}

void maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_width(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->physical_width = value;
}

guint32
maplibre_native_ffi_metal_borrowed_texture_replacement_get_physical_height(
  const MaplibreNativeFfiMetalBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->physical_height;
}

void maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_height(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->physical_height = value;
}

GPtrArray* maplibre_native_ffi_metal_borrowed_texture_replacement_get_textures(
  const MaplibreNativeFfiMetalBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->textures;
}

void maplibre_native_ffi_metal_borrowed_texture_replacement_set_textures(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self,
  MaplibreNativeFfiMetalBorrowedTexture* const* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->textures;
  self->textures = g_ptr_array_new_full(
    (guint)value_count,
    (GDestroyNotify)maplibre_native_ffi_metal_borrowed_texture_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->textures,
      maplibre_native_ffi_metal_borrowed_texture_copy(value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

gpointer maplibre_native_ffi_metal_borrowed_texture_replacement_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_metal_borrowed_texture_replacement_copy(
    (const MaplibreNativeFfiMetalBorrowedTextureReplacement*)self
  );
}

void maplibre_native_ffi_metal_borrowed_texture_replacement_to_native(
  const MaplibreNativeFfiMetalBorrowedTextureReplacement* self,
  mln_metal_borrowed_texture_replacement* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  out->physical_width = (uint32_t)(self->physical_width);
  out->physical_height = (uint32_t)(self->physical_height);
  if (self->textures != NULL && self->textures->len > 0) {
    mln_metal_borrowed_texture* items =
      maplibre_native_ffi_call_alloc(call, self->textures->len, sizeof *items);
    for (guint index = 0; index < self->textures->len; index++) {
      maplibre_native_ffi_metal_borrowed_texture_to_native(
        self->textures->pdata[index], &items[index], call
      );
    }
    out->textures = items;
    out->texture_count = self->textures->len;
  }
}

MaplibreNativeFfiMetalBorrowedTextureReplacement*
maplibre_native_ffi_metal_borrowed_texture_replacement_from_native(
  const mln_metal_borrowed_texture_replacement* in
) {
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self =
    maplibre_native_ffi_metal_borrowed_texture_replacement_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  self->physical_width = (guint32)(in->physical_width);
  self->physical_height = (guint32)(in->physical_height);
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  self->textures = g_ptr_array_new_full(
    (guint)in->texture_count,
    (GDestroyNotify)maplibre_native_ffi_metal_borrowed_texture_free
  );
  for (size_t index = 0; index < in->texture_count; index++) {
    g_ptr_array_add(
      self->textures, maplibre_native_ffi_metal_borrowed_texture_from_native(
                        &in->textures[index]
                      )
    );
  }
  return self;
}

struct _MaplibreNativeFfiMetalSurfaceDescriptor {
  MaplibreNativeFfiLogicalSize logical_size;
  MaplibreNativeFfiMetalContextDescriptor* context;
  gpointer layer;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMetalSurfaceDescriptor,
  maplibre_native_ffi_metal_surface_descriptor,
  maplibre_native_ffi_metal_surface_descriptor_copy,
  maplibre_native_ffi_metal_surface_descriptor_free
)

MaplibreNativeFfiMetalSurfaceDescriptor*
maplibre_native_ffi_metal_surface_descriptor_new(void) {
  MaplibreNativeFfiMetalSurfaceDescriptor* self =
    g_new0(MaplibreNativeFfiMetalSurfaceDescriptor, 1);
  self->context = maplibre_native_ffi_metal_context_descriptor_new();
  return self;
}

MaplibreNativeFfiMetalSurfaceDescriptor*
maplibre_native_ffi_metal_surface_descriptor_copy(
  const MaplibreNativeFfiMetalSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiMetalSurfaceDescriptor* copy = g_memdup2(self, sizeof *self);
  copy->context =
    self->context != NULL
      ? maplibre_native_ffi_metal_context_descriptor_copy(self->context)
      : NULL;
  return copy;
}

void maplibre_native_ffi_metal_surface_descriptor_free(
  MaplibreNativeFfiMetalSurfaceDescriptor* self
) {
  if (self == NULL) return;
  g_clear_pointer(
    &self->context, maplibre_native_ffi_metal_context_descriptor_free
  );
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_surface_descriptor_get_logical_size(
  const MaplibreNativeFfiMetalSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_metal_surface_descriptor_set_logical_size(
  MaplibreNativeFfiMetalSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_metal_surface_descriptor_get_context(
  const MaplibreNativeFfiMetalSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->context;
}

void maplibre_native_ffi_metal_surface_descriptor_set_context(
  MaplibreNativeFfiMetalSurfaceDescriptor* self,
  const MaplibreNativeFfiMetalContextDescriptor* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiMetalContextDescriptor* previous = self->context;
  self->context = maplibre_native_ffi_metal_context_descriptor_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_metal_context_descriptor_free(previous);
}

gpointer maplibre_native_ffi_metal_surface_descriptor_get_layer(
  const MaplibreNativeFfiMetalSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->layer;
}

void maplibre_native_ffi_metal_surface_descriptor_set_layer(
  MaplibreNativeFfiMetalSurfaceDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->layer = value;
}

gpointer maplibre_native_ffi_metal_surface_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_metal_surface_descriptor_copy(
    (const MaplibreNativeFfiMetalSurfaceDescriptor*)self
  );
}

void maplibre_native_ffi_metal_surface_descriptor_to_native(
  const MaplibreNativeFfiMetalSurfaceDescriptor* self,
  mln_metal_surface_descriptor* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  if (self->context != NULL)
    maplibre_native_ffi_metal_context_descriptor_to_native(
      self->context, &out->context, call
    );
  out->layer = (void*)(self->layer);
}

MaplibreNativeFfiMetalSurfaceDescriptor*
maplibre_native_ffi_metal_surface_descriptor_from_native(
  const mln_metal_surface_descriptor* in
) {
  MaplibreNativeFfiMetalSurfaceDescriptor* self =
    maplibre_native_ffi_metal_surface_descriptor_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  g_clear_pointer(
    &self->context, maplibre_native_ffi_metal_context_descriptor_free
  );
  self->context =
    maplibre_native_ffi_metal_context_descriptor_from_native(&in->context);
  self->layer = (gpointer)(in->layer);
  return self;
}

struct _MaplibreNativeFfiMetalSurfaceReplacement {
  MaplibreNativeFfiLogicalSize logical_size;
  gpointer layer;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMetalSurfaceReplacement,
  maplibre_native_ffi_metal_surface_replacement,
  maplibre_native_ffi_metal_surface_replacement_copy,
  maplibre_native_ffi_metal_surface_replacement_free
)

MaplibreNativeFfiMetalSurfaceReplacement*
maplibre_native_ffi_metal_surface_replacement_new(void) {
  MaplibreNativeFfiMetalSurfaceReplacement* self =
    g_new0(MaplibreNativeFfiMetalSurfaceReplacement, 1);
  return self;
}

MaplibreNativeFfiMetalSurfaceReplacement*
maplibre_native_ffi_metal_surface_replacement_copy(
  const MaplibreNativeFfiMetalSurfaceReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiMetalSurfaceReplacement* copy =
    g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_metal_surface_replacement_free(
  MaplibreNativeFfiMetalSurfaceReplacement* self
) {
  if (self == NULL) return;
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_surface_replacement_get_logical_size(
  const MaplibreNativeFfiMetalSurfaceReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_metal_surface_replacement_set_logical_size(
  MaplibreNativeFfiMetalSurfaceReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

gpointer maplibre_native_ffi_metal_surface_replacement_get_layer(
  const MaplibreNativeFfiMetalSurfaceReplacement* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->layer;
}

void maplibre_native_ffi_metal_surface_replacement_set_layer(
  MaplibreNativeFfiMetalSurfaceReplacement* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->layer = value;
}

gpointer maplibre_native_ffi_metal_surface_replacement_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_metal_surface_replacement_copy(
    (const MaplibreNativeFfiMetalSurfaceReplacement*)self
  );
}

void maplibre_native_ffi_metal_surface_replacement_to_native(
  const MaplibreNativeFfiMetalSurfaceReplacement* self,
  mln_metal_surface_replacement* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  out->layer = (void*)(self->layer);
}

MaplibreNativeFfiMetalSurfaceReplacement*
maplibre_native_ffi_metal_surface_replacement_from_native(
  const mln_metal_surface_replacement* in
) {
  MaplibreNativeFfiMetalSurfaceReplacement* self =
    maplibre_native_ffi_metal_surface_replacement_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  self->layer = (gpointer)(in->layer);
  return self;
}

struct _MaplibreNativeFfiMetalTextureFrame {
  guint32 width;
  guint32 height;
  gdouble scale_factor;
  guint32 slot;
  gpointer texture;
  gpointer device;
  guint64 pixel_format;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiMetalTextureFrame, maplibre_native_ffi_metal_texture_frame,
  maplibre_native_ffi_metal_texture_frame_copy,
  maplibre_native_ffi_metal_texture_frame_free
)

MaplibreNativeFfiMetalTextureFrame* maplibre_native_ffi_metal_texture_frame_new(
  void
) {
  MaplibreNativeFfiMetalTextureFrame* self =
    g_new0(MaplibreNativeFfiMetalTextureFrame, 1);
  return self;
}

MaplibreNativeFfiMetalTextureFrame*
maplibre_native_ffi_metal_texture_frame_copy(
  const MaplibreNativeFfiMetalTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiMetalTextureFrame* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_metal_texture_frame_free(
  MaplibreNativeFfiMetalTextureFrame* self
) {
  if (self == NULL) return;
  g_free(self);
}

guint32 maplibre_native_ffi_metal_texture_frame_get_width(
  const MaplibreNativeFfiMetalTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->width;
}

guint32 maplibre_native_ffi_metal_texture_frame_get_height(
  const MaplibreNativeFfiMetalTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->height;
}

gdouble maplibre_native_ffi_metal_texture_frame_get_scale_factor(
  const MaplibreNativeFfiMetalTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->scale_factor;
}

guint32 maplibre_native_ffi_metal_texture_frame_get_slot(
  const MaplibreNativeFfiMetalTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->slot;
}

gpointer maplibre_native_ffi_metal_texture_frame_get_texture(
  const MaplibreNativeFfiMetalTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->texture;
}

gpointer maplibre_native_ffi_metal_texture_frame_get_device(
  const MaplibreNativeFfiMetalTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->device;
}

guint64 maplibre_native_ffi_metal_texture_frame_get_pixel_format(
  const MaplibreNativeFfiMetalTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->pixel_format;
}

gpointer maplibre_native_ffi_metal_texture_frame_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_metal_texture_frame_copy(
    (const MaplibreNativeFfiMetalTextureFrame*)self
  );
}

void maplibre_native_ffi_metal_texture_frame_to_native(
  const MaplibreNativeFfiMetalTextureFrame* self, mln_metal_texture_frame* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->width = (uint32_t)(self->width);
  out->height = (uint32_t)(self->height);
  out->scale_factor = (double)(self->scale_factor);
  out->slot = (uint32_t)(self->slot);
  out->texture = (void*)(self->texture);
  out->device = (void*)(self->device);
  out->pixel_format = (uint64_t)(self->pixel_format);
}

MaplibreNativeFfiMetalTextureFrame*
maplibre_native_ffi_metal_texture_frame_from_native(
  const mln_metal_texture_frame* in
) {
  MaplibreNativeFfiMetalTextureFrame* self =
    maplibre_native_ffi_metal_texture_frame_new();
  self->width = (guint32)(in->width);
  self->height = (guint32)(in->height);
  self->scale_factor = (gdouble)(in->scale_factor);
  self->slot = (guint32)(in->slot);
  self->texture = (gpointer)(in->texture);
  self->device = (gpointer)(in->device);
  self->pixel_format = (guint64)(in->pixel_format);
  return self;
}

struct _MaplibreNativeFfiOfflineGeometryRegionDefinition {
  char* style_url;
  char* geometry;
  gdouble min_zoom;
  gdouble max_zoom;
  gfloat pixel_ratio;
  gboolean include_ideographs;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOfflineGeometryRegionDefinition,
  maplibre_native_ffi_offline_geometry_region_definition,
  maplibre_native_ffi_offline_geometry_region_definition_copy,
  maplibre_native_ffi_offline_geometry_region_definition_free
)

MaplibreNativeFfiOfflineGeometryRegionDefinition*
maplibre_native_ffi_offline_geometry_region_definition_new(void) {
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self =
    g_new0(MaplibreNativeFfiOfflineGeometryRegionDefinition, 1);
  self->style_url = g_strdup("");
  self->geometry = g_strdup("");
  return self;
}

MaplibreNativeFfiOfflineGeometryRegionDefinition*
maplibre_native_ffi_offline_geometry_region_definition_copy(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiOfflineGeometryRegionDefinition* copy =
    g_memdup2(self, sizeof *self);
  copy->style_url = g_strdup(self->style_url);
  copy->geometry = g_strdup(self->geometry);
  return copy;
}

void maplibre_native_ffi_offline_geometry_region_definition_free(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  if (self == NULL) return;
  g_free(self->style_url);
  g_free(self->geometry);
  g_free(self);
}

const char*
maplibre_native_ffi_offline_geometry_region_definition_get_style_url(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->style_url;
}

void maplibre_native_ffi_offline_geometry_region_definition_set_style_url(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->style_url;
  self->style_url = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

const char* maplibre_native_ffi_offline_geometry_region_definition_get_geometry(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->geometry;
}

void maplibre_native_ffi_offline_geometry_region_definition_set_geometry(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->geometry;
  self->geometry = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

gdouble maplibre_native_ffi_offline_geometry_region_definition_get_min_zoom(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->min_zoom;
}

void maplibre_native_ffi_offline_geometry_region_definition_set_min_zoom(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->min_zoom = value;
}

gdouble maplibre_native_ffi_offline_geometry_region_definition_get_max_zoom(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->max_zoom;
}

void maplibre_native_ffi_offline_geometry_region_definition_set_max_zoom(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->max_zoom = value;
}

gfloat maplibre_native_ffi_offline_geometry_region_definition_get_pixel_ratio(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->pixel_ratio;
}

void maplibre_native_ffi_offline_geometry_region_definition_set_pixel_ratio(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gfloat value
) {
  g_return_if_fail(self != NULL);
  self->pixel_ratio = value;
}

gboolean
maplibre_native_ffi_offline_geometry_region_definition_get_include_ideographs(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->include_ideographs;
}

void maplibre_native_ffi_offline_geometry_region_definition_set_include_ideographs(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->include_ideographs = value;
}

gpointer maplibre_native_ffi_offline_geometry_region_definition_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_offline_geometry_region_definition_copy(
    (const MaplibreNativeFfiOfflineGeometryRegionDefinition*)self
  );
}

void maplibre_native_ffi_offline_geometry_region_definition_to_native(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self,
  mln_offline_geometry_region_definition* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->style_url = maplibre_native_ffi_text_view(self->style_url);
  out->geometry = maplibre_native_ffi_text_view(self->geometry);
  out->min_zoom = (double)(self->min_zoom);
  out->max_zoom = (double)(self->max_zoom);
  out->pixel_ratio = (float)(self->pixel_ratio);
  out->include_ideographs = (self->include_ideographs) != FALSE;
}

MaplibreNativeFfiOfflineGeometryRegionDefinition*
maplibre_native_ffi_offline_geometry_region_definition_from_native(
  const mln_offline_geometry_region_definition* in
) {
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self =
    maplibre_native_ffi_offline_geometry_region_definition_new();
  g_free(self->style_url);
  self->style_url = maplibre_native_ffi_text_new(in->style_url);
  g_free(self->geometry);
  self->geometry = maplibre_native_ffi_text_new(in->geometry);
  self->min_zoom = (gdouble)(in->min_zoom);
  self->max_zoom = (gdouble)(in->max_zoom);
  self->pixel_ratio = (gfloat)(in->pixel_ratio);
  self->include_ideographs = (in->include_ideographs) ? TRUE : FALSE;
  return self;
}

struct _MaplibreNativeFfiOfflineTilePyramidRegionDefinition {
  char* style_url;
  MaplibreNativeFfiLatLngBounds bounds;
  gdouble min_zoom;
  gdouble max_zoom;
  gfloat pixel_ratio;
  gboolean include_ideographs;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition,
  maplibre_native_ffi_offline_tile_pyramid_region_definition,
  maplibre_native_ffi_offline_tile_pyramid_region_definition_copy,
  maplibre_native_ffi_offline_tile_pyramid_region_definition_free
)

MaplibreNativeFfiOfflineTilePyramidRegionDefinition*
maplibre_native_ffi_offline_tile_pyramid_region_definition_new(void) {
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self =
    g_new0(MaplibreNativeFfiOfflineTilePyramidRegionDefinition, 1);
  self->style_url = g_strdup("");
  return self;
}

MaplibreNativeFfiOfflineTilePyramidRegionDefinition*
maplibre_native_ffi_offline_tile_pyramid_region_definition_copy(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* copy =
    g_memdup2(self, sizeof *self);
  copy->style_url = g_strdup(self->style_url);
  return copy;
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_free(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  if (self == NULL) return;
  g_free(self->style_url);
  g_free(self);
}

const char*
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_style_url(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->style_url;
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_style_url(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->style_url;
  self->style_url = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

MaplibreNativeFfiLatLngBounds*
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_bounds(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLatLngBounds*)&self->bounds;
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_bounds(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self,
  const MaplibreNativeFfiLatLngBounds* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->bounds = *value;
}

gdouble maplibre_native_ffi_offline_tile_pyramid_region_definition_get_min_zoom(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->min_zoom;
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_min_zoom(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->min_zoom = value;
}

gdouble maplibre_native_ffi_offline_tile_pyramid_region_definition_get_max_zoom(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->max_zoom;
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_max_zoom(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->max_zoom = value;
}

gfloat
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_pixel_ratio(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->pixel_ratio;
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_pixel_ratio(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gfloat value
) {
  g_return_if_fail(self != NULL);
  self->pixel_ratio = value;
}

gboolean
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_include_ideographs(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->include_ideographs;
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_include_ideographs(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->include_ideographs = value;
}

gpointer maplibre_native_ffi_offline_tile_pyramid_region_definition_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_offline_tile_pyramid_region_definition_copy(
    (const MaplibreNativeFfiOfflineTilePyramidRegionDefinition*)self
  );
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_to_native(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self,
  mln_offline_tile_pyramid_region_definition* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->style_url = maplibre_native_ffi_text_view(self->style_url);
  maplibre_native_ffi_lat_lng_bounds_to_native(
    &self->bounds, &out->bounds, call
  );
  out->min_zoom = (double)(self->min_zoom);
  out->max_zoom = (double)(self->max_zoom);
  out->pixel_ratio = (float)(self->pixel_ratio);
  out->include_ideographs = (self->include_ideographs) != FALSE;
}

MaplibreNativeFfiOfflineTilePyramidRegionDefinition*
maplibre_native_ffi_offline_tile_pyramid_region_definition_from_native(
  const mln_offline_tile_pyramid_region_definition* in
) {
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self =
    maplibre_native_ffi_offline_tile_pyramid_region_definition_new();
  g_free(self->style_url);
  self->style_url = maplibre_native_ffi_text_new(in->style_url);
  maplibre_native_ffi_lat_lng_bounds_read(&self->bounds, &in->bounds);
  self->min_zoom = (gdouble)(in->min_zoom);
  self->max_zoom = (gdouble)(in->max_zoom);
  self->pixel_ratio = (gfloat)(in->pixel_ratio);
  self->include_ideographs = (in->include_ideographs) ? TRUE : FALSE;
  return self;
}

struct _MaplibreNativeFfiOfflineRegionDefinition {
  gint64 tag;
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* tile_pyramid;
  MaplibreNativeFfiOfflineGeometryRegionDefinition* geometry;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOfflineRegionDefinition,
  maplibre_native_ffi_offline_region_definition,
  maplibre_native_ffi_offline_region_definition_copy,
  maplibre_native_ffi_offline_region_definition_free
)

MaplibreNativeFfiOfflineRegionDefinition*
maplibre_native_ffi_offline_region_definition_copy(
  const MaplibreNativeFfiOfflineRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiOfflineRegionDefinition* copy =
    g_memdup2(self, sizeof *self);
  copy->tile_pyramid =
    self->tile_pyramid != NULL
      ? maplibre_native_ffi_offline_tile_pyramid_region_definition_copy(
          self->tile_pyramid
        )
      : NULL;
  copy->geometry =
    self->geometry != NULL
      ? maplibre_native_ffi_offline_geometry_region_definition_copy(
          self->geometry
        )
      : NULL;
  return copy;
}

void maplibre_native_ffi_offline_region_definition_free(
  MaplibreNativeFfiOfflineRegionDefinition* self
) {
  if (self == NULL) return;
  g_clear_pointer(
    &self->tile_pyramid,
    maplibre_native_ffi_offline_tile_pyramid_region_definition_free
  );
  g_clear_pointer(
    &self->geometry, maplibre_native_ffi_offline_geometry_region_definition_free
  );
  g_free(self);
}

MaplibreNativeFfiOfflineRegionDefinition*
maplibre_native_ffi_offline_region_definition_new_tile_pyramid(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* value
) {
  g_return_val_if_fail(value != NULL, NULL);
  MaplibreNativeFfiOfflineRegionDefinition* self =
    g_new0(MaplibreNativeFfiOfflineRegionDefinition, 1);
  self->tag = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
  self->tile_pyramid =
    maplibre_native_ffi_offline_tile_pyramid_region_definition_new();
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* previous =
    self->tile_pyramid;
  self->tile_pyramid =
    maplibre_native_ffi_offline_tile_pyramid_region_definition_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_offline_tile_pyramid_region_definition_free(previous);
  return self;
}

gboolean maplibre_native_ffi_offline_region_definition_is_tile_pyramid(
  const MaplibreNativeFfiOfflineRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
}

MaplibreNativeFfiOfflineTilePyramidRegionDefinition*
maplibre_native_ffi_offline_region_definition_get_tile_pyramid(
  const MaplibreNativeFfiOfflineRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(
    self->tag == MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID, NULL
  );
  return self->tile_pyramid;
}

MaplibreNativeFfiOfflineRegionDefinition*
maplibre_native_ffi_offline_region_definition_new_geometry(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* value
) {
  g_return_val_if_fail(value != NULL, NULL);
  MaplibreNativeFfiOfflineRegionDefinition* self =
    g_new0(MaplibreNativeFfiOfflineRegionDefinition, 1);
  self->tag = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
  self->geometry = maplibre_native_ffi_offline_geometry_region_definition_new();
  MaplibreNativeFfiOfflineGeometryRegionDefinition* previous = self->geometry;
  self->geometry =
    maplibre_native_ffi_offline_geometry_region_definition_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_offline_geometry_region_definition_free(previous);
  return self;
}

gboolean maplibre_native_ffi_offline_region_definition_is_geometry(
  const MaplibreNativeFfiOfflineRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
}

MaplibreNativeFfiOfflineGeometryRegionDefinition*
maplibre_native_ffi_offline_region_definition_get_geometry(
  const MaplibreNativeFfiOfflineRegionDefinition* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(
    self->tag == MLN_OFFLINE_REGION_DEFINITION_GEOMETRY, NULL
  );
  return self->geometry;
}

void maplibre_native_ffi_offline_region_definition_to_native(
  const MaplibreNativeFfiOfflineRegionDefinition* self,
  mln_offline_region_definition* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->type = (mln_offline_region_definition_type)self->tag;
  mln_offline_region_definition_data* data = &out->data;
  switch (self->tag) {
    case MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID:
      if (self->tile_pyramid != NULL)
        maplibre_native_ffi_offline_tile_pyramid_region_definition_to_native(
          self->tile_pyramid, &data->tile_pyramid, call
        );
      break;
    case MLN_OFFLINE_REGION_DEFINITION_GEOMETRY:
      if (self->geometry != NULL)
        maplibre_native_ffi_offline_geometry_region_definition_to_native(
          self->geometry, &data->geometry, call
        );
      break;
    default:
      break;
  }
}

MaplibreNativeFfiOfflineRegionDefinition*
maplibre_native_ffi_offline_region_definition_from_native(
  const mln_offline_region_definition* in
) {
  gint64 tag = (gint64)in->type;
  const mln_offline_region_definition_data* data = &in->data;
  MaplibreNativeFfiOfflineRegionDefinition* self =
    g_new0(MaplibreNativeFfiOfflineRegionDefinition, 1);
  self->tag = tag;
  switch (tag) {
    case MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID:
      self->tile_pyramid =
        maplibre_native_ffi_offline_tile_pyramid_region_definition_from_native(
          &data->tile_pyramid
        );
      break;
    case MLN_OFFLINE_REGION_DEFINITION_GEOMETRY:
      self->geometry =
        maplibre_native_ffi_offline_geometry_region_definition_from_native(
          &data->geometry
        );
      break;
    default:
      break;
  }
  return self;
}

gpointer maplibre_native_ffi_offline_region_definition_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_offline_region_definition_copy(
    (const MaplibreNativeFfiOfflineRegionDefinition*)self
  );
}

struct _MaplibreNativeFfiOfflineRegionInfo {
  gint64 id;
  MaplibreNativeFfiOfflineRegionDefinition* definition;
  GBytes* metadata;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOfflineRegionInfo, maplibre_native_ffi_offline_region_info,
  maplibre_native_ffi_offline_region_info_copy,
  maplibre_native_ffi_offline_region_info_free
)

MaplibreNativeFfiOfflineRegionInfo* maplibre_native_ffi_offline_region_info_new(
  void
) {
  MaplibreNativeFfiOfflineRegionInfo* self =
    g_new0(MaplibreNativeFfiOfflineRegionInfo, 1);
  return self;
}

MaplibreNativeFfiOfflineRegionInfo*
maplibre_native_ffi_offline_region_info_copy(
  const MaplibreNativeFfiOfflineRegionInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiOfflineRegionInfo* copy = g_memdup2(self, sizeof *self);
  copy->definition =
    self->definition != NULL
      ? maplibre_native_ffi_offline_region_definition_copy(self->definition)
      : NULL;
  copy->metadata = self->metadata != NULL ? g_bytes_ref(self->metadata) : NULL;
  return copy;
}

void maplibre_native_ffi_offline_region_info_free(
  MaplibreNativeFfiOfflineRegionInfo* self
) {
  if (self == NULL) return;
  g_clear_pointer(
    &self->definition, maplibre_native_ffi_offline_region_definition_free
  );
  g_clear_pointer(&self->metadata, g_bytes_unref);
  g_free(self);
}

gint64 maplibre_native_ffi_offline_region_info_get_id(
  const MaplibreNativeFfiOfflineRegionInfo* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->id;
}

MaplibreNativeFfiOfflineRegionDefinition*
maplibre_native_ffi_offline_region_info_get_definition(
  const MaplibreNativeFfiOfflineRegionInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->definition;
}

GBytes* maplibre_native_ffi_offline_region_info_get_metadata(
  const MaplibreNativeFfiOfflineRegionInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->metadata;
}

gpointer maplibre_native_ffi_offline_region_info_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_offline_region_info_copy(
    (const MaplibreNativeFfiOfflineRegionInfo*)self
  );
}

void maplibre_native_ffi_offline_region_info_to_native(
  const MaplibreNativeFfiOfflineRegionInfo* self, mln_offline_region_info* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->id = (mln_offline_region_id)(self->id);
  if (self->definition != NULL)
    maplibre_native_ffi_offline_region_definition_to_native(
      self->definition, &out->definition, call
    );
  {
    mln_buffer_view metadata_view =
      maplibre_native_ffi_bytes_view(self->metadata);
    out->metadata = metadata_view.data;
    out->metadata_size = metadata_view.size;
  }
}

MaplibreNativeFfiOfflineRegionInfo*
maplibre_native_ffi_offline_region_info_from_native(
  const mln_offline_region_info* in
) {
  MaplibreNativeFfiOfflineRegionInfo* self =
    maplibre_native_ffi_offline_region_info_new();
  self->id = (gint64)(in->id);
  g_clear_pointer(
    &self->definition, maplibre_native_ffi_offline_region_definition_free
  );
  self->definition =
    maplibre_native_ffi_offline_region_definition_from_native(&in->definition);
  g_clear_pointer(&self->metadata, g_bytes_unref);
  self->metadata =
    maplibre_native_ffi_bytes_new(in->metadata, in->metadata_size);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOfflineRegionStatus,
  maplibre_native_ffi_offline_region_status,
  maplibre_native_ffi_offline_region_status_copy,
  maplibre_native_ffi_offline_region_status_free
)

MaplibreNativeFfiOfflineRegionStatus*
maplibre_native_ffi_offline_region_status_copy(
  const MaplibreNativeFfiOfflineRegionStatus* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_offline_region_status_free(
  MaplibreNativeFfiOfflineRegionStatus* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_offline_region_status_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_offline_region_status_copy(
    (const MaplibreNativeFfiOfflineRegionStatus*)self
  );
}

void maplibre_native_ffi_offline_region_status_to_native(
  const MaplibreNativeFfiOfflineRegionStatus* self,
  mln_offline_region_status* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->download_state =
    (mln_offline_region_download_state)(self->download_state);
  out->completed_resource_count = (uint64_t)(self->completed_resource_count);
  out->completed_resource_size = (uint64_t)(self->completed_resource_size);
  out->completed_tile_count = (uint64_t)(self->completed_tile_count);
  out->required_tile_count = (uint64_t)(self->required_tile_count);
  out->completed_tile_size = (uint64_t)(self->completed_tile_size);
  out->required_resource_count = (uint64_t)(self->required_resource_count);
  out->required_resource_count_is_precise =
    (self->required_resource_count_is_precise) != FALSE;
  out->complete = (self->complete) != FALSE;
}

void maplibre_native_ffi_offline_region_status_read(
  MaplibreNativeFfiOfflineRegionStatus* out, const mln_offline_region_status* in
) {
  out->download_state =
    (MaplibreNativeFfiOfflineRegionDownloadState)(in->download_state);
  out->completed_resource_count = (guint64)(in->completed_resource_count);
  out->completed_resource_size = (guint64)(in->completed_resource_size);
  out->completed_tile_count = (guint64)(in->completed_tile_count);
  out->required_tile_count = (guint64)(in->required_tile_count);
  out->completed_tile_size = (guint64)(in->completed_tile_size);
  out->required_resource_count = (guint64)(in->required_resource_count);
  out->required_resource_count_is_precise =
    (in->required_resource_count_is_precise) ? TRUE : FALSE;
  out->complete = (in->complete) ? TRUE : FALSE;
}

MaplibreNativeFfiOfflineRegionStatus*
maplibre_native_ffi_offline_region_status_from_native(
  const mln_offline_region_status* in
) {
  MaplibreNativeFfiOfflineRegionStatus* self =
    g_new0(MaplibreNativeFfiOfflineRegionStatus, 1);
  maplibre_native_ffi_offline_region_status_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOffscreenDescriptor,
  maplibre_native_ffi_offscreen_descriptor,
  maplibre_native_ffi_offscreen_descriptor_copy,
  maplibre_native_ffi_offscreen_descriptor_free
)

MaplibreNativeFfiOffscreenDescriptor*
maplibre_native_ffi_offscreen_descriptor_copy(
  const MaplibreNativeFfiOffscreenDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_offscreen_descriptor_free(
  MaplibreNativeFfiOffscreenDescriptor* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_offscreen_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_offscreen_descriptor_copy(
    (const MaplibreNativeFfiOffscreenDescriptor*)self
  );
}

void maplibre_native_ffi_offscreen_descriptor_to_native(
  const MaplibreNativeFfiOffscreenDescriptor* self,
  mln_offscreen_descriptor* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
}

void maplibre_native_ffi_offscreen_descriptor_read(
  MaplibreNativeFfiOffscreenDescriptor* out, const mln_offscreen_descriptor* in
) {
  maplibre_native_ffi_logical_size_read(&out->logical_size, &in->logical_size);
}

MaplibreNativeFfiOffscreenDescriptor*
maplibre_native_ffi_offscreen_descriptor_from_native(
  const mln_offscreen_descriptor* in
) {
  MaplibreNativeFfiOffscreenDescriptor* self =
    g_new0(MaplibreNativeFfiOffscreenDescriptor, 1);
  maplibre_native_ffi_offscreen_descriptor_read(self, in);
  return self;
}

struct _MaplibreNativeFfiOffscreenDeviceOptions {
  guint64 fields;
  guint32 adapter_index;
  gboolean single_context;
  MaplibreNativeFfiOpenGlClientApi client_api;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOffscreenDeviceOptions,
  maplibre_native_ffi_offscreen_device_options,
  maplibre_native_ffi_offscreen_device_options_copy,
  maplibre_native_ffi_offscreen_device_options_free
)

MaplibreNativeFfiOffscreenDeviceOptions*
maplibre_native_ffi_offscreen_device_options_new(void) {
  MaplibreNativeFfiOffscreenDeviceOptions* self =
    g_new0(MaplibreNativeFfiOffscreenDeviceOptions, 1);
  return self;
}

MaplibreNativeFfiOffscreenDeviceOptions*
maplibre_native_ffi_offscreen_device_options_copy(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiOffscreenDeviceOptions* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_offscreen_device_options_free(
  MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  if (self == NULL) return;
  g_free(self);
}

guint32 maplibre_native_ffi_offscreen_device_options_get_adapter_index(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->adapter_index;
}

gboolean maplibre_native_ffi_offscreen_device_options_has_adapter_index(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX) != 0;
}

void maplibre_native_ffi_offscreen_device_options_set_adapter_index(
  MaplibreNativeFfiOffscreenDeviceOptions* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->adapter_index = value;
  self->fields |= MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX;
}

gboolean maplibre_native_ffi_offscreen_device_options_get_single_context(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->single_context;
}

gboolean maplibre_native_ffi_offscreen_device_options_has_single_context(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT) != 0;
}

void maplibre_native_ffi_offscreen_device_options_set_single_context(
  MaplibreNativeFfiOffscreenDeviceOptions* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->single_context = value;
  self->fields |= MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT;
}

MaplibreNativeFfiOpenGlClientApi
maplibre_native_ffi_offscreen_device_options_get_client_api(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiOpenGlClientApi)0);
  return self->client_api;
}

gboolean maplibre_native_ffi_offscreen_device_options_has_client_api(
  const MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API) != 0;
}

void maplibre_native_ffi_offscreen_device_options_set_client_api(
  MaplibreNativeFfiOffscreenDeviceOptions* self,
  MaplibreNativeFfiOpenGlClientApi value
) {
  g_return_if_fail(self != NULL);
  self->client_api = value;
  self->fields |= MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API;
}

gpointer maplibre_native_ffi_offscreen_device_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_offscreen_device_options_copy(
    (const MaplibreNativeFfiOffscreenDeviceOptions*)self
  );
}

void maplibre_native_ffi_offscreen_device_options_to_native(
  const MaplibreNativeFfiOffscreenDeviceOptions* self,
  mln_offscreen_device_options* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_offscreen_device_option_fields)self->fields;
  out->adapter_index = (uint32_t)(self->adapter_index);
  out->single_context = (self->single_context) != FALSE;
  out->client_api = (mln_open_gl_client_api)(self->client_api);
}

MaplibreNativeFfiOffscreenDeviceOptions*
maplibre_native_ffi_offscreen_device_options_from_native(
  const mln_offscreen_device_options* in
) {
  MaplibreNativeFfiOffscreenDeviceOptions* self =
    maplibre_native_ffi_offscreen_device_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX) != 0) {
    self->adapter_index = (guint32)(in->adapter_index);
  }
  if ((in->fields & MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT) != 0) {
    self->single_context = (in->single_context) ? TRUE : FALSE;
  }
  if ((in->fields & MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API) != 0) {
    self->client_api = (MaplibreNativeFfiOpenGlClientApi)(in->client_api);
  }
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOpenGlBorrowedTexture,
  maplibre_native_ffi_open_gl_borrowed_texture,
  maplibre_native_ffi_open_gl_borrowed_texture_copy,
  maplibre_native_ffi_open_gl_borrowed_texture_free
)

MaplibreNativeFfiOpenGlBorrowedTexture*
maplibre_native_ffi_open_gl_borrowed_texture_copy(
  const MaplibreNativeFfiOpenGlBorrowedTexture* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_open_gl_borrowed_texture_free(
  MaplibreNativeFfiOpenGlBorrowedTexture* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_open_gl_borrowed_texture_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_open_gl_borrowed_texture_copy(
    (const MaplibreNativeFfiOpenGlBorrowedTexture*)self
  );
}

void maplibre_native_ffi_open_gl_borrowed_texture_to_native(
  const MaplibreNativeFfiOpenGlBorrowedTexture* self,
  mln_open_gl_borrowed_texture* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->texture = (uint32_t)(self->texture);
}

void maplibre_native_ffi_open_gl_borrowed_texture_read(
  MaplibreNativeFfiOpenGlBorrowedTexture* out,
  const mln_open_gl_borrowed_texture* in
) {
  out->texture = (guint32)(in->texture);
}

MaplibreNativeFfiOpenGlBorrowedTexture*
maplibre_native_ffi_open_gl_borrowed_texture_from_native(
  const mln_open_gl_borrowed_texture* in
) {
  MaplibreNativeFfiOpenGlBorrowedTexture* self =
    g_new0(MaplibreNativeFfiOpenGlBorrowedTexture, 1);
  maplibre_native_ffi_open_gl_borrowed_texture_read(self, in);
  return self;
}

struct _MaplibreNativeFfiWglContextDescriptor {
  gpointer device_context;
  gpointer share_context;
  gpointer get_proc_address;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiWglContextDescriptor,
  maplibre_native_ffi_wgl_context_descriptor,
  maplibre_native_ffi_wgl_context_descriptor_copy,
  maplibre_native_ffi_wgl_context_descriptor_free
)

MaplibreNativeFfiWglContextDescriptor*
maplibre_native_ffi_wgl_context_descriptor_new(void) {
  MaplibreNativeFfiWglContextDescriptor* self =
    g_new0(MaplibreNativeFfiWglContextDescriptor, 1);
  return self;
}

MaplibreNativeFfiWglContextDescriptor*
maplibre_native_ffi_wgl_context_descriptor_copy(
  const MaplibreNativeFfiWglContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiWglContextDescriptor* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_wgl_context_descriptor_free(
  MaplibreNativeFfiWglContextDescriptor* self
) {
  if (self == NULL) return;
  g_free(self);
}

gpointer maplibre_native_ffi_wgl_context_descriptor_get_device_context(
  const MaplibreNativeFfiWglContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->device_context;
}

void maplibre_native_ffi_wgl_context_descriptor_set_device_context(
  MaplibreNativeFfiWglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->device_context = value;
}

gpointer maplibre_native_ffi_wgl_context_descriptor_get_share_context(
  const MaplibreNativeFfiWglContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->share_context;
}

void maplibre_native_ffi_wgl_context_descriptor_set_share_context(
  MaplibreNativeFfiWglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->share_context = value;
}

gpointer maplibre_native_ffi_wgl_context_descriptor_get_get_proc_address(
  const MaplibreNativeFfiWglContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->get_proc_address;
}

void maplibre_native_ffi_wgl_context_descriptor_set_get_proc_address(
  MaplibreNativeFfiWglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->get_proc_address = value;
}

gpointer maplibre_native_ffi_wgl_context_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_wgl_context_descriptor_copy(
    (const MaplibreNativeFfiWglContextDescriptor*)self
  );
}

void maplibre_native_ffi_wgl_context_descriptor_to_native(
  const MaplibreNativeFfiWglContextDescriptor* self,
  mln_wgl_context_descriptor* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->device_context = (void*)(self->device_context);
  out->share_context = (void*)(self->share_context);
  out->get_proc_address = (void*)(self->get_proc_address);
}

MaplibreNativeFfiWglContextDescriptor*
maplibre_native_ffi_wgl_context_descriptor_from_native(
  const mln_wgl_context_descriptor* in
) {
  MaplibreNativeFfiWglContextDescriptor* self =
    maplibre_native_ffi_wgl_context_descriptor_new();
  self->device_context = (gpointer)(in->device_context);
  self->share_context = (gpointer)(in->share_context);
  self->get_proc_address = (gpointer)(in->get_proc_address);
  return self;
}

struct _MaplibreNativeFfiWebGlContextDescriptor {
  MaplibreNativeFfiWebGlContextKind kind;
  gint32 context;
  char* canvas_selector;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiWebGlContextDescriptor,
  maplibre_native_ffi_web_gl_context_descriptor,
  maplibre_native_ffi_web_gl_context_descriptor_copy,
  maplibre_native_ffi_web_gl_context_descriptor_free
)

MaplibreNativeFfiWebGlContextDescriptor*
maplibre_native_ffi_web_gl_context_descriptor_new(void) {
  MaplibreNativeFfiWebGlContextDescriptor* self =
    g_new0(MaplibreNativeFfiWebGlContextDescriptor, 1);
  self->canvas_selector = g_strdup("");
  return self;
}

MaplibreNativeFfiWebGlContextDescriptor*
maplibre_native_ffi_web_gl_context_descriptor_copy(
  const MaplibreNativeFfiWebGlContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiWebGlContextDescriptor* copy = g_memdup2(self, sizeof *self);
  copy->canvas_selector = g_strdup(self->canvas_selector);
  return copy;
}

void maplibre_native_ffi_web_gl_context_descriptor_free(
  MaplibreNativeFfiWebGlContextDescriptor* self
) {
  if (self == NULL) return;
  g_free(self->canvas_selector);
  g_free(self);
}

MaplibreNativeFfiWebGlContextKind
maplibre_native_ffi_web_gl_context_descriptor_get_kind(
  const MaplibreNativeFfiWebGlContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiWebGlContextKind)0);
  return self->kind;
}

void maplibre_native_ffi_web_gl_context_descriptor_set_kind(
  MaplibreNativeFfiWebGlContextDescriptor* self,
  MaplibreNativeFfiWebGlContextKind value
) {
  g_return_if_fail(self != NULL);
  self->kind = value;
}

gint32 maplibre_native_ffi_web_gl_context_descriptor_get_context(
  const MaplibreNativeFfiWebGlContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->context;
}

void maplibre_native_ffi_web_gl_context_descriptor_set_context(
  MaplibreNativeFfiWebGlContextDescriptor* self, gint32 value
) {
  g_return_if_fail(self != NULL);
  self->context = value;
}

const char* maplibre_native_ffi_web_gl_context_descriptor_get_canvas_selector(
  const MaplibreNativeFfiWebGlContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->canvas_selector;
}

void maplibre_native_ffi_web_gl_context_descriptor_set_canvas_selector(
  MaplibreNativeFfiWebGlContextDescriptor* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->canvas_selector;
  self->canvas_selector = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

gpointer maplibre_native_ffi_web_gl_context_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_web_gl_context_descriptor_copy(
    (const MaplibreNativeFfiWebGlContextDescriptor*)self
  );
}

void maplibre_native_ffi_web_gl_context_descriptor_to_native(
  const MaplibreNativeFfiWebGlContextDescriptor* self,
  mln_web_gl_context_descriptor* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->kind = (mln_web_gl_context_kind)(self->kind);
  out->context = (int32_t)(self->context);
  out->canvas_selector = maplibre_native_ffi_text_view(self->canvas_selector);
}

MaplibreNativeFfiWebGlContextDescriptor*
maplibre_native_ffi_web_gl_context_descriptor_from_native(
  const mln_web_gl_context_descriptor* in
) {
  MaplibreNativeFfiWebGlContextDescriptor* self =
    maplibre_native_ffi_web_gl_context_descriptor_new();
  self->kind = (MaplibreNativeFfiWebGlContextKind)(in->kind);
  self->context = (gint32)(in->context);
  g_free(self->canvas_selector);
  self->canvas_selector = maplibre_native_ffi_text_new(in->canvas_selector);
  return self;
}

struct _MaplibreNativeFfiOpenGlContextDescriptorData {
  gint64 tag;
  MaplibreNativeFfiWglContextDescriptor* wgl;
  MaplibreNativeFfiEglContextDescriptor* egl;
  MaplibreNativeFfiWebGlContextDescriptor* web_gl;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOpenGlContextDescriptorData,
  maplibre_native_ffi_open_gl_context_descriptor_data,
  maplibre_native_ffi_open_gl_context_descriptor_data_copy,
  maplibre_native_ffi_open_gl_context_descriptor_data_free
)

MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_data_copy(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiOpenGlContextDescriptorData* copy =
    g_memdup2(self, sizeof *self);
  copy->wgl = self->wgl != NULL
                ? maplibre_native_ffi_wgl_context_descriptor_copy(self->wgl)
                : NULL;
  copy->egl = self->egl != NULL
                ? maplibre_native_ffi_egl_context_descriptor_copy(self->egl)
                : NULL;
  copy->web_gl =
    self->web_gl != NULL
      ? maplibre_native_ffi_web_gl_context_descriptor_copy(self->web_gl)
      : NULL;
  return copy;
}

void maplibre_native_ffi_open_gl_context_descriptor_data_free(
  MaplibreNativeFfiOpenGlContextDescriptorData* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->wgl, maplibre_native_ffi_wgl_context_descriptor_free);
  g_clear_pointer(&self->egl, maplibre_native_ffi_egl_context_descriptor_free);
  g_clear_pointer(
    &self->web_gl, maplibre_native_ffi_web_gl_context_descriptor_free
  );
  g_free(self);
}

MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_data_new_wgl(
  const MaplibreNativeFfiWglContextDescriptor* value
) {
  g_return_val_if_fail(value != NULL, NULL);
  MaplibreNativeFfiOpenGlContextDescriptorData* self =
    g_new0(MaplibreNativeFfiOpenGlContextDescriptorData, 1);
  self->tag = MLN_OPEN_GL_CONTEXT_PLATFORM_WGL;
  self->wgl = maplibre_native_ffi_wgl_context_descriptor_new();
  MaplibreNativeFfiWglContextDescriptor* previous = self->wgl;
  self->wgl = maplibre_native_ffi_wgl_context_descriptor_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_wgl_context_descriptor_free(previous);
  return self;
}

gboolean maplibre_native_ffi_open_gl_context_descriptor_data_is_wgl(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_OPEN_GL_CONTEXT_PLATFORM_WGL;
}

MaplibreNativeFfiWglContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_data_get_wgl(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(self->tag == MLN_OPEN_GL_CONTEXT_PLATFORM_WGL, NULL);
  return self->wgl;
}

MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_data_new_egl(
  const MaplibreNativeFfiEglContextDescriptor* value
) {
  g_return_val_if_fail(value != NULL, NULL);
  MaplibreNativeFfiOpenGlContextDescriptorData* self =
    g_new0(MaplibreNativeFfiOpenGlContextDescriptorData, 1);
  self->tag = MLN_OPEN_GL_CONTEXT_PLATFORM_EGL;
  self->egl = maplibre_native_ffi_egl_context_descriptor_new();
  MaplibreNativeFfiEglContextDescriptor* previous = self->egl;
  self->egl = maplibre_native_ffi_egl_context_descriptor_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_egl_context_descriptor_free(previous);
  return self;
}

gboolean maplibre_native_ffi_open_gl_context_descriptor_data_is_egl(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_OPEN_GL_CONTEXT_PLATFORM_EGL;
}

MaplibreNativeFfiEglContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_data_get_egl(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(self->tag == MLN_OPEN_GL_CONTEXT_PLATFORM_EGL, NULL);
  return self->egl;
}

MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_data_new_web_gl(
  const MaplibreNativeFfiWebGlContextDescriptor* value
) {
  g_return_val_if_fail(value != NULL, NULL);
  MaplibreNativeFfiOpenGlContextDescriptorData* self =
    g_new0(MaplibreNativeFfiOpenGlContextDescriptorData, 1);
  self->tag = MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL;
  self->web_gl = maplibre_native_ffi_web_gl_context_descriptor_new();
  MaplibreNativeFfiWebGlContextDescriptor* previous = self->web_gl;
  self->web_gl = maplibre_native_ffi_web_gl_context_descriptor_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_web_gl_context_descriptor_free(previous);
  return self;
}

gboolean maplibre_native_ffi_open_gl_context_descriptor_data_is_web_gl(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL;
}

MaplibreNativeFfiWebGlContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_data_get_web_gl(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(self->tag == MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL, NULL);
  return self->web_gl;
}

void maplibre_native_ffi_open_gl_context_descriptor_data_to_native(
  const MaplibreNativeFfiOpenGlContextDescriptorData* self,
  mln_open_gl_context_platform* out_tag,
  mln_open_gl_context_descriptor_data* out, MaplibreNativeFfiCall* call
) {
  mln_open_gl_context_descriptor_data* data = out;
  memset(data, 0, sizeof *data);
  *out_tag = (mln_open_gl_context_platform)self->tag;
  switch (self->tag) {
    case MLN_OPEN_GL_CONTEXT_PLATFORM_WGL:
      if (self->wgl != NULL)
        maplibre_native_ffi_wgl_context_descriptor_to_native(
          self->wgl, &data->wgl, call
        );
      break;
    case MLN_OPEN_GL_CONTEXT_PLATFORM_EGL:
      if (self->egl != NULL)
        maplibre_native_ffi_egl_context_descriptor_to_native(
          self->egl, &data->egl, call
        );
      break;
    case MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL:
      if (self->web_gl != NULL)
        maplibre_native_ffi_web_gl_context_descriptor_to_native(
          self->web_gl, &data->web_gl, call
        );
      break;
    default:
      break;
  }
}

MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_data_from_native(
  gint64 tag, const mln_open_gl_context_descriptor_data* in
) {
  const mln_open_gl_context_descriptor_data* data = in;
  MaplibreNativeFfiOpenGlContextDescriptorData* self =
    g_new0(MaplibreNativeFfiOpenGlContextDescriptorData, 1);
  self->tag = tag;
  switch (tag) {
    case MLN_OPEN_GL_CONTEXT_PLATFORM_WGL:
      self->wgl =
        maplibre_native_ffi_wgl_context_descriptor_from_native(&data->wgl);
      break;
    case MLN_OPEN_GL_CONTEXT_PLATFORM_EGL:
      self->egl =
        maplibre_native_ffi_egl_context_descriptor_from_native(&data->egl);
      break;
    case MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL:
      self->web_gl = maplibre_native_ffi_web_gl_context_descriptor_from_native(
        &data->web_gl
      );
      break;
    default:
      break;
  }
  return self;
}

gpointer maplibre_native_ffi_open_gl_context_descriptor_data_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_open_gl_context_descriptor_data_copy(
    (const MaplibreNativeFfiOpenGlContextDescriptorData*)self
  );
}

struct _MaplibreNativeFfiOpenGlContextDescriptor {
  MaplibreNativeFfiOpenGlContextOwnership ownership;
  MaplibreNativeFfiOpenGlContextDescriptorData* data;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOpenGlContextDescriptor,
  maplibre_native_ffi_open_gl_context_descriptor,
  maplibre_native_ffi_open_gl_context_descriptor_copy,
  maplibre_native_ffi_open_gl_context_descriptor_free
)

MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_new(void) {
  MaplibreNativeFfiOpenGlContextDescriptor* self =
    g_new0(MaplibreNativeFfiOpenGlContextDescriptor, 1);
  return self;
}

MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_copy(
  const MaplibreNativeFfiOpenGlContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiOpenGlContextDescriptor* copy =
    g_memdup2(self, sizeof *self);
  copy->data =
    self->data != NULL
      ? maplibre_native_ffi_open_gl_context_descriptor_data_copy(self->data)
      : NULL;
  return copy;
}

void maplibre_native_ffi_open_gl_context_descriptor_free(
  MaplibreNativeFfiOpenGlContextDescriptor* self
) {
  if (self == NULL) return;
  g_clear_pointer(
    &self->data, maplibre_native_ffi_open_gl_context_descriptor_data_free
  );
  g_free(self);
}

MaplibreNativeFfiOpenGlContextOwnership
maplibre_native_ffi_open_gl_context_descriptor_get_ownership(
  const MaplibreNativeFfiOpenGlContextDescriptor* self
) {
  g_return_val_if_fail(
    self != NULL, (MaplibreNativeFfiOpenGlContextOwnership)0
  );
  return self->ownership;
}

void maplibre_native_ffi_open_gl_context_descriptor_set_ownership(
  MaplibreNativeFfiOpenGlContextDescriptor* self,
  MaplibreNativeFfiOpenGlContextOwnership value
) {
  g_return_if_fail(self != NULL);
  self->ownership = value;
}

MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_get_data(
  const MaplibreNativeFfiOpenGlContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->data;
}

void maplibre_native_ffi_open_gl_context_descriptor_set_data(
  MaplibreNativeFfiOpenGlContextDescriptor* self,
  const MaplibreNativeFfiOpenGlContextDescriptorData* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiOpenGlContextDescriptorData* previous = self->data;
  self->data = maplibre_native_ffi_open_gl_context_descriptor_data_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_open_gl_context_descriptor_data_free(previous);
}

gpointer maplibre_native_ffi_open_gl_context_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_open_gl_context_descriptor_copy(
    (const MaplibreNativeFfiOpenGlContextDescriptor*)self
  );
}

void maplibre_native_ffi_open_gl_context_descriptor_to_native(
  const MaplibreNativeFfiOpenGlContextDescriptor* self,
  mln_open_gl_context_descriptor* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->ownership = (mln_open_gl_context_ownership)(self->ownership);
  if (self->data != NULL)
    maplibre_native_ffi_open_gl_context_descriptor_data_to_native(
      self->data, &out->platform, &out->data, call
    );
}

MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_from_native(
  const mln_open_gl_context_descriptor* in
) {
  MaplibreNativeFfiOpenGlContextDescriptor* self =
    maplibre_native_ffi_open_gl_context_descriptor_new();
  self->ownership = (MaplibreNativeFfiOpenGlContextOwnership)(in->ownership);
  g_clear_pointer(
    &self->data, maplibre_native_ffi_open_gl_context_descriptor_data_free
  );
  self->data = maplibre_native_ffi_open_gl_context_descriptor_data_from_native(
    (gint64)in->platform, &in->data
  );
  return self;
}

struct _MaplibreNativeFfiOpenGlBorrowedTextureDescriptor {
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 physical_width;
  guint32 physical_height;
  MaplibreNativeFfiOpenGlContextDescriptor* context;
  GPtrArray* textures;
  guint32 target;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor,
  maplibre_native_ffi_open_gl_borrowed_texture_descriptor,
  maplibre_native_ffi_open_gl_borrowed_texture_descriptor_copy,
  maplibre_native_ffi_open_gl_borrowed_texture_descriptor_free
)

MaplibreNativeFfiOpenGlBorrowedTextureDescriptor*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_new(void) {
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self =
    g_new0(MaplibreNativeFfiOpenGlBorrowedTextureDescriptor, 1);
  self->context = maplibre_native_ffi_open_gl_context_descriptor_new();
  self->textures = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_open_gl_borrowed_texture_free
  );
  return self;
}

MaplibreNativeFfiOpenGlBorrowedTextureDescriptor*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_copy(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* copy =
    g_memdup2(self, sizeof *self);
  copy->context =
    self->context != NULL
      ? maplibre_native_ffi_open_gl_context_descriptor_copy(self->context)
      : NULL;
  copy->textures =
    self->textures != NULL
      ? g_ptr_array_copy(
          self->textures,
          maplibre_native_ffi_open_gl_borrowed_texture_copy_func, NULL
        )
      : NULL;
  if (copy->textures != NULL)
    g_ptr_array_set_free_func(
      copy->textures,
      (GDestroyNotify)maplibre_native_ffi_open_gl_borrowed_texture_free
    );
  return copy;
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_free(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  if (self == NULL) return;
  g_clear_pointer(
    &self->context, maplibre_native_ffi_open_gl_context_descriptor_free
  );
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_logical_size(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_logical_size(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

guint32
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_physical_width(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->physical_width;
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_width(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->physical_width = value;
}

guint32
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_physical_height(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->physical_height;
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_height(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->physical_height = value;
}

MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_context(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->context;
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_context(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiOpenGlContextDescriptor* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiOpenGlContextDescriptor* previous = self->context;
  self->context = maplibre_native_ffi_open_gl_context_descriptor_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_open_gl_context_descriptor_free(previous);
}

GPtrArray* maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_textures(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->textures;
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_textures(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiOpenGlBorrowedTexture* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->textures;
  self->textures = g_ptr_array_new_full(
    (guint)value_count,
    (GDestroyNotify)maplibre_native_ffi_open_gl_borrowed_texture_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->textures,
      maplibre_native_ffi_open_gl_borrowed_texture_copy(&value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

guint32 maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_target(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->target;
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_target(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->target = value;
}

gpointer maplibre_native_ffi_open_gl_borrowed_texture_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_open_gl_borrowed_texture_descriptor_copy(
    (const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor*)self
  );
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_to_native(
  const MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self,
  mln_open_gl_borrowed_texture_descriptor* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  out->physical_width = (uint32_t)(self->physical_width);
  out->physical_height = (uint32_t)(self->physical_height);
  if (self->context != NULL)
    maplibre_native_ffi_open_gl_context_descriptor_to_native(
      self->context, &out->context, call
    );
  if (self->textures != NULL && self->textures->len > 0) {
    mln_open_gl_borrowed_texture* items =
      maplibre_native_ffi_call_alloc(call, self->textures->len, sizeof *items);
    for (guint index = 0; index < self->textures->len; index++) {
      maplibre_native_ffi_open_gl_borrowed_texture_to_native(
        self->textures->pdata[index], &items[index], call
      );
    }
    out->textures = items;
    out->texture_count = self->textures->len;
  }
  out->target = (uint32_t)(self->target);
}

MaplibreNativeFfiOpenGlBorrowedTextureDescriptor*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_from_native(
  const mln_open_gl_borrowed_texture_descriptor* in
) {
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self =
    maplibre_native_ffi_open_gl_borrowed_texture_descriptor_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  self->physical_width = (guint32)(in->physical_width);
  self->physical_height = (guint32)(in->physical_height);
  g_clear_pointer(
    &self->context, maplibre_native_ffi_open_gl_context_descriptor_free
  );
  self->context =
    maplibre_native_ffi_open_gl_context_descriptor_from_native(&in->context);
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  self->textures = g_ptr_array_new_full(
    (guint)in->texture_count,
    (GDestroyNotify)maplibre_native_ffi_open_gl_borrowed_texture_free
  );
  for (size_t index = 0; index < in->texture_count; index++) {
    g_ptr_array_add(
      self->textures, maplibre_native_ffi_open_gl_borrowed_texture_from_native(
                        &in->textures[index]
                      )
    );
  }
  self->target = (guint32)(in->target);
  return self;
}

struct _MaplibreNativeFfiOpenGlBorrowedTextureReplacement {
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 physical_width;
  guint32 physical_height;
  GPtrArray* textures;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement,
  maplibre_native_ffi_open_gl_borrowed_texture_replacement,
  maplibre_native_ffi_open_gl_borrowed_texture_replacement_copy,
  maplibre_native_ffi_open_gl_borrowed_texture_replacement_free
)

MaplibreNativeFfiOpenGlBorrowedTextureReplacement*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_new(void) {
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self =
    g_new0(MaplibreNativeFfiOpenGlBorrowedTextureReplacement, 1);
  self->textures = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_open_gl_borrowed_texture_free
  );
  return self;
}

MaplibreNativeFfiOpenGlBorrowedTextureReplacement*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_copy(
  const MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* copy =
    g_memdup2(self, sizeof *self);
  copy->textures =
    self->textures != NULL
      ? g_ptr_array_copy(
          self->textures,
          maplibre_native_ffi_open_gl_borrowed_texture_copy_func, NULL
        )
      : NULL;
  if (copy->textures != NULL)
    g_ptr_array_set_free_func(
      copy->textures,
      (GDestroyNotify)maplibre_native_ffi_open_gl_borrowed_texture_free
    );
  return copy;
}

void maplibre_native_ffi_open_gl_borrowed_texture_replacement_free(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_logical_size(
  const MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_logical_size(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

guint32
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_physical_width(
  const MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->physical_width;
}

void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_width(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->physical_width = value;
}

guint32
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_physical_height(
  const MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->physical_height;
}

void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_height(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->physical_height = value;
}

GPtrArray*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_textures(
  const MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->textures;
}

void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_textures(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self,
  const MaplibreNativeFfiOpenGlBorrowedTexture* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->textures;
  self->textures = g_ptr_array_new_full(
    (guint)value_count,
    (GDestroyNotify)maplibre_native_ffi_open_gl_borrowed_texture_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->textures,
      maplibre_native_ffi_open_gl_borrowed_texture_copy(&value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

gpointer maplibre_native_ffi_open_gl_borrowed_texture_replacement_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_open_gl_borrowed_texture_replacement_copy(
    (const MaplibreNativeFfiOpenGlBorrowedTextureReplacement*)self
  );
}

void maplibre_native_ffi_open_gl_borrowed_texture_replacement_to_native(
  const MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self,
  mln_open_gl_borrowed_texture_replacement* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  out->physical_width = (uint32_t)(self->physical_width);
  out->physical_height = (uint32_t)(self->physical_height);
  if (self->textures != NULL && self->textures->len > 0) {
    mln_open_gl_borrowed_texture* items =
      maplibre_native_ffi_call_alloc(call, self->textures->len, sizeof *items);
    for (guint index = 0; index < self->textures->len; index++) {
      maplibre_native_ffi_open_gl_borrowed_texture_to_native(
        self->textures->pdata[index], &items[index], call
      );
    }
    out->textures = items;
    out->texture_count = self->textures->len;
  }
}

MaplibreNativeFfiOpenGlBorrowedTextureReplacement*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_from_native(
  const mln_open_gl_borrowed_texture_replacement* in
) {
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self =
    maplibre_native_ffi_open_gl_borrowed_texture_replacement_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  self->physical_width = (guint32)(in->physical_width);
  self->physical_height = (guint32)(in->physical_height);
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  self->textures = g_ptr_array_new_full(
    (guint)in->texture_count,
    (GDestroyNotify)maplibre_native_ffi_open_gl_borrowed_texture_free
  );
  for (size_t index = 0; index < in->texture_count; index++) {
    g_ptr_array_add(
      self->textures, maplibre_native_ffi_open_gl_borrowed_texture_from_native(
                        &in->textures[index]
                      )
    );
  }
  return self;
}

struct _MaplibreNativeFfiOpenGlSurfaceDescriptor {
  MaplibreNativeFfiLogicalSize logical_size;
  MaplibreNativeFfiOpenGlContextDescriptor* context;
  gpointer surface;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOpenGlSurfaceDescriptor,
  maplibre_native_ffi_open_gl_surface_descriptor,
  maplibre_native_ffi_open_gl_surface_descriptor_copy,
  maplibre_native_ffi_open_gl_surface_descriptor_free
)

MaplibreNativeFfiOpenGlSurfaceDescriptor*
maplibre_native_ffi_open_gl_surface_descriptor_new(void) {
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self =
    g_new0(MaplibreNativeFfiOpenGlSurfaceDescriptor, 1);
  self->context = maplibre_native_ffi_open_gl_context_descriptor_new();
  return self;
}

MaplibreNativeFfiOpenGlSurfaceDescriptor*
maplibre_native_ffi_open_gl_surface_descriptor_copy(
  const MaplibreNativeFfiOpenGlSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiOpenGlSurfaceDescriptor* copy =
    g_memdup2(self, sizeof *self);
  copy->context =
    self->context != NULL
      ? maplibre_native_ffi_open_gl_context_descriptor_copy(self->context)
      : NULL;
  return copy;
}

void maplibre_native_ffi_open_gl_surface_descriptor_free(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self
) {
  if (self == NULL) return;
  g_clear_pointer(
    &self->context, maplibre_native_ffi_open_gl_context_descriptor_free
  );
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_surface_descriptor_get_logical_size(
  const MaplibreNativeFfiOpenGlSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_open_gl_surface_descriptor_set_logical_size(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_surface_descriptor_get_context(
  const MaplibreNativeFfiOpenGlSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->context;
}

void maplibre_native_ffi_open_gl_surface_descriptor_set_context(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self,
  const MaplibreNativeFfiOpenGlContextDescriptor* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiOpenGlContextDescriptor* previous = self->context;
  self->context = maplibre_native_ffi_open_gl_context_descriptor_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_open_gl_context_descriptor_free(previous);
}

gpointer maplibre_native_ffi_open_gl_surface_descriptor_get_surface(
  const MaplibreNativeFfiOpenGlSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->surface;
}

void maplibre_native_ffi_open_gl_surface_descriptor_set_surface(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->surface = value;
}

gpointer maplibre_native_ffi_open_gl_surface_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_open_gl_surface_descriptor_copy(
    (const MaplibreNativeFfiOpenGlSurfaceDescriptor*)self
  );
}

void maplibre_native_ffi_open_gl_surface_descriptor_to_native(
  const MaplibreNativeFfiOpenGlSurfaceDescriptor* self,
  mln_open_gl_surface_descriptor* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  if (self->context != NULL)
    maplibre_native_ffi_open_gl_context_descriptor_to_native(
      self->context, &out->context, call
    );
  out->surface = (void*)(self->surface);
}

MaplibreNativeFfiOpenGlSurfaceDescriptor*
maplibre_native_ffi_open_gl_surface_descriptor_from_native(
  const mln_open_gl_surface_descriptor* in
) {
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self =
    maplibre_native_ffi_open_gl_surface_descriptor_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  g_clear_pointer(
    &self->context, maplibre_native_ffi_open_gl_context_descriptor_free
  );
  self->context =
    maplibre_native_ffi_open_gl_context_descriptor_from_native(&in->context);
  self->surface = (gpointer)(in->surface);
  return self;
}

struct _MaplibreNativeFfiOpenGlSurfaceReplacement {
  MaplibreNativeFfiLogicalSize logical_size;
  gpointer surface;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOpenGlSurfaceReplacement,
  maplibre_native_ffi_open_gl_surface_replacement,
  maplibre_native_ffi_open_gl_surface_replacement_copy,
  maplibre_native_ffi_open_gl_surface_replacement_free
)

MaplibreNativeFfiOpenGlSurfaceReplacement*
maplibre_native_ffi_open_gl_surface_replacement_new(void) {
  MaplibreNativeFfiOpenGlSurfaceReplacement* self =
    g_new0(MaplibreNativeFfiOpenGlSurfaceReplacement, 1);
  return self;
}

MaplibreNativeFfiOpenGlSurfaceReplacement*
maplibre_native_ffi_open_gl_surface_replacement_copy(
  const MaplibreNativeFfiOpenGlSurfaceReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiOpenGlSurfaceReplacement* copy =
    g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_open_gl_surface_replacement_free(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self
) {
  if (self == NULL) return;
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_surface_replacement_get_logical_size(
  const MaplibreNativeFfiOpenGlSurfaceReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_open_gl_surface_replacement_set_logical_size(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

gpointer maplibre_native_ffi_open_gl_surface_replacement_get_surface(
  const MaplibreNativeFfiOpenGlSurfaceReplacement* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->surface;
}

void maplibre_native_ffi_open_gl_surface_replacement_set_surface(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->surface = value;
}

gpointer maplibre_native_ffi_open_gl_surface_replacement_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_open_gl_surface_replacement_copy(
    (const MaplibreNativeFfiOpenGlSurfaceReplacement*)self
  );
}

void maplibre_native_ffi_open_gl_surface_replacement_to_native(
  const MaplibreNativeFfiOpenGlSurfaceReplacement* self,
  mln_open_gl_surface_replacement* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  out->surface = (void*)(self->surface);
}

MaplibreNativeFfiOpenGlSurfaceReplacement*
maplibre_native_ffi_open_gl_surface_replacement_from_native(
  const mln_open_gl_surface_replacement* in
) {
  MaplibreNativeFfiOpenGlSurfaceReplacement* self =
    maplibre_native_ffi_open_gl_surface_replacement_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  self->surface = (gpointer)(in->surface);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiOpenGlTextureFrame,
  maplibre_native_ffi_open_gl_texture_frame,
  maplibre_native_ffi_open_gl_texture_frame_copy,
  maplibre_native_ffi_open_gl_texture_frame_free
)

MaplibreNativeFfiOpenGlTextureFrame*
maplibre_native_ffi_open_gl_texture_frame_copy(
  const MaplibreNativeFfiOpenGlTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_open_gl_texture_frame_free(
  MaplibreNativeFfiOpenGlTextureFrame* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_open_gl_texture_frame_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_open_gl_texture_frame_copy(
    (const MaplibreNativeFfiOpenGlTextureFrame*)self
  );
}

void maplibre_native_ffi_open_gl_texture_frame_to_native(
  const MaplibreNativeFfiOpenGlTextureFrame* self,
  mln_open_gl_texture_frame* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->width = (uint32_t)(self->width);
  out->height = (uint32_t)(self->height);
  out->scale_factor = (double)(self->scale_factor);
  out->slot = (uint32_t)(self->slot);
  out->texture = (uint32_t)(self->texture);
  out->target = (uint32_t)(self->target);
}

void maplibre_native_ffi_open_gl_texture_frame_read(
  MaplibreNativeFfiOpenGlTextureFrame* out, const mln_open_gl_texture_frame* in
) {
  out->width = (guint32)(in->width);
  out->height = (guint32)(in->height);
  out->scale_factor = (gdouble)(in->scale_factor);
  out->slot = (guint32)(in->slot);
  out->texture = (guint32)(in->texture);
  out->target = (guint32)(in->target);
}

MaplibreNativeFfiOpenGlTextureFrame*
maplibre_native_ffi_open_gl_texture_frame_from_native(
  const mln_open_gl_texture_frame* in
) {
  MaplibreNativeFfiOpenGlTextureFrame* self =
    g_new0(MaplibreNativeFfiOpenGlTextureFrame, 1);
  maplibre_native_ffi_open_gl_texture_frame_read(self, in);
  return self;
}

struct _MaplibreNativeFfiPremultipliedRgba8Image {
  guint32 width;
  guint32 height;
  guint32 stride;
  GBytes* pixels;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiPremultipliedRgba8Image,
  maplibre_native_ffi_premultiplied_rgba8_image,
  maplibre_native_ffi_premultiplied_rgba8_image_copy,
  maplibre_native_ffi_premultiplied_rgba8_image_free
)

MaplibreNativeFfiPremultipliedRgba8Image*
maplibre_native_ffi_premultiplied_rgba8_image_new(void) {
  MaplibreNativeFfiPremultipliedRgba8Image* self =
    g_new0(MaplibreNativeFfiPremultipliedRgba8Image, 1);
  return self;
}

MaplibreNativeFfiPremultipliedRgba8Image*
maplibre_native_ffi_premultiplied_rgba8_image_copy(
  const MaplibreNativeFfiPremultipliedRgba8Image* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiPremultipliedRgba8Image* copy =
    g_memdup2(self, sizeof *self);
  copy->pixels = self->pixels != NULL ? g_bytes_ref(self->pixels) : NULL;
  return copy;
}

void maplibre_native_ffi_premultiplied_rgba8_image_free(
  MaplibreNativeFfiPremultipliedRgba8Image* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->pixels, g_bytes_unref);
  g_free(self);
}

guint32 maplibre_native_ffi_premultiplied_rgba8_image_get_width(
  const MaplibreNativeFfiPremultipliedRgba8Image* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->width;
}

void maplibre_native_ffi_premultiplied_rgba8_image_set_width(
  MaplibreNativeFfiPremultipliedRgba8Image* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->width = value;
}

guint32 maplibre_native_ffi_premultiplied_rgba8_image_get_height(
  const MaplibreNativeFfiPremultipliedRgba8Image* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->height;
}

void maplibre_native_ffi_premultiplied_rgba8_image_set_height(
  MaplibreNativeFfiPremultipliedRgba8Image* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->height = value;
}

guint32 maplibre_native_ffi_premultiplied_rgba8_image_get_stride(
  const MaplibreNativeFfiPremultipliedRgba8Image* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->stride;
}

void maplibre_native_ffi_premultiplied_rgba8_image_set_stride(
  MaplibreNativeFfiPremultipliedRgba8Image* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->stride = value;
}

GBytes* maplibre_native_ffi_premultiplied_rgba8_image_get_pixels(
  const MaplibreNativeFfiPremultipliedRgba8Image* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->pixels;
}

void maplibre_native_ffi_premultiplied_rgba8_image_set_pixels(
  MaplibreNativeFfiPremultipliedRgba8Image* self, GBytes* value
) {
  g_return_if_fail(self != NULL);
  GBytes* previous = self->pixels;
  self->pixels = value != NULL ? g_bytes_ref(value) : NULL;
  if (previous != NULL) g_bytes_unref(previous);
}

gpointer maplibre_native_ffi_premultiplied_rgba8_image_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_premultiplied_rgba8_image_copy(
    (const MaplibreNativeFfiPremultipliedRgba8Image*)self
  );
}

void maplibre_native_ffi_premultiplied_rgba8_image_to_native(
  const MaplibreNativeFfiPremultipliedRgba8Image* self,
  mln_premultiplied_rgba8_image* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->width = (uint32_t)(self->width);
  out->height = (uint32_t)(self->height);
  out->stride = (uint32_t)(self->stride);
  {
    mln_buffer_view pixels_view = maplibre_native_ffi_bytes_view(self->pixels);
    out->pixels = pixels_view.data;
    out->byte_length = pixels_view.size;
  }
}

MaplibreNativeFfiPremultipliedRgba8Image*
maplibre_native_ffi_premultiplied_rgba8_image_from_native(
  const mln_premultiplied_rgba8_image* in
) {
  MaplibreNativeFfiPremultipliedRgba8Image* self =
    maplibre_native_ffi_premultiplied_rgba8_image_new();
  self->width = (guint32)(in->width);
  self->height = (guint32)(in->height);
  self->stride = (guint32)(in->stride);
  g_clear_pointer(&self->pixels, g_bytes_unref);
  self->pixels = maplibre_native_ffi_bytes_new(in->pixels, in->byte_length);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiProjectedMeters, maplibre_native_ffi_projected_meters,
  maplibre_native_ffi_projected_meters_copy,
  maplibre_native_ffi_projected_meters_free
)

MaplibreNativeFfiProjectedMeters* maplibre_native_ffi_projected_meters_copy(
  const MaplibreNativeFfiProjectedMeters* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_projected_meters_free(
  MaplibreNativeFfiProjectedMeters* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_projected_meters_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_projected_meters_copy(
    (const MaplibreNativeFfiProjectedMeters*)self
  );
}

void maplibre_native_ffi_projected_meters_to_native(
  const MaplibreNativeFfiProjectedMeters* self, mln_projected_meters* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->northing = (double)(self->northing);
  out->easting = (double)(self->easting);
}

void maplibre_native_ffi_projected_meters_read(
  MaplibreNativeFfiProjectedMeters* out, const mln_projected_meters* in
) {
  out->northing = (gdouble)(in->northing);
  out->easting = (gdouble)(in->easting);
}

MaplibreNativeFfiProjectedMeters*
maplibre_native_ffi_projected_meters_from_native(
  const mln_projected_meters* in
) {
  MaplibreNativeFfiProjectedMeters* self =
    g_new0(MaplibreNativeFfiProjectedMeters, 1);
  maplibre_native_ffi_projected_meters_read(self, in);
  return self;
}

struct _MaplibreNativeFfiProjectionModeOptions {
  guint64 fields;
  gboolean axonometric;
  gdouble x_skew;
  gdouble y_skew;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiProjectionModeOptions,
  maplibre_native_ffi_projection_mode_options,
  maplibre_native_ffi_projection_mode_options_copy,
  maplibre_native_ffi_projection_mode_options_free
)

MaplibreNativeFfiProjectionModeOptions*
maplibre_native_ffi_projection_mode_options_new(void) {
  MaplibreNativeFfiProjectionModeOptions* self =
    g_new0(MaplibreNativeFfiProjectionModeOptions, 1);
  return self;
}

MaplibreNativeFfiProjectionModeOptions*
maplibre_native_ffi_projection_mode_options_copy(
  const MaplibreNativeFfiProjectionModeOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiProjectionModeOptions* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_projection_mode_options_free(
  MaplibreNativeFfiProjectionModeOptions* self
) {
  if (self == NULL) return;
  g_free(self);
}

gboolean maplibre_native_ffi_projection_mode_options_get_axonometric(
  const MaplibreNativeFfiProjectionModeOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->axonometric;
}

gboolean maplibre_native_ffi_projection_mode_options_has_axonometric(
  const MaplibreNativeFfiProjectionModeOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_PROJECTION_MODE_OPTION_AXONOMETRIC) != 0;
}

void maplibre_native_ffi_projection_mode_options_set_axonometric(
  MaplibreNativeFfiProjectionModeOptions* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->axonometric = value;
  self->fields |= MLN_PROJECTION_MODE_OPTION_AXONOMETRIC;
}

gdouble maplibre_native_ffi_projection_mode_options_get_x_skew(
  const MaplibreNativeFfiProjectionModeOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->x_skew;
}

gboolean maplibre_native_ffi_projection_mode_options_has_x_skew(
  const MaplibreNativeFfiProjectionModeOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_PROJECTION_MODE_OPTION_X_SKEW) != 0;
}

void maplibre_native_ffi_projection_mode_options_set_x_skew(
  MaplibreNativeFfiProjectionModeOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->x_skew = value;
  self->fields |= MLN_PROJECTION_MODE_OPTION_X_SKEW;
}

gdouble maplibre_native_ffi_projection_mode_options_get_y_skew(
  const MaplibreNativeFfiProjectionModeOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->y_skew;
}

gboolean maplibre_native_ffi_projection_mode_options_has_y_skew(
  const MaplibreNativeFfiProjectionModeOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_PROJECTION_MODE_OPTION_Y_SKEW) != 0;
}

void maplibre_native_ffi_projection_mode_options_set_y_skew(
  MaplibreNativeFfiProjectionModeOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->y_skew = value;
  self->fields |= MLN_PROJECTION_MODE_OPTION_Y_SKEW;
}

gpointer maplibre_native_ffi_projection_mode_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_projection_mode_options_copy(
    (const MaplibreNativeFfiProjectionModeOptions*)self
  );
}

void maplibre_native_ffi_projection_mode_options_to_native(
  const MaplibreNativeFfiProjectionModeOptions* self,
  mln_projection_mode_options* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_projection_mode_option_fields)self->fields;
  out->axonometric = (self->axonometric) != FALSE;
  out->x_skew = (double)(self->x_skew);
  out->y_skew = (double)(self->y_skew);
}

MaplibreNativeFfiProjectionModeOptions*
maplibre_native_ffi_projection_mode_options_from_native(
  const mln_projection_mode_options* in
) {
  MaplibreNativeFfiProjectionModeOptions* self =
    maplibre_native_ffi_projection_mode_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_PROJECTION_MODE_OPTION_AXONOMETRIC) != 0) {
    self->axonometric = (in->axonometric) ? TRUE : FALSE;
  }
  if ((in->fields & MLN_PROJECTION_MODE_OPTION_X_SKEW) != 0) {
    self->x_skew = (gdouble)(in->x_skew);
  }
  if ((in->fields & MLN_PROJECTION_MODE_OPTION_Y_SKEW) != 0) {
    self->y_skew = (gdouble)(in->y_skew);
  }
  return self;
}

struct _MaplibreNativeFfiQueueLock {
  MaplibreNativeFfiClosure* lock;
  MaplibreNativeFfiClosure* unlock;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiQueueLock, maplibre_native_ffi_queue_lock,
  maplibre_native_ffi_queue_lock_copy, maplibre_native_ffi_queue_lock_free
)

MaplibreNativeFfiQueueLock* maplibre_native_ffi_queue_lock_new(void) {
  MaplibreNativeFfiQueueLock* self = g_new0(MaplibreNativeFfiQueueLock, 1);
  return self;
}

MaplibreNativeFfiQueueLock* maplibre_native_ffi_queue_lock_copy(
  const MaplibreNativeFfiQueueLock* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiQueueLock* copy = g_memdup2(self, sizeof *self);
  maplibre_native_ffi_closure_ref(copy->lock);
  maplibre_native_ffi_closure_ref(copy->unlock);
  return copy;
}

void maplibre_native_ffi_queue_lock_free(MaplibreNativeFfiQueueLock* self) {
  if (self == NULL) return;
  maplibre_native_ffi_closure_unref(self->lock);
  maplibre_native_ffi_closure_unref(self->unlock);
  g_free(self);
}

void maplibre_native_ffi_queue_lock_set_lock(
  MaplibreNativeFfiQueueLock* self, MaplibreNativeFfiQueueLockCallback callback,
  gpointer user_data, GDestroyNotify notify
) {
  g_return_if_fail(self != NULL);
  MaplibreNativeFfiClosure* previous = self->lock;
  self->lock =
    callback != NULL
      ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify)
      : NULL;
  if (callback == NULL && notify != NULL) notify(user_data);
  maplibre_native_ffi_closure_unref(previous);
}

void maplibre_native_ffi_queue_lock_set_unlock(
  MaplibreNativeFfiQueueLock* self, MaplibreNativeFfiQueueLockCallback callback,
  gpointer user_data, GDestroyNotify notify
) {
  g_return_if_fail(self != NULL);
  MaplibreNativeFfiClosure* previous = self->unlock;
  self->unlock =
    callback != NULL
      ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify)
      : NULL;
  if (callback == NULL && notify != NULL) notify(user_data);
  maplibre_native_ffi_closure_unref(previous);
}

gpointer maplibre_native_ffi_queue_lock_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_queue_lock_copy(
    (const MaplibreNativeFfiQueueLock*)self
  );
}

static void queue_lock_lock_trampoline(void* user_data) {
  MaplibreNativeFfiClosure* closure =
    maplibre_native_ffi_registration_closure(user_data, 0);
  if (closure == NULL) return;
  ((MaplibreNativeFfiQueueLockCallback)(void (*)(void))closure->func)(
    closure->user_data
  );
}

static void queue_lock_unlock_trampoline(void* user_data) {
  MaplibreNativeFfiClosure* closure =
    maplibre_native_ffi_registration_closure(user_data, 1);
  if (closure == NULL) return;
  ((MaplibreNativeFfiQueueLockCallback)(void (*)(void))closure->func)(
    closure->user_data
  );
}

void maplibre_native_ffi_queue_lock_to_native(
  const MaplibreNativeFfiQueueLock* self, mln_queue_lock* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  if (self->lock != NULL || self->unlock != NULL) {
    MaplibreNativeFfiClosure* const closures[] = {self->lock, self->unlock};
    out->user_data = maplibre_native_ffi_call_register(call, closures, 2, NULL);
    out->release_user_data = maplibre_native_ffi_registration_release;
    out->lock = self->lock != NULL ? queue_lock_lock_trampoline : NULL;
    out->unlock = self->unlock != NULL ? queue_lock_unlock_trampoline : NULL;
  }
}

MaplibreNativeFfiQueueLock* maplibre_native_ffi_queue_lock_from_native(
  const mln_queue_lock* in
) {
  MaplibreNativeFfiQueueLock* self = maplibre_native_ffi_queue_lock_new();
  (void)in;
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRenderAbandonResult,
  maplibre_native_ffi_render_abandon_result,
  maplibre_native_ffi_render_abandon_result_copy,
  maplibre_native_ffi_render_abandon_result_free
)

MaplibreNativeFfiRenderAbandonResult*
maplibre_native_ffi_render_abandon_result_copy(
  const MaplibreNativeFfiRenderAbandonResult* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_render_abandon_result_free(
  MaplibreNativeFfiRenderAbandonResult* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_render_abandon_result_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_render_abandon_result_copy(
    (const MaplibreNativeFfiRenderAbandonResult*)self
  );
}

void maplibre_native_ffi_render_abandon_result_to_native(
  const MaplibreNativeFfiRenderAbandonResult* self,
  mln_render_abandon_result* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->disposition = (mln_render_abandon_disposition)(self->disposition);
  out->quarantined_resource_count =
    (uint32_t)(self->quarantined_resource_count);
}

void maplibre_native_ffi_render_abandon_result_read(
  MaplibreNativeFfiRenderAbandonResult* out, const mln_render_abandon_result* in
) {
  out->disposition =
    (MaplibreNativeFfiRenderAbandonDisposition)(in->disposition);
  out->quarantined_resource_count = (guint32)(in->quarantined_resource_count);
}

MaplibreNativeFfiRenderAbandonResult*
maplibre_native_ffi_render_abandon_result_from_native(
  const mln_render_abandon_result* in
) {
  MaplibreNativeFfiRenderAbandonResult* self =
    g_new0(MaplibreNativeFfiRenderAbandonResult, 1);
  maplibre_native_ffi_render_abandon_result_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRenderFrameResult, maplibre_native_ffi_render_frame_result,
  maplibre_native_ffi_render_frame_result_copy,
  maplibre_native_ffi_render_frame_result_free
)

MaplibreNativeFfiRenderFrameResult*
maplibre_native_ffi_render_frame_result_copy(
  const MaplibreNativeFfiRenderFrameResult* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_render_frame_result_free(
  MaplibreNativeFfiRenderFrameResult* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_render_frame_result_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_render_frame_result_copy(
    (const MaplibreNativeFfiRenderFrameResult*)self
  );
}

void maplibre_native_ffi_render_frame_result_to_native(
  const MaplibreNativeFfiRenderFrameResult* self, mln_render_frame_result* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->disposition = (mln_render_result)(self->disposition);
  out->token = (uint64_t)(self->token);
  out->map_update_generation = (uint64_t)(self->map_update_generation);
  out->generation = (uint64_t)(self->generation);
  out->frame_generation = (uint64_t)(self->frame_generation);
  out->needs_repaint = (self->needs_repaint) != FALSE;
  maplibre_native_ffi_map_view_to_native(&self->view, &out->view, call);
  out->mode = (mln_render_mode)(self->mode);
}

void maplibre_native_ffi_render_frame_result_read(
  MaplibreNativeFfiRenderFrameResult* out, const mln_render_frame_result* in
) {
  out->disposition = (MaplibreNativeFfiRenderResult)(in->disposition);
  out->token = (guint64)(in->token);
  out->map_update_generation = (guint64)(in->map_update_generation);
  out->generation = (guint64)(in->generation);
  out->frame_generation = (guint64)(in->frame_generation);
  out->needs_repaint = (in->needs_repaint) ? TRUE : FALSE;
  maplibre_native_ffi_map_view_read(&out->view, &in->view);
  out->mode = (MaplibreNativeFfiRenderMode)(in->mode);
}

MaplibreNativeFfiRenderFrameResult*
maplibre_native_ffi_render_frame_result_from_native(
  const mln_render_frame_result* in
) {
  MaplibreNativeFfiRenderFrameResult* self =
    g_new0(MaplibreNativeFfiRenderFrameResult, 1);
  maplibre_native_ffi_render_frame_result_read(self, in);
  return self;
}

struct _MaplibreNativeFfiWake {
  MaplibreNativeFfiClosure* callback;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiWake, maplibre_native_ffi_wake,
  maplibre_native_ffi_wake_copy, maplibre_native_ffi_wake_free
)

MaplibreNativeFfiWake* maplibre_native_ffi_wake_new(void) {
  MaplibreNativeFfiWake* self = g_new0(MaplibreNativeFfiWake, 1);
  return self;
}

MaplibreNativeFfiWake* maplibre_native_ffi_wake_copy(
  const MaplibreNativeFfiWake* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiWake* copy = g_memdup2(self, sizeof *self);
  maplibre_native_ffi_closure_ref(copy->callback);
  return copy;
}

void maplibre_native_ffi_wake_free(MaplibreNativeFfiWake* self) {
  if (self == NULL) return;
  maplibre_native_ffi_closure_unref(self->callback);
  g_free(self);
}

void maplibre_native_ffi_wake_set_callback(
  MaplibreNativeFfiWake* self, MaplibreNativeFfiWakeCallback callback,
  gpointer user_data, GDestroyNotify notify
) {
  g_return_if_fail(self != NULL);
  MaplibreNativeFfiClosure* previous = self->callback;
  self->callback =
    callback != NULL
      ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify)
      : NULL;
  if (callback == NULL && notify != NULL) notify(user_data);
  maplibre_native_ffi_closure_unref(previous);
}

gpointer maplibre_native_ffi_wake_copy_func(gconstpointer self, gpointer data) {
  (void)data;
  return maplibre_native_ffi_wake_copy((const MaplibreNativeFfiWake*)self);
}

static void wake_callback_trampoline(void* user_data) {
  MaplibreNativeFfiClosure* closure =
    maplibre_native_ffi_registration_closure(user_data, 0);
  if (closure == NULL) return;
  ((MaplibreNativeFfiWakeCallback)(void (*)(void))closure->func)(
    closure->user_data
  );
}

void maplibre_native_ffi_wake_to_native(
  const MaplibreNativeFfiWake* self, mln_wake* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  if (self->callback != NULL) {
    MaplibreNativeFfiClosure* const closures[] = {self->callback};
    out->user_data =
      maplibre_native_ffi_call_register(call, closures, 1, &out->queue);
    out->release_user_data = maplibre_native_ffi_registration_release;
    out->callback = self->callback != NULL ? wake_callback_trampoline : NULL;
  }
}

MaplibreNativeFfiWake* maplibre_native_ffi_wake_from_native(
  const mln_wake* in
) {
  MaplibreNativeFfiWake* self = maplibre_native_ffi_wake_new();
  (void)in;
  return self;
}

struct _MaplibreNativeFfiRenderSessionAttachOptions {
  MaplibreNativeFfiRenderDriverKind driver;
  MaplibreNativeFfiGpuSyncKindFlag producer_sync_kinds;
  MaplibreNativeFfiWake* frame_wake;
  MaplibreNativeFfiWake* redraw_wake;
  MaplibreNativeFfiWake* driver_work_wake;
  MaplibreNativeFfiQueueLock* queue_lock;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRenderSessionAttachOptions,
  maplibre_native_ffi_render_session_attach_options,
  maplibre_native_ffi_render_session_attach_options_copy,
  maplibre_native_ffi_render_session_attach_options_free
)

MaplibreNativeFfiRenderSessionAttachOptions*
maplibre_native_ffi_render_session_attach_options_new(void) {
  MaplibreNativeFfiRenderSessionAttachOptions* self =
    g_new0(MaplibreNativeFfiRenderSessionAttachOptions, 1);
  self->frame_wake = maplibre_native_ffi_wake_new();
  self->redraw_wake = maplibre_native_ffi_wake_new();
  self->driver_work_wake = maplibre_native_ffi_wake_new();
  self->queue_lock = maplibre_native_ffi_queue_lock_new();
  return self;
}

MaplibreNativeFfiRenderSessionAttachOptions*
maplibre_native_ffi_render_session_attach_options_copy(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiRenderSessionAttachOptions* copy =
    g_memdup2(self, sizeof *self);
  copy->frame_wake = self->frame_wake != NULL
                       ? maplibre_native_ffi_wake_copy(self->frame_wake)
                       : NULL;
  copy->redraw_wake = self->redraw_wake != NULL
                        ? maplibre_native_ffi_wake_copy(self->redraw_wake)
                        : NULL;
  copy->driver_work_wake =
    self->driver_work_wake != NULL
      ? maplibre_native_ffi_wake_copy(self->driver_work_wake)
      : NULL;
  copy->queue_lock = self->queue_lock != NULL
                       ? maplibre_native_ffi_queue_lock_copy(self->queue_lock)
                       : NULL;
  return copy;
}

void maplibre_native_ffi_render_session_attach_options_free(
  MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->frame_wake, maplibre_native_ffi_wake_free);
  g_clear_pointer(&self->redraw_wake, maplibre_native_ffi_wake_free);
  g_clear_pointer(&self->driver_work_wake, maplibre_native_ffi_wake_free);
  g_clear_pointer(&self->queue_lock, maplibre_native_ffi_queue_lock_free);
  g_free(self);
}

MaplibreNativeFfiRenderDriverKind
maplibre_native_ffi_render_session_attach_options_get_driver(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiRenderDriverKind)0);
  return self->driver;
}

void maplibre_native_ffi_render_session_attach_options_set_driver(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  MaplibreNativeFfiRenderDriverKind value
) {
  g_return_if_fail(self != NULL);
  self->driver = value;
}

MaplibreNativeFfiGpuSyncKindFlag
maplibre_native_ffi_render_session_attach_options_get_producer_sync_kinds(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiGpuSyncKindFlag)0);
  return self->producer_sync_kinds;
}

void maplibre_native_ffi_render_session_attach_options_set_producer_sync_kinds(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  MaplibreNativeFfiGpuSyncKindFlag value
) {
  g_return_if_fail(self != NULL);
  self->producer_sync_kinds = value;
}

MaplibreNativeFfiWake*
maplibre_native_ffi_render_session_attach_options_get_frame_wake(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->frame_wake;
}

void maplibre_native_ffi_render_session_attach_options_set_frame_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiWake* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiWake* previous = self->frame_wake;
  self->frame_wake = maplibre_native_ffi_wake_copy(value);
  if (previous != NULL) maplibre_native_ffi_wake_free(previous);
}

MaplibreNativeFfiWake*
maplibre_native_ffi_render_session_attach_options_get_redraw_wake(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->redraw_wake;
}

void maplibre_native_ffi_render_session_attach_options_set_redraw_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiWake* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiWake* previous = self->redraw_wake;
  self->redraw_wake = maplibre_native_ffi_wake_copy(value);
  if (previous != NULL) maplibre_native_ffi_wake_free(previous);
}

MaplibreNativeFfiWake*
maplibre_native_ffi_render_session_attach_options_get_driver_work_wake(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->driver_work_wake;
}

void maplibre_native_ffi_render_session_attach_options_set_driver_work_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiWake* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiWake* previous = self->driver_work_wake;
  self->driver_work_wake = maplibre_native_ffi_wake_copy(value);
  if (previous != NULL) maplibre_native_ffi_wake_free(previous);
}

MaplibreNativeFfiQueueLock*
maplibre_native_ffi_render_session_attach_options_get_queue_lock(
  const MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->queue_lock;
}

void maplibre_native_ffi_render_session_attach_options_set_queue_lock(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiQueueLock* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiQueueLock* previous = self->queue_lock;
  self->queue_lock = maplibre_native_ffi_queue_lock_copy(value);
  if (previous != NULL) maplibre_native_ffi_queue_lock_free(previous);
}

gpointer maplibre_native_ffi_render_session_attach_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_render_session_attach_options_copy(
    (const MaplibreNativeFfiRenderSessionAttachOptions*)self
  );
}

void maplibre_native_ffi_render_session_attach_options_to_native(
  const MaplibreNativeFfiRenderSessionAttachOptions* self,
  mln_render_session_attach_options* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->driver = (mln_render_driver_kind)(self->driver);
  out->producer_sync_kinds =
    (mln_gpu_sync_kind_flags)(self->producer_sync_kinds);
  if (self->frame_wake != NULL)
    maplibre_native_ffi_wake_to_native(
      self->frame_wake, &out->frame_wake, call
    );
  if (self->redraw_wake != NULL)
    maplibre_native_ffi_wake_to_native(
      self->redraw_wake, &out->redraw_wake, call
    );
  if (self->driver_work_wake != NULL)
    maplibre_native_ffi_wake_to_native(
      self->driver_work_wake, &out->driver_work_wake, call
    );
  if (self->queue_lock != NULL)
    maplibre_native_ffi_queue_lock_to_native(
      self->queue_lock, &out->queue_lock, call
    );
}

MaplibreNativeFfiRenderSessionAttachOptions*
maplibre_native_ffi_render_session_attach_options_from_native(
  const mln_render_session_attach_options* in
) {
  MaplibreNativeFfiRenderSessionAttachOptions* self =
    maplibre_native_ffi_render_session_attach_options_new();
  self->driver = (MaplibreNativeFfiRenderDriverKind)(in->driver);
  self->producer_sync_kinds =
    (MaplibreNativeFfiGpuSyncKindFlag)(in->producer_sync_kinds);
  g_clear_pointer(&self->frame_wake, maplibre_native_ffi_wake_free);
  self->frame_wake = maplibre_native_ffi_wake_from_native(&in->frame_wake);
  g_clear_pointer(&self->redraw_wake, maplibre_native_ffi_wake_free);
  self->redraw_wake = maplibre_native_ffi_wake_from_native(&in->redraw_wake);
  g_clear_pointer(&self->driver_work_wake, maplibre_native_ffi_wake_free);
  self->driver_work_wake =
    maplibre_native_ffi_wake_from_native(&in->driver_work_wake);
  g_clear_pointer(&self->queue_lock, maplibre_native_ffi_queue_lock_free);
  self->queue_lock =
    maplibre_native_ffi_queue_lock_from_native(&in->queue_lock);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRenderSessionCapabilities,
  maplibre_native_ffi_render_session_capabilities,
  maplibre_native_ffi_render_session_capabilities_copy,
  maplibre_native_ffi_render_session_capabilities_free
)

MaplibreNativeFfiRenderSessionCapabilities*
maplibre_native_ffi_render_session_capabilities_copy(
  const MaplibreNativeFfiRenderSessionCapabilities* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_render_session_capabilities_free(
  MaplibreNativeFfiRenderSessionCapabilities* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_render_session_capabilities_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_render_session_capabilities_copy(
    (const MaplibreNativeFfiRenderSessionCapabilities*)self
  );
}

void maplibre_native_ffi_render_session_capabilities_to_native(
  const MaplibreNativeFfiRenderSessionCapabilities* self,
  mln_render_session_capabilities* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->driver = (mln_render_driver_kind)(self->driver);
  out->texture_ring_depth = (uint32_t)(self->texture_ring_depth);
  out->flags = (mln_render_session_capability_flags)(self->flags);
  out->producer_sync_kind = (mln_gpu_sync_kind)(self->producer_sync_kind);
  out->consumer_sync_kinds =
    (mln_gpu_sync_kind_flags)(self->consumer_sync_kinds);
}

void maplibre_native_ffi_render_session_capabilities_read(
  MaplibreNativeFfiRenderSessionCapabilities* out,
  const mln_render_session_capabilities* in
) {
  out->driver = (MaplibreNativeFfiRenderDriverKind)(in->driver);
  out->texture_ring_depth = (guint32)(in->texture_ring_depth);
  out->flags = (MaplibreNativeFfiRenderSessionCapabilityFlag)(in->flags);
  out->producer_sync_kind =
    (MaplibreNativeFfiGpuSyncKind)(in->producer_sync_kind);
  out->consumer_sync_kinds =
    (MaplibreNativeFfiGpuSyncKindFlag)(in->consumer_sync_kinds);
}

MaplibreNativeFfiRenderSessionCapabilities*
maplibre_native_ffi_render_session_capabilities_from_native(
  const mln_render_session_capabilities* in
) {
  MaplibreNativeFfiRenderSessionCapabilities* self =
    g_new0(MaplibreNativeFfiRenderSessionCapabilities, 1);
  maplibre_native_ffi_render_session_capabilities_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRenderSessionSnapshot,
  maplibre_native_ffi_render_session_snapshot,
  maplibre_native_ffi_render_session_snapshot_copy,
  maplibre_native_ffi_render_session_snapshot_free
)

MaplibreNativeFfiRenderSessionSnapshot*
maplibre_native_ffi_render_session_snapshot_copy(
  const MaplibreNativeFfiRenderSessionSnapshot* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_render_session_snapshot_free(
  MaplibreNativeFfiRenderSessionSnapshot* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_render_session_snapshot_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_render_session_snapshot_copy(
    (const MaplibreNativeFfiRenderSessionSnapshot*)self
  );
}

void maplibre_native_ffi_render_session_snapshot_to_native(
  const MaplibreNativeFfiRenderSessionSnapshot* self,
  mln_render_session_snapshot* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->state = (mln_render_session_state)(self->state);
  maplibre_native_ffi_logical_extent_to_native(
    &self->extent, &out->extent, call
  );
  out->generation = (uint64_t)(self->generation);
  out->frame_generation = (uint64_t)(self->frame_generation);
  out->acquired_frame_count = (uint32_t)(self->acquired_frame_count);
  out->failure_status = (mln_status)(self->failure_status);
  out->has_target = (self->has_target) != FALSE;
}

void maplibre_native_ffi_render_session_snapshot_read(
  MaplibreNativeFfiRenderSessionSnapshot* out,
  const mln_render_session_snapshot* in
) {
  out->state = (MaplibreNativeFfiRenderSessionState)(in->state);
  maplibre_native_ffi_logical_extent_read(&out->extent, &in->extent);
  out->generation = (guint64)(in->generation);
  out->frame_generation = (guint64)(in->frame_generation);
  out->acquired_frame_count = (guint32)(in->acquired_frame_count);
  out->failure_status = (MaplibreNativeFfiError)(in->failure_status);
  out->has_target = (in->has_target) ? TRUE : FALSE;
}

MaplibreNativeFfiRenderSessionSnapshot*
maplibre_native_ffi_render_session_snapshot_from_native(
  const mln_render_session_snapshot* in
) {
  MaplibreNativeFfiRenderSessionSnapshot* self =
    g_new0(MaplibreNativeFfiRenderSessionSnapshot, 1);
  maplibre_native_ffi_render_session_snapshot_read(self, in);
  return self;
}

struct _MaplibreNativeFfiRenderedFeature {
  char* feature;
  char* layer_id;
  char* source_id;
  char* source_layer_id;
  char* state;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRenderedFeature, maplibre_native_ffi_rendered_feature,
  maplibre_native_ffi_rendered_feature_copy,
  maplibre_native_ffi_rendered_feature_free
)

MaplibreNativeFfiRenderedFeature* maplibre_native_ffi_rendered_feature_new(
  void
) {
  MaplibreNativeFfiRenderedFeature* self =
    g_new0(MaplibreNativeFfiRenderedFeature, 1);
  self->feature = g_strdup("");
  self->layer_id = g_strdup("");
  self->source_id = g_strdup("");
  self->source_layer_id = g_strdup("");
  self->state = g_strdup("");
  return self;
}

MaplibreNativeFfiRenderedFeature* maplibre_native_ffi_rendered_feature_copy(
  const MaplibreNativeFfiRenderedFeature* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiRenderedFeature* copy = g_memdup2(self, sizeof *self);
  copy->feature = g_strdup(self->feature);
  copy->layer_id = g_strdup(self->layer_id);
  copy->source_id = g_strdup(self->source_id);
  copy->source_layer_id = g_strdup(self->source_layer_id);
  copy->state = g_strdup(self->state);
  return copy;
}

void maplibre_native_ffi_rendered_feature_free(
  MaplibreNativeFfiRenderedFeature* self
) {
  if (self == NULL) return;
  g_free(self->feature);
  g_free(self->layer_id);
  g_free(self->source_id);
  g_free(self->source_layer_id);
  g_free(self->state);
  g_free(self);
}

const char* maplibre_native_ffi_rendered_feature_get_feature(
  const MaplibreNativeFfiRenderedFeature* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->feature;
}

const char* maplibre_native_ffi_rendered_feature_get_layer_id(
  const MaplibreNativeFfiRenderedFeature* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->layer_id;
}

const char* maplibre_native_ffi_rendered_feature_get_source_id(
  const MaplibreNativeFfiRenderedFeature* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->source_id;
}

const char* maplibre_native_ffi_rendered_feature_get_source_layer_id(
  const MaplibreNativeFfiRenderedFeature* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->source_layer_id;
}

const char* maplibre_native_ffi_rendered_feature_get_state(
  const MaplibreNativeFfiRenderedFeature* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->state;
}

gpointer maplibre_native_ffi_rendered_feature_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_rendered_feature_copy(
    (const MaplibreNativeFfiRenderedFeature*)self
  );
}

void maplibre_native_ffi_rendered_feature_to_native(
  const MaplibreNativeFfiRenderedFeature* self, mln_rendered_feature* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->feature = maplibre_native_ffi_text_view(self->feature);
  out->layer_id = maplibre_native_ffi_text_view(self->layer_id);
  out->source_id = maplibre_native_ffi_text_view(self->source_id);
  out->source_layer_id = maplibre_native_ffi_text_view(self->source_layer_id);
  out->state = maplibre_native_ffi_text_view(self->state);
}

MaplibreNativeFfiRenderedFeature*
maplibre_native_ffi_rendered_feature_from_native(
  const mln_rendered_feature* in
) {
  MaplibreNativeFfiRenderedFeature* self =
    maplibre_native_ffi_rendered_feature_new();
  g_free(self->feature);
  self->feature = maplibre_native_ffi_text_new(in->feature);
  g_free(self->layer_id);
  self->layer_id = maplibre_native_ffi_text_new(in->layer_id);
  g_free(self->source_id);
  self->source_id = maplibre_native_ffi_text_new(in->source_id);
  g_free(self->source_layer_id);
  self->source_layer_id = maplibre_native_ffi_text_new(in->source_layer_id);
  g_free(self->state);
  self->state = maplibre_native_ffi_text_new(in->state);
  return self;
}

struct _MaplibreNativeFfiRenderedFeatureQueryOptions {
  guint64 fields;
  char** layer_ids;
  char* filter;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRenderedFeatureQueryOptions,
  maplibre_native_ffi_rendered_feature_query_options,
  maplibre_native_ffi_rendered_feature_query_options_copy,
  maplibre_native_ffi_rendered_feature_query_options_free
)

MaplibreNativeFfiRenderedFeatureQueryOptions*
maplibre_native_ffi_rendered_feature_query_options_new(void) {
  MaplibreNativeFfiRenderedFeatureQueryOptions* self =
    g_new0(MaplibreNativeFfiRenderedFeatureQueryOptions, 1);
  self->layer_ids = g_new0(char*, 1);
  self->filter = g_strdup("");
  return self;
}

MaplibreNativeFfiRenderedFeatureQueryOptions*
maplibre_native_ffi_rendered_feature_query_options_copy(
  const MaplibreNativeFfiRenderedFeatureQueryOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiRenderedFeatureQueryOptions* copy =
    g_memdup2(self, sizeof *self);
  copy->layer_ids = g_strdupv(self->layer_ids);
  copy->filter = g_strdup(self->filter);
  return copy;
}

void maplibre_native_ffi_rendered_feature_query_options_free(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self
) {
  if (self == NULL) return;
  g_strfreev(self->layer_ids);
  g_free(self->filter);
  g_free(self);
}

const char* const*
maplibre_native_ffi_rendered_feature_query_options_get_layer_ids(
  const MaplibreNativeFfiRenderedFeatureQueryOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (const char* const*)self->layer_ids;
}

gboolean maplibre_native_ffi_rendered_feature_query_options_has_layer_ids(
  const MaplibreNativeFfiRenderedFeatureQueryOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS) != 0;
}

void maplibre_native_ffi_rendered_feature_query_options_set_layer_ids(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self, const char* const* value
) {
  g_return_if_fail(self != NULL);
  char** previous = self->layer_ids;
  self->layer_ids = value != NULL ? g_strdupv((char**)value) : g_new0(char*, 1);
  g_strfreev(previous);
  self->fields |= MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
}

const char* maplibre_native_ffi_rendered_feature_query_options_get_filter(
  const MaplibreNativeFfiRenderedFeatureQueryOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->filter;
}

void maplibre_native_ffi_rendered_feature_query_options_set_filter(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->filter;
  self->filter = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

gpointer maplibre_native_ffi_rendered_feature_query_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_rendered_feature_query_options_copy(
    (const MaplibreNativeFfiRenderedFeatureQueryOptions*)self
  );
}

void maplibre_native_ffi_rendered_feature_query_options_to_native(
  const MaplibreNativeFfiRenderedFeatureQueryOptions* self,
  mln_rendered_feature_query_options* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_rendered_feature_query_option_fields)self->fields;
  if (self->layer_ids != NULL && self->layer_ids[0] != NULL) {
    guint count = g_strv_length(self->layer_ids);
    mln_buffer_view* items =
      maplibre_native_ffi_call_alloc(call, count, sizeof *items);
    for (guint index = 0; index < count; index++) {
      items[index] = maplibre_native_ffi_text_view(self->layer_ids[index]);
    }
    out->layer_ids = items;
    out->layer_id_count = count;
  }
  out->filter = maplibre_native_ffi_text_view(self->filter);
}

MaplibreNativeFfiRenderedFeatureQueryOptions*
maplibre_native_ffi_rendered_feature_query_options_from_native(
  const mln_rendered_feature_query_options* in
) {
  MaplibreNativeFfiRenderedFeatureQueryOptions* self =
    maplibre_native_ffi_rendered_feature_query_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS) != 0) {
    g_strfreev(self->layer_ids);
    self->layer_ids = g_new0(char*, in->layer_id_count + 1);
    for (size_t index = 0; index < in->layer_id_count; index++) {
      self->layer_ids[index] =
        maplibre_native_ffi_text_new(in->layer_ids[index]);
    }
  }
  g_free(self->filter);
  self->filter = maplibre_native_ffi_text_new(in->filter);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiScreenBox, maplibre_native_ffi_screen_box,
  maplibre_native_ffi_screen_box_copy, maplibre_native_ffi_screen_box_free
)

MaplibreNativeFfiScreenBox* maplibre_native_ffi_screen_box_copy(
  const MaplibreNativeFfiScreenBox* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_screen_box_free(MaplibreNativeFfiScreenBox* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_screen_box_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_screen_box_copy(
    (const MaplibreNativeFfiScreenBox*)self
  );
}

void maplibre_native_ffi_screen_box_to_native(
  const MaplibreNativeFfiScreenBox* self, mln_screen_box* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  maplibre_native_ffi_screen_point_to_native(&self->min, &out->min, call);
  maplibre_native_ffi_screen_point_to_native(&self->max, &out->max, call);
}

void maplibre_native_ffi_screen_box_read(
  MaplibreNativeFfiScreenBox* out, const mln_screen_box* in
) {
  maplibre_native_ffi_screen_point_read(&out->min, &in->min);
  maplibre_native_ffi_screen_point_read(&out->max, &in->max);
}

MaplibreNativeFfiScreenBox* maplibre_native_ffi_screen_box_from_native(
  const mln_screen_box* in
) {
  MaplibreNativeFfiScreenBox* self = g_new0(MaplibreNativeFfiScreenBox, 1);
  maplibre_native_ffi_screen_box_read(self, in);
  return self;
}

struct _MaplibreNativeFfiScreenLineString {
  GPtrArray* points;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiScreenLineString, maplibre_native_ffi_screen_line_string,
  maplibre_native_ffi_screen_line_string_copy,
  maplibre_native_ffi_screen_line_string_free
)

MaplibreNativeFfiScreenLineString* maplibre_native_ffi_screen_line_string_new(
  void
) {
  MaplibreNativeFfiScreenLineString* self =
    g_new0(MaplibreNativeFfiScreenLineString, 1);
  self->points = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_screen_point_free
  );
  return self;
}

MaplibreNativeFfiScreenLineString* maplibre_native_ffi_screen_line_string_copy(
  const MaplibreNativeFfiScreenLineString* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiScreenLineString* copy = g_memdup2(self, sizeof *self);
  copy->points =
    self->points != NULL
      ? g_ptr_array_copy(
          self->points, maplibre_native_ffi_screen_point_copy_func, NULL
        )
      : NULL;
  if (copy->points != NULL)
    g_ptr_array_set_free_func(
      copy->points, (GDestroyNotify)maplibre_native_ffi_screen_point_free
    );
  return copy;
}

void maplibre_native_ffi_screen_line_string_free(
  MaplibreNativeFfiScreenLineString* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->points, g_ptr_array_unref);
  g_free(self);
}

GPtrArray* maplibre_native_ffi_screen_line_string_get_points(
  const MaplibreNativeFfiScreenLineString* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->points;
}

void maplibre_native_ffi_screen_line_string_set_points(
  MaplibreNativeFfiScreenLineString* self,
  const MaplibreNativeFfiScreenPoint* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->points;
  self->points = g_ptr_array_new_full(
    (guint)value_count, (GDestroyNotify)maplibre_native_ffi_screen_point_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->points, maplibre_native_ffi_screen_point_copy(&value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

gpointer maplibre_native_ffi_screen_line_string_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_screen_line_string_copy(
    (const MaplibreNativeFfiScreenLineString*)self
  );
}

void maplibre_native_ffi_screen_line_string_to_native(
  const MaplibreNativeFfiScreenLineString* self, mln_screen_line_string* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  if (self->points != NULL && self->points->len > 0) {
    mln_screen_point* items =
      maplibre_native_ffi_call_alloc(call, self->points->len, sizeof *items);
    for (guint index = 0; index < self->points->len; index++) {
      maplibre_native_ffi_screen_point_to_native(
        self->points->pdata[index], &items[index], call
      );
    }
    out->points = items;
    out->point_count = self->points->len;
  }
}

MaplibreNativeFfiScreenLineString*
maplibre_native_ffi_screen_line_string_from_native(
  const mln_screen_line_string* in
) {
  MaplibreNativeFfiScreenLineString* self =
    maplibre_native_ffi_screen_line_string_new();
  g_clear_pointer(&self->points, g_ptr_array_unref);
  self->points = g_ptr_array_new_full(
    (guint)in->point_count,
    (GDestroyNotify)maplibre_native_ffi_screen_point_free
  );
  for (size_t index = 0; index < in->point_count; index++) {
    g_ptr_array_add(
      self->points,
      maplibre_native_ffi_screen_point_from_native(&in->points[index])
    );
  }
  return self;
}

struct _MaplibreNativeFfiRenderedQueryGeometry {
  gint64 tag;
  MaplibreNativeFfiScreenPoint point;
  MaplibreNativeFfiScreenBox box;
  MaplibreNativeFfiScreenLineString* line_string;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRenderedQueryGeometry,
  maplibre_native_ffi_rendered_query_geometry,
  maplibre_native_ffi_rendered_query_geometry_copy,
  maplibre_native_ffi_rendered_query_geometry_free
)

MaplibreNativeFfiRenderedQueryGeometry*
maplibre_native_ffi_rendered_query_geometry_copy(
  const MaplibreNativeFfiRenderedQueryGeometry* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiRenderedQueryGeometry* copy = g_memdup2(self, sizeof *self);
  copy->line_string =
    self->line_string != NULL
      ? maplibre_native_ffi_screen_line_string_copy(self->line_string)
      : NULL;
  return copy;
}

void maplibre_native_ffi_rendered_query_geometry_free(
  MaplibreNativeFfiRenderedQueryGeometry* self
) {
  if (self == NULL) return;
  g_clear_pointer(
    &self->line_string, maplibre_native_ffi_screen_line_string_free
  );
  g_free(self);
}

MaplibreNativeFfiRenderedQueryGeometry*
maplibre_native_ffi_rendered_query_geometry_new_point(
  const MaplibreNativeFfiScreenPoint* value
) {
  g_return_val_if_fail(value != NULL, NULL);
  MaplibreNativeFfiRenderedQueryGeometry* self =
    g_new0(MaplibreNativeFfiRenderedQueryGeometry, 1);
  self->tag = MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT;
  self->point = *value;
  return self;
}

gboolean maplibre_native_ffi_rendered_query_geometry_is_point(
  const MaplibreNativeFfiRenderedQueryGeometry* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT;
}

MaplibreNativeFfiScreenPoint*
maplibre_native_ffi_rendered_query_geometry_get_point(
  const MaplibreNativeFfiRenderedQueryGeometry* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(
    self->tag == MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT, NULL
  );
  return (MaplibreNativeFfiScreenPoint*)&self->point;
}

MaplibreNativeFfiRenderedQueryGeometry*
maplibre_native_ffi_rendered_query_geometry_new_box(
  const MaplibreNativeFfiScreenBox* value
) {
  g_return_val_if_fail(value != NULL, NULL);
  MaplibreNativeFfiRenderedQueryGeometry* self =
    g_new0(MaplibreNativeFfiRenderedQueryGeometry, 1);
  self->tag = MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX;
  self->box = *value;
  return self;
}

gboolean maplibre_native_ffi_rendered_query_geometry_is_box(
  const MaplibreNativeFfiRenderedQueryGeometry* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX;
}

MaplibreNativeFfiScreenBox* maplibre_native_ffi_rendered_query_geometry_get_box(
  const MaplibreNativeFfiRenderedQueryGeometry* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(self->tag == MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX, NULL);
  return (MaplibreNativeFfiScreenBox*)&self->box;
}

MaplibreNativeFfiRenderedQueryGeometry*
maplibre_native_ffi_rendered_query_geometry_new_line_string(
  const MaplibreNativeFfiScreenLineString* value
) {
  g_return_val_if_fail(value != NULL, NULL);
  MaplibreNativeFfiRenderedQueryGeometry* self =
    g_new0(MaplibreNativeFfiRenderedQueryGeometry, 1);
  self->tag = MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING;
  self->line_string = maplibre_native_ffi_screen_line_string_new();
  MaplibreNativeFfiScreenLineString* previous = self->line_string;
  self->line_string = maplibre_native_ffi_screen_line_string_copy(value);
  if (previous != NULL) maplibre_native_ffi_screen_line_string_free(previous);
  return self;
}

gboolean maplibre_native_ffi_rendered_query_geometry_is_line_string(
  const MaplibreNativeFfiRenderedQueryGeometry* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING;
}

MaplibreNativeFfiScreenLineString*
maplibre_native_ffi_rendered_query_geometry_get_line_string(
  const MaplibreNativeFfiRenderedQueryGeometry* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(
    self->tag == MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING, NULL
  );
  return self->line_string;
}

void maplibre_native_ffi_rendered_query_geometry_to_native(
  const MaplibreNativeFfiRenderedQueryGeometry* self,
  mln_rendered_query_geometry* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->type = (mln_rendered_query_geometry_type)self->tag;
  mln_rendered_query_geometry_data* data = &out->data;
  switch (self->tag) {
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT:
      maplibre_native_ffi_screen_point_to_native(
        &self->point, &data->point, call
      );
      break;
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX:
      maplibre_native_ffi_screen_box_to_native(&self->box, &data->box, call);
      break;
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING:
      if (self->line_string != NULL)
        maplibre_native_ffi_screen_line_string_to_native(
          self->line_string, &data->line_string, call
        );
      break;
    default:
      break;
  }
}

MaplibreNativeFfiRenderedQueryGeometry*
maplibre_native_ffi_rendered_query_geometry_from_native(
  const mln_rendered_query_geometry* in
) {
  gint64 tag = (gint64)in->type;
  const mln_rendered_query_geometry_data* data = &in->data;
  MaplibreNativeFfiRenderedQueryGeometry* self =
    g_new0(MaplibreNativeFfiRenderedQueryGeometry, 1);
  self->tag = tag;
  switch (tag) {
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT:
      maplibre_native_ffi_screen_point_read(&self->point, &data->point);
      break;
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX:
      maplibre_native_ffi_screen_box_read(&self->box, &data->box);
      break;
    case MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING:
      self->line_string =
        maplibre_native_ffi_screen_line_string_from_native(&data->line_string);
      break;
    default:
      break;
  }
  return self;
}

gpointer maplibre_native_ffi_rendered_query_geometry_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_rendered_query_geometry_copy(
    (const MaplibreNativeFfiRenderedQueryGeometry*)self
  );
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRenderingStats, maplibre_native_ffi_rendering_stats,
  maplibre_native_ffi_rendering_stats_copy,
  maplibre_native_ffi_rendering_stats_free
)

MaplibreNativeFfiRenderingStats* maplibre_native_ffi_rendering_stats_copy(
  const MaplibreNativeFfiRenderingStats* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_rendering_stats_free(
  MaplibreNativeFfiRenderingStats* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_rendering_stats_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_rendering_stats_copy(
    (const MaplibreNativeFfiRenderingStats*)self
  );
}

void maplibre_native_ffi_rendering_stats_to_native(
  const MaplibreNativeFfiRenderingStats* self, mln_rendering_stats* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->encoding_time_ms = (double)(self->encoding_time_ms);
  out->rendering_time_ms = (double)(self->rendering_time_ms);
  out->frame_count = (int64_t)(self->frame_count);
  out->draw_call_count = (int64_t)(self->draw_call_count);
  out->total_draw_call_count = (int64_t)(self->total_draw_call_count);
}

void maplibre_native_ffi_rendering_stats_read(
  MaplibreNativeFfiRenderingStats* out, const mln_rendering_stats* in
) {
  out->encoding_time_ms = (gdouble)(in->encoding_time_ms);
  out->rendering_time_ms = (gdouble)(in->rendering_time_ms);
  out->frame_count = (gint64)(in->frame_count);
  out->draw_call_count = (gint64)(in->draw_call_count);
  out->total_draw_call_count = (gint64)(in->total_draw_call_count);
}

MaplibreNativeFfiRenderingStats*
maplibre_native_ffi_rendering_stats_from_native(const mln_rendering_stats* in) {
  MaplibreNativeFfiRenderingStats* self =
    g_new0(MaplibreNativeFfiRenderingStats, 1);
  maplibre_native_ffi_rendering_stats_read(self, in);
  return self;
}

struct _MaplibreNativeFfiResourceRoute {
  MaplibreNativeFfiResourceKindFlag kinds;
  MaplibreNativeFfiUrlMatchFlag flags;
  char* url;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiResourceRoute, maplibre_native_ffi_resource_route,
  maplibre_native_ffi_resource_route_copy,
  maplibre_native_ffi_resource_route_free
)

MaplibreNativeFfiResourceRoute* maplibre_native_ffi_resource_route_new(void) {
  MaplibreNativeFfiResourceRoute* self =
    g_new0(MaplibreNativeFfiResourceRoute, 1);
  self->url = g_strdup("");
  return self;
}

MaplibreNativeFfiResourceRoute* maplibre_native_ffi_resource_route_copy(
  const MaplibreNativeFfiResourceRoute* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiResourceRoute* copy = g_memdup2(self, sizeof *self);
  copy->url = g_strdup(self->url);
  return copy;
}

void maplibre_native_ffi_resource_route_free(
  MaplibreNativeFfiResourceRoute* self
) {
  if (self == NULL) return;
  g_free(self->url);
  g_free(self);
}

MaplibreNativeFfiResourceKindFlag maplibre_native_ffi_resource_route_get_kinds(
  const MaplibreNativeFfiResourceRoute* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiResourceKindFlag)0);
  return self->kinds;
}

void maplibre_native_ffi_resource_route_set_kinds(
  MaplibreNativeFfiResourceRoute* self, MaplibreNativeFfiResourceKindFlag value
) {
  g_return_if_fail(self != NULL);
  self->kinds = value;
}

MaplibreNativeFfiUrlMatchFlag maplibre_native_ffi_resource_route_get_flags(
  const MaplibreNativeFfiResourceRoute* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiUrlMatchFlag)0);
  return self->flags;
}

void maplibre_native_ffi_resource_route_set_flags(
  MaplibreNativeFfiResourceRoute* self, MaplibreNativeFfiUrlMatchFlag value
) {
  g_return_if_fail(self != NULL);
  self->flags = value;
}

const char* maplibre_native_ffi_resource_route_get_url(
  const MaplibreNativeFfiResourceRoute* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->url;
}

void maplibre_native_ffi_resource_route_set_url(
  MaplibreNativeFfiResourceRoute* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->url;
  self->url = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

gpointer maplibre_native_ffi_resource_route_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_resource_route_copy(
    (const MaplibreNativeFfiResourceRoute*)self
  );
}

void maplibre_native_ffi_resource_route_to_native(
  const MaplibreNativeFfiResourceRoute* self, mln_resource_route* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->kinds = (mln_resource_kind_flags)(self->kinds);
  out->flags = (mln_url_match_flags)(self->flags);
  out->url = maplibre_native_ffi_text_view(self->url);
}

MaplibreNativeFfiResourceRoute* maplibre_native_ffi_resource_route_from_native(
  const mln_resource_route* in
) {
  MaplibreNativeFfiResourceRoute* self =
    maplibre_native_ffi_resource_route_new();
  self->kinds = (MaplibreNativeFfiResourceKindFlag)(in->kinds);
  self->flags = (MaplibreNativeFfiUrlMatchFlag)(in->flags);
  g_free(self->url);
  self->url = maplibre_native_ffi_text_new(in->url);
  return self;
}

struct _MaplibreNativeFfiResourceProvider {
  GPtrArray* routes;
  MaplibreNativeFfiClosure* callback;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiResourceProvider, maplibre_native_ffi_resource_provider,
  maplibre_native_ffi_resource_provider_copy,
  maplibre_native_ffi_resource_provider_free
)

MaplibreNativeFfiResourceProvider* maplibre_native_ffi_resource_provider_new(
  void
) {
  MaplibreNativeFfiResourceProvider* self =
    g_new0(MaplibreNativeFfiResourceProvider, 1);
  self->routes = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_resource_route_free
  );
  return self;
}

MaplibreNativeFfiResourceProvider* maplibre_native_ffi_resource_provider_copy(
  const MaplibreNativeFfiResourceProvider* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiResourceProvider* copy = g_memdup2(self, sizeof *self);
  copy->routes =
    self->routes != NULL
      ? g_ptr_array_copy(
          self->routes, maplibre_native_ffi_resource_route_copy_func, NULL
        )
      : NULL;
  if (copy->routes != NULL)
    g_ptr_array_set_free_func(
      copy->routes, (GDestroyNotify)maplibre_native_ffi_resource_route_free
    );
  maplibre_native_ffi_closure_ref(copy->callback);
  return copy;
}

void maplibre_native_ffi_resource_provider_free(
  MaplibreNativeFfiResourceProvider* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->routes, g_ptr_array_unref);
  maplibre_native_ffi_closure_unref(self->callback);
  g_free(self);
}

GPtrArray* maplibre_native_ffi_resource_provider_get_routes(
  const MaplibreNativeFfiResourceProvider* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->routes;
}

void maplibre_native_ffi_resource_provider_set_routes(
  MaplibreNativeFfiResourceProvider* self,
  MaplibreNativeFfiResourceRoute* const* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->routes;
  self->routes = g_ptr_array_new_full(
    (guint)value_count, (GDestroyNotify)maplibre_native_ffi_resource_route_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->routes, maplibre_native_ffi_resource_route_copy(value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

void maplibre_native_ffi_resource_provider_set_callback(
  MaplibreNativeFfiResourceProvider* self,
  MaplibreNativeFfiResourceProviderCallback callback, gpointer user_data,
  GDestroyNotify notify
) {
  g_return_if_fail(self != NULL);
  MaplibreNativeFfiClosure* previous = self->callback;
  self->callback =
    callback != NULL
      ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify)
      : NULL;
  if (callback == NULL && notify != NULL) notify(user_data);
  maplibre_native_ffi_closure_unref(previous);
}

gpointer maplibre_native_ffi_resource_provider_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_resource_provider_copy(
    (const MaplibreNativeFfiResourceProvider*)self
  );
}

static void resource_provider_callback_trampoline(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  MaplibreNativeFfiClosure* closure =
    maplibre_native_ffi_registration_closure(user_data, 0);
  if (closure == NULL) return;
  MaplibreNativeFfiResourceRequestInfo* request_ =
    maplibre_native_ffi_resource_request_info_from_native(request);
  MaplibreNativeFfiResourceRequest* handle_ =
    maplibre_native_ffi_resource_request_wrap(handle, NULL, FALSE, NULL);
  ((MaplibreNativeFfiResourceProviderCallback)(void (*)(void))closure->func)(
    request_, handle_, closure->user_data
  );
  maplibre_native_ffi_resource_request_info_free(request_);
  g_object_unref(handle_);
}

void maplibre_native_ffi_resource_provider_to_native(
  const MaplibreNativeFfiResourceProvider* self, mln_resource_provider* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  if (self->routes != NULL && self->routes->len > 0) {
    mln_resource_route* items =
      maplibre_native_ffi_call_alloc(call, self->routes->len, sizeof *items);
    for (guint index = 0; index < self->routes->len; index++) {
      maplibre_native_ffi_resource_route_to_native(
        self->routes->pdata[index], &items[index], call
      );
    }
    out->routes = items;
    out->route_count = self->routes->len;
  }
  if (self->callback != NULL) {
    MaplibreNativeFfiClosure* const closures[] = {self->callback};
    out->user_data =
      maplibre_native_ffi_call_register(call, closures, 1, &out->queue);
    out->release_user_data = maplibre_native_ffi_registration_release;
    out->callback =
      self->callback != NULL ? resource_provider_callback_trampoline : NULL;
  }
}

MaplibreNativeFfiResourceProvider*
maplibre_native_ffi_resource_provider_from_native(
  const mln_resource_provider* in
) {
  MaplibreNativeFfiResourceProvider* self =
    maplibre_native_ffi_resource_provider_new();
  g_clear_pointer(&self->routes, g_ptr_array_unref);
  self->routes = g_ptr_array_new_full(
    (guint)in->route_count,
    (GDestroyNotify)maplibre_native_ffi_resource_route_free
  );
  for (size_t index = 0; index < in->route_count; index++) {
    g_ptr_array_add(
      self->routes,
      maplibre_native_ffi_resource_route_from_native(&in->routes[index])
    );
  }
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiResourceRange, maplibre_native_ffi_resource_range,
  maplibre_native_ffi_resource_range_copy,
  maplibre_native_ffi_resource_range_free
)

MaplibreNativeFfiResourceRange* maplibre_native_ffi_resource_range_copy(
  const MaplibreNativeFfiResourceRange* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_resource_range_free(
  MaplibreNativeFfiResourceRange* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_resource_range_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_resource_range_copy(
    (const MaplibreNativeFfiResourceRange*)self
  );
}

void maplibre_native_ffi_resource_range_to_native(
  const MaplibreNativeFfiResourceRange* self, mln_resource_range* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->start = (uint64_t)(self->start);
  out->end = (uint64_t)(self->end);
}

void maplibre_native_ffi_resource_range_read(
  MaplibreNativeFfiResourceRange* out, const mln_resource_range* in
) {
  out->start = (guint64)(in->start);
  out->end = (guint64)(in->end);
}

MaplibreNativeFfiResourceRange* maplibre_native_ffi_resource_range_from_native(
  const mln_resource_range* in
) {
  MaplibreNativeFfiResourceRange* self =
    g_new0(MaplibreNativeFfiResourceRange, 1);
  maplibre_native_ffi_resource_range_read(self, in);
  return self;
}

struct _MaplibreNativeFfiResourceRequestCancelHandler {
  MaplibreNativeFfiClosure* callback;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiResourceRequestCancelHandler,
  maplibre_native_ffi_resource_request_cancel_handler,
  maplibre_native_ffi_resource_request_cancel_handler_copy,
  maplibre_native_ffi_resource_request_cancel_handler_free
)

MaplibreNativeFfiResourceRequestCancelHandler*
maplibre_native_ffi_resource_request_cancel_handler_new(void) {
  MaplibreNativeFfiResourceRequestCancelHandler* self =
    g_new0(MaplibreNativeFfiResourceRequestCancelHandler, 1);
  return self;
}

MaplibreNativeFfiResourceRequestCancelHandler*
maplibre_native_ffi_resource_request_cancel_handler_copy(
  const MaplibreNativeFfiResourceRequestCancelHandler* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiResourceRequestCancelHandler* copy =
    g_memdup2(self, sizeof *self);
  maplibre_native_ffi_closure_ref(copy->callback);
  return copy;
}

void maplibre_native_ffi_resource_request_cancel_handler_free(
  MaplibreNativeFfiResourceRequestCancelHandler* self
) {
  if (self == NULL) return;
  maplibre_native_ffi_closure_unref(self->callback);
  g_free(self);
}

void maplibre_native_ffi_resource_request_cancel_handler_set_callback(
  MaplibreNativeFfiResourceRequestCancelHandler* self,
  MaplibreNativeFfiResourceRequestCancelCallback callback, gpointer user_data,
  GDestroyNotify notify
) {
  g_return_if_fail(self != NULL);
  MaplibreNativeFfiClosure* previous = self->callback;
  self->callback =
    callback != NULL
      ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify)
      : NULL;
  if (callback == NULL && notify != NULL) notify(user_data);
  maplibre_native_ffi_closure_unref(previous);
}

gpointer maplibre_native_ffi_resource_request_cancel_handler_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_resource_request_cancel_handler_copy(
    (const MaplibreNativeFfiResourceRequestCancelHandler*)self
  );
}

static void resource_request_cancel_handler_callback_trampoline(
  void* user_data, mln_resource_request handle
) {
  (void)handle;
  MaplibreNativeFfiClosure* closure =
    maplibre_native_ffi_registration_closure(user_data, 0);
  if (closure == NULL) return;
  ((MaplibreNativeFfiResourceRequestCancelCallback)(void (*)(void))
     closure->func)(closure->user_data);
}

void maplibre_native_ffi_resource_request_cancel_handler_to_native(
  const MaplibreNativeFfiResourceRequestCancelHandler* self,
  mln_resource_request_cancel_handler* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  if (self->callback != NULL) {
    MaplibreNativeFfiClosure* const closures[] = {self->callback};
    out->user_data =
      maplibre_native_ffi_call_register(call, closures, 1, &out->queue);
    out->release_user_data = maplibre_native_ffi_registration_release;
    out->callback = self->callback != NULL
                      ? resource_request_cancel_handler_callback_trampoline
                      : NULL;
  }
}

MaplibreNativeFfiResourceRequestCancelHandler*
maplibre_native_ffi_resource_request_cancel_handler_from_native(
  const mln_resource_request_cancel_handler* in
) {
  MaplibreNativeFfiResourceRequestCancelHandler* self =
    maplibre_native_ffi_resource_request_cancel_handler_new();
  (void)in;
  return self;
}

struct _MaplibreNativeFfiResourceRequestInfo {
  guint64 fields;
  char* requested_url;
  char* resolved_url;
  MaplibreNativeFfiResourceKind kind;
  MaplibreNativeFfiResourceLoadingMethod loading_method;
  MaplibreNativeFfiResourcePriority priority;
  MaplibreNativeFfiResourceUsage usage;
  MaplibreNativeFfiResourceStoragePolicy storage_policy;
  MaplibreNativeFfiResourceRange range;
  gint64 prior_modified_unix_ms;
  gint64 prior_expires_unix_ms;
  char* prior_etag;
  GBytes* prior_data;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiResourceRequestInfo,
  maplibre_native_ffi_resource_request_info,
  maplibre_native_ffi_resource_request_info_copy,
  maplibre_native_ffi_resource_request_info_free
)

MaplibreNativeFfiResourceRequestInfo*
maplibre_native_ffi_resource_request_info_new(void) {
  MaplibreNativeFfiResourceRequestInfo* self =
    g_new0(MaplibreNativeFfiResourceRequestInfo, 1);
  self->requested_url = g_strdup("");
  self->resolved_url = g_strdup("");
  self->prior_etag = g_strdup("");
  return self;
}

MaplibreNativeFfiResourceRequestInfo*
maplibre_native_ffi_resource_request_info_copy(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiResourceRequestInfo* copy = g_memdup2(self, sizeof *self);
  copy->requested_url = g_strdup(self->requested_url);
  copy->resolved_url = g_strdup(self->resolved_url);
  copy->prior_etag = g_strdup(self->prior_etag);
  copy->prior_data =
    self->prior_data != NULL ? g_bytes_ref(self->prior_data) : NULL;
  return copy;
}

void maplibre_native_ffi_resource_request_info_free(
  MaplibreNativeFfiResourceRequestInfo* self
) {
  if (self == NULL) return;
  g_free(self->requested_url);
  g_free(self->resolved_url);
  g_free(self->prior_etag);
  g_clear_pointer(&self->prior_data, g_bytes_unref);
  g_free(self);
}

const char* maplibre_native_ffi_resource_request_info_get_requested_url(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->requested_url;
}

const char* maplibre_native_ffi_resource_request_info_get_resolved_url(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->resolved_url;
}

MaplibreNativeFfiResourceKind
maplibre_native_ffi_resource_request_info_get_kind(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiResourceKind)0);
  return self->kind;
}

MaplibreNativeFfiResourceLoadingMethod
maplibre_native_ffi_resource_request_info_get_loading_method(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiResourceLoadingMethod)0);
  return self->loading_method;
}

MaplibreNativeFfiResourcePriority
maplibre_native_ffi_resource_request_info_get_priority(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiResourcePriority)0);
  return self->priority;
}

MaplibreNativeFfiResourceUsage
maplibre_native_ffi_resource_request_info_get_usage(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiResourceUsage)0);
  return self->usage;
}

MaplibreNativeFfiResourceStoragePolicy
maplibre_native_ffi_resource_request_info_get_storage_policy(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiResourceStoragePolicy)0);
  return self->storage_policy;
}

MaplibreNativeFfiResourceRange*
maplibre_native_ffi_resource_request_info_get_range(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiResourceRange*)&self->range;
}

gboolean maplibre_native_ffi_resource_request_info_has_range(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_RESOURCE_REQUEST_RANGE) != 0;
}

gint64 maplibre_native_ffi_resource_request_info_get_prior_modified_unix_ms(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->prior_modified_unix_ms;
}

gboolean maplibre_native_ffi_resource_request_info_has_prior_modified_unix_ms(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_RESOURCE_REQUEST_PRIOR_MODIFIED) != 0;
}

gint64 maplibre_native_ffi_resource_request_info_get_prior_expires_unix_ms(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->prior_expires_unix_ms;
}

gboolean maplibre_native_ffi_resource_request_info_has_prior_expires_unix_ms(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_RESOURCE_REQUEST_PRIOR_EXPIRES) != 0;
}

const char* maplibre_native_ffi_resource_request_info_get_prior_etag(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->prior_etag;
}

GBytes* maplibre_native_ffi_resource_request_info_get_prior_data(
  const MaplibreNativeFfiResourceRequestInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->prior_data;
}

gpointer maplibre_native_ffi_resource_request_info_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_resource_request_info_copy(
    (const MaplibreNativeFfiResourceRequestInfo*)self
  );
}

void maplibre_native_ffi_resource_request_info_to_native(
  const MaplibreNativeFfiResourceRequestInfo* self,
  mln_resource_request_info* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_resource_request_fields)self->fields;
  out->requested_url = maplibre_native_ffi_text_view(self->requested_url);
  out->resolved_url = maplibre_native_ffi_text_view(self->resolved_url);
  out->kind = (mln_resource_kind)(self->kind);
  out->loading_method = (mln_resource_loading_method)(self->loading_method);
  out->priority = (mln_resource_priority)(self->priority);
  out->usage = (mln_resource_usage)(self->usage);
  out->storage_policy = (mln_resource_storage_policy)(self->storage_policy);
  maplibre_native_ffi_resource_range_to_native(&self->range, &out->range, call);
  out->prior_modified_unix_ms = (int64_t)(self->prior_modified_unix_ms);
  out->prior_expires_unix_ms = (int64_t)(self->prior_expires_unix_ms);
  out->prior_etag = maplibre_native_ffi_text_view(self->prior_etag);
  {
    mln_buffer_view prior_data_view =
      maplibre_native_ffi_bytes_view(self->prior_data);
    out->prior_data = prior_data_view.data;
    out->prior_data_size = prior_data_view.size;
  }
}

MaplibreNativeFfiResourceRequestInfo*
maplibre_native_ffi_resource_request_info_from_native(
  const mln_resource_request_info* in
) {
  MaplibreNativeFfiResourceRequestInfo* self =
    maplibre_native_ffi_resource_request_info_new();
  self->fields = (guint64)in->fields;
  g_free(self->requested_url);
  self->requested_url = maplibre_native_ffi_text_new(in->requested_url);
  g_free(self->resolved_url);
  self->resolved_url = maplibre_native_ffi_text_new(in->resolved_url);
  self->kind = (MaplibreNativeFfiResourceKind)(in->kind);
  self->loading_method =
    (MaplibreNativeFfiResourceLoadingMethod)(in->loading_method);
  self->priority = (MaplibreNativeFfiResourcePriority)(in->priority);
  self->usage = (MaplibreNativeFfiResourceUsage)(in->usage);
  self->storage_policy =
    (MaplibreNativeFfiResourceStoragePolicy)(in->storage_policy);
  if ((in->fields & MLN_RESOURCE_REQUEST_RANGE) != 0) {
    maplibre_native_ffi_resource_range_read(&self->range, &in->range);
  }
  if ((in->fields & MLN_RESOURCE_REQUEST_PRIOR_MODIFIED) != 0) {
    self->prior_modified_unix_ms = (gint64)(in->prior_modified_unix_ms);
  }
  if ((in->fields & MLN_RESOURCE_REQUEST_PRIOR_EXPIRES) != 0) {
    self->prior_expires_unix_ms = (gint64)(in->prior_expires_unix_ms);
  }
  g_free(self->prior_etag);
  self->prior_etag = maplibre_native_ffi_text_new(in->prior_etag);
  g_clear_pointer(&self->prior_data, g_bytes_unref);
  self->prior_data =
    maplibre_native_ffi_bytes_new(in->prior_data, in->prior_data_size);
  return self;
}

struct _MaplibreNativeFfiResourceResponse {
  guint64 fields;
  MaplibreNativeFfiResourceResponseStatus status;
  MaplibreNativeFfiResourceErrorReason error_reason;
  GBytes* bytes;
  char* error_message;
  gboolean must_revalidate;
  gint64 modified_unix_ms;
  gint64 expires_unix_ms;
  char* etag;
  gint64 retry_after_unix_ms;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiResourceResponse, maplibre_native_ffi_resource_response,
  maplibre_native_ffi_resource_response_copy,
  maplibre_native_ffi_resource_response_free
)

MaplibreNativeFfiResourceResponse* maplibre_native_ffi_resource_response_new(
  void
) {
  MaplibreNativeFfiResourceResponse* self =
    g_new0(MaplibreNativeFfiResourceResponse, 1);
  self->error_message = g_strdup("");
  self->etag = g_strdup("");
  return self;
}

MaplibreNativeFfiResourceResponse* maplibre_native_ffi_resource_response_copy(
  const MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiResourceResponse* copy = g_memdup2(self, sizeof *self);
  copy->bytes = self->bytes != NULL ? g_bytes_ref(self->bytes) : NULL;
  copy->error_message = g_strdup(self->error_message);
  copy->etag = g_strdup(self->etag);
  return copy;
}

void maplibre_native_ffi_resource_response_free(
  MaplibreNativeFfiResourceResponse* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->bytes, g_bytes_unref);
  g_free(self->error_message);
  g_free(self->etag);
  g_free(self);
}

MaplibreNativeFfiResourceResponseStatus
maplibre_native_ffi_resource_response_get_status(
  const MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(
    self != NULL, (MaplibreNativeFfiResourceResponseStatus)0
  );
  return self->status;
}

void maplibre_native_ffi_resource_response_set_status(
  MaplibreNativeFfiResourceResponse* self,
  MaplibreNativeFfiResourceResponseStatus value
) {
  g_return_if_fail(self != NULL);
  self->status = value;
}

MaplibreNativeFfiResourceErrorReason
maplibre_native_ffi_resource_response_get_error_reason(
  const MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiResourceErrorReason)0);
  return self->error_reason;
}

void maplibre_native_ffi_resource_response_set_error_reason(
  MaplibreNativeFfiResourceResponse* self,
  MaplibreNativeFfiResourceErrorReason value
) {
  g_return_if_fail(self != NULL);
  self->error_reason = value;
}

GBytes* maplibre_native_ffi_resource_response_get_bytes(
  const MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->bytes;
}

void maplibre_native_ffi_resource_response_set_bytes(
  MaplibreNativeFfiResourceResponse* self, GBytes* value
) {
  g_return_if_fail(self != NULL);
  GBytes* previous = self->bytes;
  self->bytes = value != NULL ? g_bytes_ref(value) : NULL;
  if (previous != NULL) g_bytes_unref(previous);
}

const char* maplibre_native_ffi_resource_response_get_error_message(
  const MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->error_message;
}

void maplibre_native_ffi_resource_response_set_error_message(
  MaplibreNativeFfiResourceResponse* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->error_message;
  self->error_message = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

gboolean maplibre_native_ffi_resource_response_get_must_revalidate(
  const MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->must_revalidate;
}

void maplibre_native_ffi_resource_response_set_must_revalidate(
  MaplibreNativeFfiResourceResponse* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->must_revalidate = value;
}

gint64 maplibre_native_ffi_resource_response_get_modified_unix_ms(
  const MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->modified_unix_ms;
}

gboolean maplibre_native_ffi_resource_response_has_modified_unix_ms(
  const MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_RESOURCE_RESPONSE_MODIFIED) != 0;
}

void maplibre_native_ffi_resource_response_set_modified_unix_ms(
  MaplibreNativeFfiResourceResponse* self, gint64 value
) {
  g_return_if_fail(self != NULL);
  self->modified_unix_ms = value;
  self->fields |= MLN_RESOURCE_RESPONSE_MODIFIED;
}

gint64 maplibre_native_ffi_resource_response_get_expires_unix_ms(
  const MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->expires_unix_ms;
}

gboolean maplibre_native_ffi_resource_response_has_expires_unix_ms(
  const MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_RESOURCE_RESPONSE_EXPIRES) != 0;
}

void maplibre_native_ffi_resource_response_set_expires_unix_ms(
  MaplibreNativeFfiResourceResponse* self, gint64 value
) {
  g_return_if_fail(self != NULL);
  self->expires_unix_ms = value;
  self->fields |= MLN_RESOURCE_RESPONSE_EXPIRES;
}

const char* maplibre_native_ffi_resource_response_get_etag(
  const MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->etag;
}

void maplibre_native_ffi_resource_response_set_etag(
  MaplibreNativeFfiResourceResponse* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->etag;
  self->etag = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

gint64 maplibre_native_ffi_resource_response_get_retry_after_unix_ms(
  const MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->retry_after_unix_ms;
}

gboolean maplibre_native_ffi_resource_response_has_retry_after_unix_ms(
  const MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_RESOURCE_RESPONSE_RETRY_AFTER) != 0;
}

void maplibre_native_ffi_resource_response_set_retry_after_unix_ms(
  MaplibreNativeFfiResourceResponse* self, gint64 value
) {
  g_return_if_fail(self != NULL);
  self->retry_after_unix_ms = value;
  self->fields |= MLN_RESOURCE_RESPONSE_RETRY_AFTER;
}

gpointer maplibre_native_ffi_resource_response_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_resource_response_copy(
    (const MaplibreNativeFfiResourceResponse*)self
  );
}

void maplibre_native_ffi_resource_response_to_native(
  const MaplibreNativeFfiResourceResponse* self, mln_resource_response* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_resource_response_fields)self->fields;
  out->status = (mln_resource_response_status)(self->status);
  out->error_reason = (mln_resource_error_reason)(self->error_reason);
  {
    mln_buffer_view bytes_view = maplibre_native_ffi_bytes_view(self->bytes);
    out->bytes = bytes_view.data;
    out->byte_count = bytes_view.size;
  }
  out->error_message = maplibre_native_ffi_text_view(self->error_message);
  out->must_revalidate = (self->must_revalidate) != FALSE;
  out->modified_unix_ms = (int64_t)(self->modified_unix_ms);
  out->expires_unix_ms = (int64_t)(self->expires_unix_ms);
  out->etag = maplibre_native_ffi_text_view(self->etag);
  out->retry_after_unix_ms = (int64_t)(self->retry_after_unix_ms);
}

MaplibreNativeFfiResourceResponse*
maplibre_native_ffi_resource_response_from_native(
  const mln_resource_response* in
) {
  MaplibreNativeFfiResourceResponse* self =
    maplibre_native_ffi_resource_response_new();
  self->fields = (guint64)in->fields;
  self->status = (MaplibreNativeFfiResourceResponseStatus)(in->status);
  self->error_reason = (MaplibreNativeFfiResourceErrorReason)(in->error_reason);
  g_clear_pointer(&self->bytes, g_bytes_unref);
  self->bytes = maplibre_native_ffi_bytes_new(in->bytes, in->byte_count);
  g_free(self->error_message);
  self->error_message = maplibre_native_ffi_text_new(in->error_message);
  self->must_revalidate = (in->must_revalidate) ? TRUE : FALSE;
  if ((in->fields & MLN_RESOURCE_RESPONSE_MODIFIED) != 0) {
    self->modified_unix_ms = (gint64)(in->modified_unix_ms);
  }
  if ((in->fields & MLN_RESOURCE_RESPONSE_EXPIRES) != 0) {
    self->expires_unix_ms = (gint64)(in->expires_unix_ms);
  }
  g_free(self->etag);
  self->etag = maplibre_native_ffi_text_new(in->etag);
  if ((in->fields & MLN_RESOURCE_RESPONSE_RETRY_AFTER) != 0) {
    self->retry_after_unix_ms = (gint64)(in->retry_after_unix_ms);
  }
  return self;
}

struct _MaplibreNativeFfiUrlQueryParameter {
  char* name;
  char* value;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiUrlQueryParameter, maplibre_native_ffi_url_query_parameter,
  maplibre_native_ffi_url_query_parameter_copy,
  maplibre_native_ffi_url_query_parameter_free
)

MaplibreNativeFfiUrlQueryParameter* maplibre_native_ffi_url_query_parameter_new(
  void
) {
  MaplibreNativeFfiUrlQueryParameter* self =
    g_new0(MaplibreNativeFfiUrlQueryParameter, 1);
  self->name = g_strdup("");
  self->value = g_strdup("");
  return self;
}

MaplibreNativeFfiUrlQueryParameter*
maplibre_native_ffi_url_query_parameter_copy(
  const MaplibreNativeFfiUrlQueryParameter* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiUrlQueryParameter* copy = g_memdup2(self, sizeof *self);
  copy->name = g_strdup(self->name);
  copy->value = g_strdup(self->value);
  return copy;
}

void maplibre_native_ffi_url_query_parameter_free(
  MaplibreNativeFfiUrlQueryParameter* self
) {
  if (self == NULL) return;
  g_free(self->name);
  g_free(self->value);
  g_free(self);
}

const char* maplibre_native_ffi_url_query_parameter_get_name(
  const MaplibreNativeFfiUrlQueryParameter* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->name;
}

void maplibre_native_ffi_url_query_parameter_set_name(
  MaplibreNativeFfiUrlQueryParameter* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->name;
  self->name = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

const char* maplibre_native_ffi_url_query_parameter_get_value(
  const MaplibreNativeFfiUrlQueryParameter* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->value;
}

void maplibre_native_ffi_url_query_parameter_set_value(
  MaplibreNativeFfiUrlQueryParameter* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->value;
  self->value = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

gpointer maplibre_native_ffi_url_query_parameter_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_url_query_parameter_copy(
    (const MaplibreNativeFfiUrlQueryParameter*)self
  );
}

void maplibre_native_ffi_url_query_parameter_to_native(
  const MaplibreNativeFfiUrlQueryParameter* self, mln_url_query_parameter* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->name = maplibre_native_ffi_text_view(self->name);
  out->value = maplibre_native_ffi_text_view(self->value);
}

MaplibreNativeFfiUrlQueryParameter*
maplibre_native_ffi_url_query_parameter_from_native(
  const mln_url_query_parameter* in
) {
  MaplibreNativeFfiUrlQueryParameter* self =
    maplibre_native_ffi_url_query_parameter_new();
  g_free(self->name);
  self->name = maplibre_native_ffi_text_new(in->name);
  g_free(self->value);
  self->value = maplibre_native_ffi_text_new(in->value);
  return self;
}

struct _MaplibreNativeFfiResourceTransformRule {
  MaplibreNativeFfiResourceKindFlag kinds;
  MaplibreNativeFfiUrlMatchFlag flags;
  char* url;
  GPtrArray* parameters;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiResourceTransformRule,
  maplibre_native_ffi_resource_transform_rule,
  maplibre_native_ffi_resource_transform_rule_copy,
  maplibre_native_ffi_resource_transform_rule_free
)

MaplibreNativeFfiResourceTransformRule*
maplibre_native_ffi_resource_transform_rule_new(void) {
  MaplibreNativeFfiResourceTransformRule* self =
    g_new0(MaplibreNativeFfiResourceTransformRule, 1);
  self->url = g_strdup("");
  self->parameters = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_url_query_parameter_free
  );
  return self;
}

MaplibreNativeFfiResourceTransformRule*
maplibre_native_ffi_resource_transform_rule_copy(
  const MaplibreNativeFfiResourceTransformRule* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiResourceTransformRule* copy = g_memdup2(self, sizeof *self);
  copy->url = g_strdup(self->url);
  copy->parameters =
    self->parameters != NULL
      ? g_ptr_array_copy(
          self->parameters, maplibre_native_ffi_url_query_parameter_copy_func,
          NULL
        )
      : NULL;
  if (copy->parameters != NULL)
    g_ptr_array_set_free_func(
      copy->parameters,
      (GDestroyNotify)maplibre_native_ffi_url_query_parameter_free
    );
  return copy;
}

void maplibre_native_ffi_resource_transform_rule_free(
  MaplibreNativeFfiResourceTransformRule* self
) {
  if (self == NULL) return;
  g_free(self->url);
  g_clear_pointer(&self->parameters, g_ptr_array_unref);
  g_free(self);
}

MaplibreNativeFfiResourceKindFlag
maplibre_native_ffi_resource_transform_rule_get_kinds(
  const MaplibreNativeFfiResourceTransformRule* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiResourceKindFlag)0);
  return self->kinds;
}

void maplibre_native_ffi_resource_transform_rule_set_kinds(
  MaplibreNativeFfiResourceTransformRule* self,
  MaplibreNativeFfiResourceKindFlag value
) {
  g_return_if_fail(self != NULL);
  self->kinds = value;
}

MaplibreNativeFfiUrlMatchFlag
maplibre_native_ffi_resource_transform_rule_get_flags(
  const MaplibreNativeFfiResourceTransformRule* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiUrlMatchFlag)0);
  return self->flags;
}

void maplibre_native_ffi_resource_transform_rule_set_flags(
  MaplibreNativeFfiResourceTransformRule* self,
  MaplibreNativeFfiUrlMatchFlag value
) {
  g_return_if_fail(self != NULL);
  self->flags = value;
}

const char* maplibre_native_ffi_resource_transform_rule_get_url(
  const MaplibreNativeFfiResourceTransformRule* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->url;
}

void maplibre_native_ffi_resource_transform_rule_set_url(
  MaplibreNativeFfiResourceTransformRule* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->url;
  self->url = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

GPtrArray* maplibre_native_ffi_resource_transform_rule_get_parameters(
  const MaplibreNativeFfiResourceTransformRule* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->parameters;
}

void maplibre_native_ffi_resource_transform_rule_set_parameters(
  MaplibreNativeFfiResourceTransformRule* self,
  MaplibreNativeFfiUrlQueryParameter* const* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->parameters;
  self->parameters = g_ptr_array_new_full(
    (guint)value_count,
    (GDestroyNotify)maplibre_native_ffi_url_query_parameter_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->parameters,
      maplibre_native_ffi_url_query_parameter_copy(value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

gpointer maplibre_native_ffi_resource_transform_rule_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_resource_transform_rule_copy(
    (const MaplibreNativeFfiResourceTransformRule*)self
  );
}

void maplibre_native_ffi_resource_transform_rule_to_native(
  const MaplibreNativeFfiResourceTransformRule* self,
  mln_resource_transform_rule* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->kinds = (mln_resource_kind_flags)(self->kinds);
  out->flags = (mln_url_match_flags)(self->flags);
  out->url = maplibre_native_ffi_text_view(self->url);
  if (self->parameters != NULL && self->parameters->len > 0) {
    mln_url_query_parameter* items = maplibre_native_ffi_call_alloc(
      call, self->parameters->len, sizeof *items
    );
    for (guint index = 0; index < self->parameters->len; index++) {
      maplibre_native_ffi_url_query_parameter_to_native(
        self->parameters->pdata[index], &items[index], call
      );
    }
    out->parameters = items;
    out->parameter_count = self->parameters->len;
  }
}

MaplibreNativeFfiResourceTransformRule*
maplibre_native_ffi_resource_transform_rule_from_native(
  const mln_resource_transform_rule* in
) {
  MaplibreNativeFfiResourceTransformRule* self =
    maplibre_native_ffi_resource_transform_rule_new();
  self->kinds = (MaplibreNativeFfiResourceKindFlag)(in->kinds);
  self->flags = (MaplibreNativeFfiUrlMatchFlag)(in->flags);
  g_free(self->url);
  self->url = maplibre_native_ffi_text_new(in->url);
  g_clear_pointer(&self->parameters, g_ptr_array_unref);
  self->parameters = g_ptr_array_new_full(
    (guint)in->parameter_count,
    (GDestroyNotify)maplibre_native_ffi_url_query_parameter_free
  );
  for (size_t index = 0; index < in->parameter_count; index++) {
    g_ptr_array_add(
      self->parameters, maplibre_native_ffi_url_query_parameter_from_native(
                          &in->parameters[index]
                        )
    );
  }
  return self;
}

struct _MaplibreNativeFfiResourceTransform {
  GPtrArray* rules;
  MaplibreNativeFfiClosure* callback;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiResourceTransform, maplibre_native_ffi_resource_transform,
  maplibre_native_ffi_resource_transform_copy,
  maplibre_native_ffi_resource_transform_free
)

MaplibreNativeFfiResourceTransform* maplibre_native_ffi_resource_transform_new(
  void
) {
  MaplibreNativeFfiResourceTransform* self =
    g_new0(MaplibreNativeFfiResourceTransform, 1);
  self->rules = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_resource_transform_rule_free
  );
  return self;
}

MaplibreNativeFfiResourceTransform* maplibre_native_ffi_resource_transform_copy(
  const MaplibreNativeFfiResourceTransform* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiResourceTransform* copy = g_memdup2(self, sizeof *self);
  copy->rules =
    self->rules != NULL
      ? g_ptr_array_copy(
          self->rules, maplibre_native_ffi_resource_transform_rule_copy_func,
          NULL
        )
      : NULL;
  if (copy->rules != NULL)
    g_ptr_array_set_free_func(
      copy->rules,
      (GDestroyNotify)maplibre_native_ffi_resource_transform_rule_free
    );
  maplibre_native_ffi_closure_ref(copy->callback);
  return copy;
}

void maplibre_native_ffi_resource_transform_free(
  MaplibreNativeFfiResourceTransform* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->rules, g_ptr_array_unref);
  maplibre_native_ffi_closure_unref(self->callback);
  g_free(self);
}

GPtrArray* maplibre_native_ffi_resource_transform_get_rules(
  const MaplibreNativeFfiResourceTransform* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->rules;
}

void maplibre_native_ffi_resource_transform_set_rules(
  MaplibreNativeFfiResourceTransform* self,
  MaplibreNativeFfiResourceTransformRule* const* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->rules;
  self->rules = g_ptr_array_new_full(
    (guint)value_count,
    (GDestroyNotify)maplibre_native_ffi_resource_transform_rule_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->rules,
      maplibre_native_ffi_resource_transform_rule_copy(value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

void maplibre_native_ffi_resource_transform_set_callback(
  MaplibreNativeFfiResourceTransform* self,
  MaplibreNativeFfiResourceTransformCallback callback, gpointer user_data,
  GDestroyNotify notify
) {
  g_return_if_fail(self != NULL);
  MaplibreNativeFfiClosure* previous = self->callback;
  self->callback =
    callback != NULL
      ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify)
      : NULL;
  if (callback == NULL && notify != NULL) notify(user_data);
  maplibre_native_ffi_closure_unref(previous);
}

gpointer maplibre_native_ffi_resource_transform_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_resource_transform_copy(
    (const MaplibreNativeFfiResourceTransform*)self
  );
}

static void resource_transform_callback_trampoline(
  void* user_data, mln_resource_kind kind, mln_buffer_view url,
  mln_resource_transform_request request
) {
  MaplibreNativeFfiClosure* closure =
    maplibre_native_ffi_registration_closure(user_data, 0);
  if (closure == NULL) return;
  char* url_ = maplibre_native_ffi_text_new(url);
  MaplibreNativeFfiResourceTransformRequest* request_ =
    maplibre_native_ffi_resource_transform_request_wrap(
      request, NULL, FALSE, NULL
    );
  ((MaplibreNativeFfiResourceTransformCallback)(void (*)(void))closure->func)(
    (MaplibreNativeFfiResourceKind)(kind), url_, request_, closure->user_data
  );
  g_free(url_);
  g_object_unref(request_);
}

void maplibre_native_ffi_resource_transform_to_native(
  const MaplibreNativeFfiResourceTransform* self, mln_resource_transform* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  if (self->rules != NULL && self->rules->len > 0) {
    mln_resource_transform_rule* items =
      maplibre_native_ffi_call_alloc(call, self->rules->len, sizeof *items);
    for (guint index = 0; index < self->rules->len; index++) {
      maplibre_native_ffi_resource_transform_rule_to_native(
        self->rules->pdata[index], &items[index], call
      );
    }
    out->rules = items;
    out->rule_count = self->rules->len;
  }
  if (self->callback != NULL) {
    MaplibreNativeFfiClosure* const closures[] = {self->callback};
    out->user_data =
      maplibre_native_ffi_call_register(call, closures, 1, &out->queue);
    out->release_user_data = maplibre_native_ffi_registration_release;
    out->callback =
      self->callback != NULL ? resource_transform_callback_trampoline : NULL;
  }
}

MaplibreNativeFfiResourceTransform*
maplibre_native_ffi_resource_transform_from_native(
  const mln_resource_transform* in
) {
  MaplibreNativeFfiResourceTransform* self =
    maplibre_native_ffi_resource_transform_new();
  g_clear_pointer(&self->rules, g_ptr_array_unref);
  self->rules = g_ptr_array_new_full(
    (guint)in->rule_count,
    (GDestroyNotify)maplibre_native_ffi_resource_transform_rule_free
  );
  for (size_t index = 0; index < in->rule_count; index++) {
    g_ptr_array_add(
      self->rules,
      maplibre_native_ffi_resource_transform_rule_from_native(&in->rules[index])
    );
  }
  return self;
}

struct _MaplibreNativeFfiResourceTransformError {
  guint64 fields;
  MaplibreNativeFfiResourceErrorReason reason;
  char* message;
  gint64 retry_after_unix_ms;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiResourceTransformError,
  maplibre_native_ffi_resource_transform_error,
  maplibre_native_ffi_resource_transform_error_copy,
  maplibre_native_ffi_resource_transform_error_free
)

MaplibreNativeFfiResourceTransformError*
maplibre_native_ffi_resource_transform_error_new(void) {
  MaplibreNativeFfiResourceTransformError* self =
    g_new0(MaplibreNativeFfiResourceTransformError, 1);
  self->message = g_strdup("");
  return self;
}

MaplibreNativeFfiResourceTransformError*
maplibre_native_ffi_resource_transform_error_copy(
  const MaplibreNativeFfiResourceTransformError* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiResourceTransformError* copy = g_memdup2(self, sizeof *self);
  copy->message = g_strdup(self->message);
  return copy;
}

void maplibre_native_ffi_resource_transform_error_free(
  MaplibreNativeFfiResourceTransformError* self
) {
  if (self == NULL) return;
  g_free(self->message);
  g_free(self);
}

MaplibreNativeFfiResourceErrorReason
maplibre_native_ffi_resource_transform_error_get_reason(
  const MaplibreNativeFfiResourceTransformError* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiResourceErrorReason)0);
  return self->reason;
}

void maplibre_native_ffi_resource_transform_error_set_reason(
  MaplibreNativeFfiResourceTransformError* self,
  MaplibreNativeFfiResourceErrorReason value
) {
  g_return_if_fail(self != NULL);
  self->reason = value;
}

const char* maplibre_native_ffi_resource_transform_error_get_message(
  const MaplibreNativeFfiResourceTransformError* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->message;
}

void maplibre_native_ffi_resource_transform_error_set_message(
  MaplibreNativeFfiResourceTransformError* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->message;
  self->message = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

gint64 maplibre_native_ffi_resource_transform_error_get_retry_after_unix_ms(
  const MaplibreNativeFfiResourceTransformError* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->retry_after_unix_ms;
}

gboolean maplibre_native_ffi_resource_transform_error_has_retry_after_unix_ms(
  const MaplibreNativeFfiResourceTransformError* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER) != 0;
}

void maplibre_native_ffi_resource_transform_error_set_retry_after_unix_ms(
  MaplibreNativeFfiResourceTransformError* self, gint64 value
) {
  g_return_if_fail(self != NULL);
  self->retry_after_unix_ms = value;
  self->fields |= MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER;
}

gpointer maplibre_native_ffi_resource_transform_error_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_resource_transform_error_copy(
    (const MaplibreNativeFfiResourceTransformError*)self
  );
}

void maplibre_native_ffi_resource_transform_error_to_native(
  const MaplibreNativeFfiResourceTransformError* self,
  mln_resource_transform_error* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_resource_transform_error_fields)self->fields;
  out->reason = (mln_resource_error_reason)(self->reason);
  out->message = maplibre_native_ffi_text_view(self->message);
  out->retry_after_unix_ms = (int64_t)(self->retry_after_unix_ms);
}

MaplibreNativeFfiResourceTransformError*
maplibre_native_ffi_resource_transform_error_from_native(
  const mln_resource_transform_error* in
) {
  MaplibreNativeFfiResourceTransformError* self =
    maplibre_native_ffi_resource_transform_error_new();
  self->fields = (guint64)in->fields;
  self->reason = (MaplibreNativeFfiResourceErrorReason)(in->reason);
  g_free(self->message);
  self->message = maplibre_native_ffi_text_new(in->message);
  if ((in->fields & MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER) != 0) {
    self->retry_after_unix_ms = (gint64)(in->retry_after_unix_ms);
  }
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEventCameraChange,
  maplibre_native_ffi_runtime_event_camera_change,
  maplibre_native_ffi_runtime_event_camera_change_copy,
  maplibre_native_ffi_runtime_event_camera_change_free
)

MaplibreNativeFfiRuntimeEventCameraChange*
maplibre_native_ffi_runtime_event_camera_change_copy(
  const MaplibreNativeFfiRuntimeEventCameraChange* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_runtime_event_camera_change_free(
  MaplibreNativeFfiRuntimeEventCameraChange* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_runtime_event_camera_change_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_camera_change_copy(
    (const MaplibreNativeFfiRuntimeEventCameraChange*)self
  );
}

void maplibre_native_ffi_runtime_event_camera_change_to_native(
  const MaplibreNativeFfiRuntimeEventCameraChange* self,
  mln_runtime_event_camera_change* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->mode = (mln_camera_change_mode)(self->mode);
}

void maplibre_native_ffi_runtime_event_camera_change_read(
  MaplibreNativeFfiRuntimeEventCameraChange* out,
  const mln_runtime_event_camera_change* in
) {
  out->mode = (MaplibreNativeFfiCameraChangeMode)(in->mode);
}

MaplibreNativeFfiRuntimeEventCameraChange*
maplibre_native_ffi_runtime_event_camera_change_from_native(
  const mln_runtime_event_camera_change* in
) {
  MaplibreNativeFfiRuntimeEventCameraChange* self =
    g_new0(MaplibreNativeFfiRuntimeEventCameraChange, 1);
  maplibre_native_ffi_runtime_event_camera_change_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEventLoadingFailed,
  maplibre_native_ffi_runtime_event_loading_failed,
  maplibre_native_ffi_runtime_event_loading_failed_copy,
  maplibre_native_ffi_runtime_event_loading_failed_free
)

MaplibreNativeFfiRuntimeEventLoadingFailed*
maplibre_native_ffi_runtime_event_loading_failed_copy(
  const MaplibreNativeFfiRuntimeEventLoadingFailed* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_runtime_event_loading_failed_free(
  MaplibreNativeFfiRuntimeEventLoadingFailed* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_runtime_event_loading_failed_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_loading_failed_copy(
    (const MaplibreNativeFfiRuntimeEventLoadingFailed*)self
  );
}

void maplibre_native_ffi_runtime_event_loading_failed_to_native(
  const MaplibreNativeFfiRuntimeEventLoadingFailed* self,
  mln_runtime_event_loading_failed* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->error = (mln_map_load_error)(self->error);
}

void maplibre_native_ffi_runtime_event_loading_failed_read(
  MaplibreNativeFfiRuntimeEventLoadingFailed* out,
  const mln_runtime_event_loading_failed* in
) {
  out->error = (MaplibreNativeFfiMapLoadError)(in->error);
}

MaplibreNativeFfiRuntimeEventLoadingFailed*
maplibre_native_ffi_runtime_event_loading_failed_from_native(
  const mln_runtime_event_loading_failed* in
) {
  MaplibreNativeFfiRuntimeEventLoadingFailed* self =
    g_new0(MaplibreNativeFfiRuntimeEventLoadingFailed, 1);
  maplibre_native_ffi_runtime_event_loading_failed_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEventRenderFrame,
  maplibre_native_ffi_runtime_event_render_frame,
  maplibre_native_ffi_runtime_event_render_frame_copy,
  maplibre_native_ffi_runtime_event_render_frame_free
)

MaplibreNativeFfiRuntimeEventRenderFrame*
maplibre_native_ffi_runtime_event_render_frame_copy(
  const MaplibreNativeFfiRuntimeEventRenderFrame* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_runtime_event_render_frame_free(
  MaplibreNativeFfiRuntimeEventRenderFrame* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_runtime_event_render_frame_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_render_frame_copy(
    (const MaplibreNativeFfiRuntimeEventRenderFrame*)self
  );
}

void maplibre_native_ffi_runtime_event_render_frame_to_native(
  const MaplibreNativeFfiRuntimeEventRenderFrame* self,
  mln_runtime_event_render_frame* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->mode = (mln_render_mode)(self->mode);
  out->needs_repaint = (self->needs_repaint) != FALSE;
  out->placement_changed = (self->placement_changed) != FALSE;
  maplibre_native_ffi_rendering_stats_to_native(
    &self->stats, &out->stats, call
  );
}

void maplibre_native_ffi_runtime_event_render_frame_read(
  MaplibreNativeFfiRuntimeEventRenderFrame* out,
  const mln_runtime_event_render_frame* in
) {
  out->mode = (MaplibreNativeFfiRenderMode)(in->mode);
  out->needs_repaint = (in->needs_repaint) ? TRUE : FALSE;
  out->placement_changed = (in->placement_changed) ? TRUE : FALSE;
  maplibre_native_ffi_rendering_stats_read(&out->stats, &in->stats);
}

MaplibreNativeFfiRuntimeEventRenderFrame*
maplibre_native_ffi_runtime_event_render_frame_from_native(
  const mln_runtime_event_render_frame* in
) {
  MaplibreNativeFfiRuntimeEventRenderFrame* self =
    g_new0(MaplibreNativeFfiRuntimeEventRenderFrame, 1);
  maplibre_native_ffi_runtime_event_render_frame_read(self, in);
  return self;
}

struct _MaplibreNativeFfiRuntimeEventStyleImage {
  char* image_id;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEventStyleImage,
  maplibre_native_ffi_runtime_event_style_image,
  maplibre_native_ffi_runtime_event_style_image_copy,
  maplibre_native_ffi_runtime_event_style_image_free
)

MaplibreNativeFfiRuntimeEventStyleImage*
maplibre_native_ffi_runtime_event_style_image_new(void) {
  MaplibreNativeFfiRuntimeEventStyleImage* self =
    g_new0(MaplibreNativeFfiRuntimeEventStyleImage, 1);
  self->image_id = g_strdup("");
  return self;
}

MaplibreNativeFfiRuntimeEventStyleImage*
maplibre_native_ffi_runtime_event_style_image_copy(
  const MaplibreNativeFfiRuntimeEventStyleImage* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiRuntimeEventStyleImage* copy = g_memdup2(self, sizeof *self);
  copy->image_id = g_strdup(self->image_id);
  return copy;
}

void maplibre_native_ffi_runtime_event_style_image_free(
  MaplibreNativeFfiRuntimeEventStyleImage* self
) {
  if (self == NULL) return;
  g_free(self->image_id);
  g_free(self);
}

const char* maplibre_native_ffi_runtime_event_style_image_get_image_id(
  const MaplibreNativeFfiRuntimeEventStyleImage* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->image_id;
}

gpointer maplibre_native_ffi_runtime_event_style_image_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_style_image_copy(
    (const MaplibreNativeFfiRuntimeEventStyleImage*)self
  );
}

void maplibre_native_ffi_runtime_event_style_image_to_native(
  const MaplibreNativeFfiRuntimeEventStyleImage* self,
  mln_runtime_event_style_image* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->image_id = maplibre_native_ffi_text_view(self->image_id);
}

MaplibreNativeFfiRuntimeEventStyleImage*
maplibre_native_ffi_runtime_event_style_image_from_native(
  const mln_runtime_event_style_image* in
) {
  MaplibreNativeFfiRuntimeEventStyleImage* self =
    maplibre_native_ffi_runtime_event_style_image_new();
  g_free(self->image_id);
  self->image_id = maplibre_native_ffi_text_new(in->image_id);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiTileId, maplibre_native_ffi_tile_id,
  maplibre_native_ffi_tile_id_copy, maplibre_native_ffi_tile_id_free
)

MaplibreNativeFfiTileId* maplibre_native_ffi_tile_id_copy(
  const MaplibreNativeFfiTileId* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_tile_id_free(MaplibreNativeFfiTileId* self) {
  g_free(self);
}

gpointer maplibre_native_ffi_tile_id_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_tile_id_copy((const MaplibreNativeFfiTileId*)self);
}

void maplibre_native_ffi_tile_id_to_native(
  const MaplibreNativeFfiTileId* self, mln_tile_id* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->overscaled_z = (uint32_t)(self->overscaled_z);
  out->wrap = (int32_t)(self->wrap);
  out->canonical_z = (uint32_t)(self->canonical_z);
  out->canonical_x = (uint32_t)(self->canonical_x);
  out->canonical_y = (uint32_t)(self->canonical_y);
}

void maplibre_native_ffi_tile_id_read(
  MaplibreNativeFfiTileId* out, const mln_tile_id* in
) {
  out->overscaled_z = (guint32)(in->overscaled_z);
  out->wrap = (gint32)(in->wrap);
  out->canonical_z = (guint32)(in->canonical_z);
  out->canonical_x = (guint32)(in->canonical_x);
  out->canonical_y = (guint32)(in->canonical_y);
}

MaplibreNativeFfiTileId* maplibre_native_ffi_tile_id_from_native(
  const mln_tile_id* in
) {
  MaplibreNativeFfiTileId* self = g_new0(MaplibreNativeFfiTileId, 1);
  maplibre_native_ffi_tile_id_read(self, in);
  return self;
}

struct _MaplibreNativeFfiRuntimeEventTileAction {
  char* source_id;
  MaplibreNativeFfiTileOperation operation;
  MaplibreNativeFfiTileId tile_id;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEventTileAction,
  maplibre_native_ffi_runtime_event_tile_action,
  maplibre_native_ffi_runtime_event_tile_action_copy,
  maplibre_native_ffi_runtime_event_tile_action_free
)

MaplibreNativeFfiRuntimeEventTileAction*
maplibre_native_ffi_runtime_event_tile_action_new(void) {
  MaplibreNativeFfiRuntimeEventTileAction* self =
    g_new0(MaplibreNativeFfiRuntimeEventTileAction, 1);
  self->source_id = g_strdup("");
  return self;
}

MaplibreNativeFfiRuntimeEventTileAction*
maplibre_native_ffi_runtime_event_tile_action_copy(
  const MaplibreNativeFfiRuntimeEventTileAction* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiRuntimeEventTileAction* copy = g_memdup2(self, sizeof *self);
  copy->source_id = g_strdup(self->source_id);
  return copy;
}

void maplibre_native_ffi_runtime_event_tile_action_free(
  MaplibreNativeFfiRuntimeEventTileAction* self
) {
  if (self == NULL) return;
  g_free(self->source_id);
  g_free(self);
}

const char* maplibre_native_ffi_runtime_event_tile_action_get_source_id(
  const MaplibreNativeFfiRuntimeEventTileAction* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->source_id;
}

MaplibreNativeFfiTileOperation
maplibre_native_ffi_runtime_event_tile_action_get_operation(
  const MaplibreNativeFfiRuntimeEventTileAction* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiTileOperation)0);
  return self->operation;
}

MaplibreNativeFfiTileId*
maplibre_native_ffi_runtime_event_tile_action_get_tile_id(
  const MaplibreNativeFfiRuntimeEventTileAction* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiTileId*)&self->tile_id;
}

gpointer maplibre_native_ffi_runtime_event_tile_action_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_tile_action_copy(
    (const MaplibreNativeFfiRuntimeEventTileAction*)self
  );
}

void maplibre_native_ffi_runtime_event_tile_action_to_native(
  const MaplibreNativeFfiRuntimeEventTileAction* self,
  mln_runtime_event_tile_action* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->source_id = maplibre_native_ffi_text_view(self->source_id);
  out->operation = (mln_tile_operation)(self->operation);
  maplibre_native_ffi_tile_id_to_native(&self->tile_id, &out->tile_id, call);
}

MaplibreNativeFfiRuntimeEventTileAction*
maplibre_native_ffi_runtime_event_tile_action_from_native(
  const mln_runtime_event_tile_action* in
) {
  MaplibreNativeFfiRuntimeEventTileAction* self =
    maplibre_native_ffi_runtime_event_tile_action_new();
  g_free(self->source_id);
  self->source_id = maplibre_native_ffi_text_new(in->source_id);
  self->operation = (MaplibreNativeFfiTileOperation)(in->operation);
  maplibre_native_ffi_tile_id_read(&self->tile_id, &in->tile_id);
  return self;
}

struct _MaplibreNativeFfiRuntimeEventSourceChanged {
  char* source_id;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEventSourceChanged,
  maplibre_native_ffi_runtime_event_source_changed,
  maplibre_native_ffi_runtime_event_source_changed_copy,
  maplibre_native_ffi_runtime_event_source_changed_free
)

MaplibreNativeFfiRuntimeEventSourceChanged*
maplibre_native_ffi_runtime_event_source_changed_new(void) {
  MaplibreNativeFfiRuntimeEventSourceChanged* self =
    g_new0(MaplibreNativeFfiRuntimeEventSourceChanged, 1);
  self->source_id = g_strdup("");
  return self;
}

MaplibreNativeFfiRuntimeEventSourceChanged*
maplibre_native_ffi_runtime_event_source_changed_copy(
  const MaplibreNativeFfiRuntimeEventSourceChanged* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiRuntimeEventSourceChanged* copy =
    g_memdup2(self, sizeof *self);
  copy->source_id = g_strdup(self->source_id);
  return copy;
}

void maplibre_native_ffi_runtime_event_source_changed_free(
  MaplibreNativeFfiRuntimeEventSourceChanged* self
) {
  if (self == NULL) return;
  g_free(self->source_id);
  g_free(self);
}

const char* maplibre_native_ffi_runtime_event_source_changed_get_source_id(
  const MaplibreNativeFfiRuntimeEventSourceChanged* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->source_id;
}

gpointer maplibre_native_ffi_runtime_event_source_changed_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_source_changed_copy(
    (const MaplibreNativeFfiRuntimeEventSourceChanged*)self
  );
}

void maplibre_native_ffi_runtime_event_source_changed_to_native(
  const MaplibreNativeFfiRuntimeEventSourceChanged* self,
  mln_runtime_event_source_changed* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->source_id = maplibre_native_ffi_text_view(self->source_id);
}

MaplibreNativeFfiRuntimeEventSourceChanged*
maplibre_native_ffi_runtime_event_source_changed_from_native(
  const mln_runtime_event_source_changed* in
) {
  MaplibreNativeFfiRuntimeEventSourceChanged* self =
    maplibre_native_ffi_runtime_event_source_changed_new();
  g_free(self->source_id);
  self->source_id = maplibre_native_ffi_text_new(in->source_id);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEventOfflineRegionStatus,
  maplibre_native_ffi_runtime_event_offline_region_status,
  maplibre_native_ffi_runtime_event_offline_region_status_copy,
  maplibre_native_ffi_runtime_event_offline_region_status_free
)

MaplibreNativeFfiRuntimeEventOfflineRegionStatus*
maplibre_native_ffi_runtime_event_offline_region_status_copy(
  const MaplibreNativeFfiRuntimeEventOfflineRegionStatus* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_runtime_event_offline_region_status_free(
  MaplibreNativeFfiRuntimeEventOfflineRegionStatus* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_runtime_event_offline_region_status_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_offline_region_status_copy(
    (const MaplibreNativeFfiRuntimeEventOfflineRegionStatus*)self
  );
}

void maplibre_native_ffi_runtime_event_offline_region_status_to_native(
  const MaplibreNativeFfiRuntimeEventOfflineRegionStatus* self,
  mln_runtime_event_offline_region_status* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->region_id = (mln_offline_region_id)(self->region_id);
  maplibre_native_ffi_offline_region_status_to_native(
    &self->status, &out->status, call
  );
}

void maplibre_native_ffi_runtime_event_offline_region_status_read(
  MaplibreNativeFfiRuntimeEventOfflineRegionStatus* out,
  const mln_runtime_event_offline_region_status* in
) {
  out->region_id = (gint64)(in->region_id);
  maplibre_native_ffi_offline_region_status_read(&out->status, &in->status);
}

MaplibreNativeFfiRuntimeEventOfflineRegionStatus*
maplibre_native_ffi_runtime_event_offline_region_status_from_native(
  const mln_runtime_event_offline_region_status* in
) {
  MaplibreNativeFfiRuntimeEventOfflineRegionStatus* self =
    g_new0(MaplibreNativeFfiRuntimeEventOfflineRegionStatus, 1);
  maplibre_native_ffi_runtime_event_offline_region_status_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEventOfflineRegionResponseError,
  maplibre_native_ffi_runtime_event_offline_region_response_error,
  maplibre_native_ffi_runtime_event_offline_region_response_error_copy,
  maplibre_native_ffi_runtime_event_offline_region_response_error_free
)

MaplibreNativeFfiRuntimeEventOfflineRegionResponseError*
maplibre_native_ffi_runtime_event_offline_region_response_error_copy(
  const MaplibreNativeFfiRuntimeEventOfflineRegionResponseError* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_runtime_event_offline_region_response_error_free(
  MaplibreNativeFfiRuntimeEventOfflineRegionResponseError* self
) {
  g_free(self);
}

gpointer
maplibre_native_ffi_runtime_event_offline_region_response_error_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_offline_region_response_error_copy(
    (const MaplibreNativeFfiRuntimeEventOfflineRegionResponseError*)self
  );
}

void maplibre_native_ffi_runtime_event_offline_region_response_error_to_native(
  const MaplibreNativeFfiRuntimeEventOfflineRegionResponseError* self,
  mln_runtime_event_offline_region_response_error* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->region_id = (mln_offline_region_id)(self->region_id);
  out->reason = (mln_resource_error_reason)(self->reason);
}

void maplibre_native_ffi_runtime_event_offline_region_response_error_read(
  MaplibreNativeFfiRuntimeEventOfflineRegionResponseError* out,
  const mln_runtime_event_offline_region_response_error* in
) {
  out->region_id = (gint64)(in->region_id);
  out->reason = (MaplibreNativeFfiResourceErrorReason)(in->reason);
}

MaplibreNativeFfiRuntimeEventOfflineRegionResponseError*
maplibre_native_ffi_runtime_event_offline_region_response_error_from_native(
  const mln_runtime_event_offline_region_response_error* in
) {
  MaplibreNativeFfiRuntimeEventOfflineRegionResponseError* self =
    g_new0(MaplibreNativeFfiRuntimeEventOfflineRegionResponseError, 1);
  maplibre_native_ffi_runtime_event_offline_region_response_error_read(
    self, in
  );
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit,
  maplibre_native_ffi_runtime_event_offline_region_tile_count_limit,
  maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_copy,
  maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_free
)

MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit*
maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_copy(
  const MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_free(
  MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit* self
) {
  g_free(self);
}

gpointer
maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_copy(
    (const MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit*)self
  );
}

void maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_to_native(
  const MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit* self,
  mln_runtime_event_offline_region_tile_count_limit* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->region_id = (mln_offline_region_id)(self->region_id);
  out->limit = (uint64_t)(self->limit);
}

void maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_read(
  MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit* out,
  const mln_runtime_event_offline_region_tile_count_limit* in
) {
  out->region_id = (gint64)(in->region_id);
  out->limit = (guint64)(in->limit);
}

MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit*
maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_from_native(
  const mln_runtime_event_offline_region_tile_count_limit* in
) {
  MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit* self =
    g_new0(MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit, 1);
  maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_read(
    self, in
  );
  return self;
}

struct _MaplibreNativeFfiRuntimeEventGlyphs {
  char** font_stack;
  guint32 first;
  guint32 last;
  gboolean font_face;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEventGlyphs, maplibre_native_ffi_runtime_event_glyphs,
  maplibre_native_ffi_runtime_event_glyphs_copy,
  maplibre_native_ffi_runtime_event_glyphs_free
)

MaplibreNativeFfiRuntimeEventGlyphs*
maplibre_native_ffi_runtime_event_glyphs_new(void) {
  MaplibreNativeFfiRuntimeEventGlyphs* self =
    g_new0(MaplibreNativeFfiRuntimeEventGlyphs, 1);
  self->font_stack = g_new0(char*, 1);
  return self;
}

MaplibreNativeFfiRuntimeEventGlyphs*
maplibre_native_ffi_runtime_event_glyphs_copy(
  const MaplibreNativeFfiRuntimeEventGlyphs* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiRuntimeEventGlyphs* copy = g_memdup2(self, sizeof *self);
  copy->font_stack = g_strdupv(self->font_stack);
  return copy;
}

void maplibre_native_ffi_runtime_event_glyphs_free(
  MaplibreNativeFfiRuntimeEventGlyphs* self
) {
  if (self == NULL) return;
  g_strfreev(self->font_stack);
  g_free(self);
}

const char* const* maplibre_native_ffi_runtime_event_glyphs_get_font_stack(
  const MaplibreNativeFfiRuntimeEventGlyphs* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (const char* const*)self->font_stack;
}

guint32 maplibre_native_ffi_runtime_event_glyphs_get_first(
  const MaplibreNativeFfiRuntimeEventGlyphs* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->first;
}

guint32 maplibre_native_ffi_runtime_event_glyphs_get_last(
  const MaplibreNativeFfiRuntimeEventGlyphs* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->last;
}

gboolean maplibre_native_ffi_runtime_event_glyphs_get_font_face(
  const MaplibreNativeFfiRuntimeEventGlyphs* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->font_face;
}

gpointer maplibre_native_ffi_runtime_event_glyphs_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_glyphs_copy(
    (const MaplibreNativeFfiRuntimeEventGlyphs*)self
  );
}

void maplibre_native_ffi_runtime_event_glyphs_to_native(
  const MaplibreNativeFfiRuntimeEventGlyphs* self,
  mln_runtime_event_glyphs* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  if (self->font_stack != NULL && self->font_stack[0] != NULL) {
    guint count = g_strv_length(self->font_stack);
    mln_buffer_view* items =
      maplibre_native_ffi_call_alloc(call, count, sizeof *items);
    for (guint index = 0; index < count; index++) {
      items[index] = maplibre_native_ffi_text_view(self->font_stack[index]);
    }
    out->font_stack = items;
    out->font_count = count;
  }
  out->first = (uint32_t)(self->first);
  out->last = (uint32_t)(self->last);
  out->font_face = (self->font_face) != FALSE;
}

MaplibreNativeFfiRuntimeEventGlyphs*
maplibre_native_ffi_runtime_event_glyphs_from_native(
  const mln_runtime_event_glyphs* in
) {
  MaplibreNativeFfiRuntimeEventGlyphs* self =
    maplibre_native_ffi_runtime_event_glyphs_new();
  g_strfreev(self->font_stack);
  self->font_stack = g_new0(char*, in->font_count + 1);
  for (size_t index = 0; index < in->font_count; index++) {
    self->font_stack[index] =
      maplibre_native_ffi_text_new(in->font_stack[index]);
  }
  self->first = (guint32)(in->first);
  self->last = (guint32)(in->last);
  self->font_face = (in->font_face) ? TRUE : FALSE;
  return self;
}

struct _MaplibreNativeFfiRuntimeEventSprite {
  char* id;
  char* url;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEventSprite, maplibre_native_ffi_runtime_event_sprite,
  maplibre_native_ffi_runtime_event_sprite_copy,
  maplibre_native_ffi_runtime_event_sprite_free
)

MaplibreNativeFfiRuntimeEventSprite*
maplibre_native_ffi_runtime_event_sprite_new(void) {
  MaplibreNativeFfiRuntimeEventSprite* self =
    g_new0(MaplibreNativeFfiRuntimeEventSprite, 1);
  self->id = g_strdup("");
  self->url = g_strdup("");
  return self;
}

MaplibreNativeFfiRuntimeEventSprite*
maplibre_native_ffi_runtime_event_sprite_copy(
  const MaplibreNativeFfiRuntimeEventSprite* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiRuntimeEventSprite* copy = g_memdup2(self, sizeof *self);
  copy->id = g_strdup(self->id);
  copy->url = g_strdup(self->url);
  return copy;
}

void maplibre_native_ffi_runtime_event_sprite_free(
  MaplibreNativeFfiRuntimeEventSprite* self
) {
  if (self == NULL) return;
  g_free(self->id);
  g_free(self->url);
  g_free(self);
}

const char* maplibre_native_ffi_runtime_event_sprite_get_id(
  const MaplibreNativeFfiRuntimeEventSprite* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->id;
}

const char* maplibre_native_ffi_runtime_event_sprite_get_url(
  const MaplibreNativeFfiRuntimeEventSprite* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->url;
}

gpointer maplibre_native_ffi_runtime_event_sprite_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_sprite_copy(
    (const MaplibreNativeFfiRuntimeEventSprite*)self
  );
}

void maplibre_native_ffi_runtime_event_sprite_to_native(
  const MaplibreNativeFfiRuntimeEventSprite* self,
  mln_runtime_event_sprite* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->id = maplibre_native_ffi_text_view(self->id);
  out->url = maplibre_native_ffi_text_view(self->url);
}

MaplibreNativeFfiRuntimeEventSprite*
maplibre_native_ffi_runtime_event_sprite_from_native(
  const mln_runtime_event_sprite* in
) {
  MaplibreNativeFfiRuntimeEventSprite* self =
    maplibre_native_ffi_runtime_event_sprite_new();
  g_free(self->id);
  self->id = maplibre_native_ffi_text_new(in->id);
  g_free(self->url);
  self->url = maplibre_native_ffi_text_new(in->url);
  return self;
}

struct _MaplibreNativeFfiRuntimeEventPayload {
  gint64 tag;
  MaplibreNativeFfiRuntimeEventCameraChange camera_change;
  MaplibreNativeFfiRuntimeEventLoadingFailed loading_failed;
  MaplibreNativeFfiRuntimeEventRenderFrame render_frame;
  MaplibreNativeFfiRuntimeEventStyleImage* style_image;
  MaplibreNativeFfiRuntimeEventTileAction* tile_action;
  MaplibreNativeFfiRuntimeEventSourceChanged* source_changed;
  MaplibreNativeFfiRuntimeEventOfflineRegionStatus offline_region_status;
  MaplibreNativeFfiRuntimeEventOfflineRegionResponseError
    offline_region_response_error;
  MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit
    offline_region_tile_count_limit;
  MaplibreNativeFfiRuntimeEventGlyphs* glyphs;
  MaplibreNativeFfiRuntimeEventSprite* sprite;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEventPayload,
  maplibre_native_ffi_runtime_event_payload,
  maplibre_native_ffi_runtime_event_payload_copy,
  maplibre_native_ffi_runtime_event_payload_free
)

MaplibreNativeFfiRuntimeEventPayload*
maplibre_native_ffi_runtime_event_payload_copy(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiRuntimeEventPayload* copy = g_memdup2(self, sizeof *self);
  copy->style_image =
    self->style_image != NULL
      ? maplibre_native_ffi_runtime_event_style_image_copy(self->style_image)
      : NULL;
  copy->tile_action =
    self->tile_action != NULL
      ? maplibre_native_ffi_runtime_event_tile_action_copy(self->tile_action)
      : NULL;
  copy->source_changed =
    self->source_changed != NULL
      ? maplibre_native_ffi_runtime_event_source_changed_copy(
          self->source_changed
        )
      : NULL;
  copy->glyphs = self->glyphs != NULL
                   ? maplibre_native_ffi_runtime_event_glyphs_copy(self->glyphs)
                   : NULL;
  copy->sprite = self->sprite != NULL
                   ? maplibre_native_ffi_runtime_event_sprite_copy(self->sprite)
                   : NULL;
  return copy;
}

void maplibre_native_ffi_runtime_event_payload_free(
  MaplibreNativeFfiRuntimeEventPayload* self
) {
  if (self == NULL) return;
  g_clear_pointer(
    &self->style_image, maplibre_native_ffi_runtime_event_style_image_free
  );
  g_clear_pointer(
    &self->tile_action, maplibre_native_ffi_runtime_event_tile_action_free
  );
  g_clear_pointer(
    &self->source_changed, maplibre_native_ffi_runtime_event_source_changed_free
  );
  g_clear_pointer(&self->glyphs, maplibre_native_ffi_runtime_event_glyphs_free);
  g_clear_pointer(&self->sprite, maplibre_native_ffi_runtime_event_sprite_free);
  g_free(self);
}

gboolean maplibre_native_ffi_runtime_event_payload_is_camera_change(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_CHANGE;
}

MaplibreNativeFfiRuntimeEventCameraChange*
maplibre_native_ffi_runtime_event_payload_get_camera_change(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(
    self->tag == MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_CHANGE, NULL
  );
  return (MaplibreNativeFfiRuntimeEventCameraChange*)&self->camera_change;
}

gboolean maplibre_native_ffi_runtime_event_payload_is_loading_failed(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RUNTIME_EVENT_PAYLOAD_LOADING_FAILED;
}

MaplibreNativeFfiRuntimeEventLoadingFailed*
maplibre_native_ffi_runtime_event_payload_get_loading_failed(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(
    self->tag == MLN_RUNTIME_EVENT_PAYLOAD_LOADING_FAILED, NULL
  );
  return (MaplibreNativeFfiRuntimeEventLoadingFailed*)&self->loading_failed;
}

gboolean maplibre_native_ffi_runtime_event_payload_is_render_frame(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME;
}

MaplibreNativeFfiRuntimeEventRenderFrame*
maplibre_native_ffi_runtime_event_payload_get_render_frame(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(
    self->tag == MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME, NULL
  );
  return (MaplibreNativeFfiRuntimeEventRenderFrame*)&self->render_frame;
}

gboolean maplibre_native_ffi_runtime_event_payload_is_style_image(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE;
}

MaplibreNativeFfiRuntimeEventStyleImage*
maplibre_native_ffi_runtime_event_payload_get_style_image(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(
    self->tag == MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE, NULL
  );
  return self->style_image;
}

gboolean maplibre_native_ffi_runtime_event_payload_is_tile_action(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION;
}

MaplibreNativeFfiRuntimeEventTileAction*
maplibre_native_ffi_runtime_event_payload_get_tile_action(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(
    self->tag == MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION, NULL
  );
  return self->tile_action;
}

gboolean maplibre_native_ffi_runtime_event_payload_is_source_changed(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RUNTIME_EVENT_PAYLOAD_SOURCE_CHANGED;
}

MaplibreNativeFfiRuntimeEventSourceChanged*
maplibre_native_ffi_runtime_event_payload_get_source_changed(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(
    self->tag == MLN_RUNTIME_EVENT_PAYLOAD_SOURCE_CHANGED, NULL
  );
  return self->source_changed;
}

gboolean maplibre_native_ffi_runtime_event_payload_is_offline_region_status(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS;
}

MaplibreNativeFfiRuntimeEventOfflineRegionStatus*
maplibre_native_ffi_runtime_event_payload_get_offline_region_status(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(
    self->tag == MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS, NULL
  );
  return (
           MaplibreNativeFfiRuntimeEventOfflineRegionStatus*
  )&self->offline_region_status;
}

gboolean
maplibre_native_ffi_runtime_event_payload_is_offline_region_response_error(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR;
}

MaplibreNativeFfiRuntimeEventOfflineRegionResponseError*
maplibre_native_ffi_runtime_event_payload_get_offline_region_response_error(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(
    self->tag == MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR, NULL
  );
  return (
           MaplibreNativeFfiRuntimeEventOfflineRegionResponseError*
  )&self->offline_region_response_error;
}

gboolean
maplibre_native_ffi_runtime_event_payload_is_offline_region_tile_count_limit(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT;
}

MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit*
maplibre_native_ffi_runtime_event_payload_get_offline_region_tile_count_limit(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(
    self->tag == MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT, NULL
  );
  return (
           MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit*
  )&self->offline_region_tile_count_limit;
}

gboolean maplibre_native_ffi_runtime_event_payload_is_glyphs(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS;
}

MaplibreNativeFfiRuntimeEventGlyphs*
maplibre_native_ffi_runtime_event_payload_get_glyphs(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(self->tag == MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS, NULL);
  return self->glyphs;
}

gboolean maplibre_native_ffi_runtime_event_payload_is_sprite(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RUNTIME_EVENT_PAYLOAD_SPRITE;
}

MaplibreNativeFfiRuntimeEventSprite*
maplibre_native_ffi_runtime_event_payload_get_sprite(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(self->tag == MLN_RUNTIME_EVENT_PAYLOAD_SPRITE, NULL);
  return self->sprite;
}

gboolean maplibre_native_ffi_runtime_event_payload_is_none(
  const MaplibreNativeFfiRuntimeEventPayload* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->tag == MLN_RUNTIME_EVENT_PAYLOAD_NONE;
}

void maplibre_native_ffi_runtime_event_payload_to_native(
  const MaplibreNativeFfiRuntimeEventPayload* self,
  mln_runtime_event_payload_type* out_tag, mln_runtime_event_payload* out,
  MaplibreNativeFfiCall* call
) {
  mln_runtime_event_payload* data = out;
  memset(data, 0, sizeof *data);
  *out_tag = (mln_runtime_event_payload_type)self->tag;
  switch (self->tag) {
    case MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_CHANGE:
      maplibre_native_ffi_runtime_event_camera_change_to_native(
        &self->camera_change, &data->camera_change, call
      );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_LOADING_FAILED:
      maplibre_native_ffi_runtime_event_loading_failed_to_native(
        &self->loading_failed, &data->loading_failed, call
      );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME:
      maplibre_native_ffi_runtime_event_render_frame_to_native(
        &self->render_frame, &data->render_frame, call
      );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE:
      if (self->style_image != NULL)
        maplibre_native_ffi_runtime_event_style_image_to_native(
          self->style_image, &data->style_image, call
        );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION:
      if (self->tile_action != NULL)
        maplibre_native_ffi_runtime_event_tile_action_to_native(
          self->tile_action, &data->tile_action, call
        );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_SOURCE_CHANGED:
      if (self->source_changed != NULL)
        maplibre_native_ffi_runtime_event_source_changed_to_native(
          self->source_changed, &data->source_changed, call
        );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS:
      maplibre_native_ffi_runtime_event_offline_region_status_to_native(
        &self->offline_region_status, &data->offline_region_status, call
      );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR:
      maplibre_native_ffi_runtime_event_offline_region_response_error_to_native(
        &self->offline_region_response_error,
        &data->offline_region_response_error, call
      );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT:
      maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_to_native(
        &self->offline_region_tile_count_limit,
        &data->offline_region_tile_count_limit, call
      );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS:
      if (self->glyphs != NULL)
        maplibre_native_ffi_runtime_event_glyphs_to_native(
          self->glyphs, &data->glyphs, call
        );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_SPRITE:
      if (self->sprite != NULL)
        maplibre_native_ffi_runtime_event_sprite_to_native(
          self->sprite, &data->sprite, call
        );
      break;
    default:
      break;
  }
}

MaplibreNativeFfiRuntimeEventPayload*
maplibre_native_ffi_runtime_event_payload_from_native(
  gint64 tag, const mln_runtime_event_payload* in
) {
  const mln_runtime_event_payload* data = in;
  MaplibreNativeFfiRuntimeEventPayload* self =
    g_new0(MaplibreNativeFfiRuntimeEventPayload, 1);
  self->tag = tag;
  switch (tag) {
    case MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_CHANGE:
      maplibre_native_ffi_runtime_event_camera_change_read(
        &self->camera_change, &data->camera_change
      );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_LOADING_FAILED:
      maplibre_native_ffi_runtime_event_loading_failed_read(
        &self->loading_failed, &data->loading_failed
      );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME:
      maplibre_native_ffi_runtime_event_render_frame_read(
        &self->render_frame, &data->render_frame
      );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE:
      self->style_image =
        maplibre_native_ffi_runtime_event_style_image_from_native(
          &data->style_image
        );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION:
      self->tile_action =
        maplibre_native_ffi_runtime_event_tile_action_from_native(
          &data->tile_action
        );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_SOURCE_CHANGED:
      self->source_changed =
        maplibre_native_ffi_runtime_event_source_changed_from_native(
          &data->source_changed
        );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS:
      maplibre_native_ffi_runtime_event_offline_region_status_read(
        &self->offline_region_status, &data->offline_region_status
      );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR:
      maplibre_native_ffi_runtime_event_offline_region_response_error_read(
        &self->offline_region_response_error,
        &data->offline_region_response_error
      );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT:
      maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_read(
        &self->offline_region_tile_count_limit,
        &data->offline_region_tile_count_limit
      );
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS:
      self->glyphs =
        maplibre_native_ffi_runtime_event_glyphs_from_native(&data->glyphs);
      break;
    case MLN_RUNTIME_EVENT_PAYLOAD_SPRITE:
      self->sprite =
        maplibre_native_ffi_runtime_event_sprite_from_native(&data->sprite);
      break;
    default:
      break;
  }
  return self;
}

gpointer maplibre_native_ffi_runtime_event_payload_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_payload_copy(
    (const MaplibreNativeFfiRuntimeEventPayload*)self
  );
}

struct _MaplibreNativeFfiRuntimeEvent {
  MaplibreNativeFfiRuntimeEventType type;
  MaplibreNativeFfiRuntimeEventSourceType source_type;
  guint64 source;
  guint64 generation;
  char* message;
  MaplibreNativeFfiRuntimeEventPayload* payload;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEvent, maplibre_native_ffi_runtime_event,
  maplibre_native_ffi_runtime_event_copy, maplibre_native_ffi_runtime_event_free
)

MaplibreNativeFfiRuntimeEvent* maplibre_native_ffi_runtime_event_new(void) {
  MaplibreNativeFfiRuntimeEvent* self =
    g_new0(MaplibreNativeFfiRuntimeEvent, 1);
  self->message = g_strdup("");
  return self;
}

MaplibreNativeFfiRuntimeEvent* maplibre_native_ffi_runtime_event_copy(
  const MaplibreNativeFfiRuntimeEvent* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiRuntimeEvent* copy = g_memdup2(self, sizeof *self);
  copy->message = g_strdup(self->message);
  copy->payload =
    self->payload != NULL
      ? maplibre_native_ffi_runtime_event_payload_copy(self->payload)
      : NULL;
  return copy;
}

void maplibre_native_ffi_runtime_event_free(
  MaplibreNativeFfiRuntimeEvent* self
) {
  if (self == NULL) return;
  g_free(self->message);
  g_clear_pointer(
    &self->payload, maplibre_native_ffi_runtime_event_payload_free
  );
  g_free(self);
}

MaplibreNativeFfiRuntimeEventType
maplibre_native_ffi_runtime_event_get_event_type(
  const MaplibreNativeFfiRuntimeEvent* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiRuntimeEventType)0);
  return self->type;
}

MaplibreNativeFfiRuntimeEventSourceType
maplibre_native_ffi_runtime_event_get_source_type(
  const MaplibreNativeFfiRuntimeEvent* self
) {
  g_return_val_if_fail(
    self != NULL, (MaplibreNativeFfiRuntimeEventSourceType)0
  );
  return self->source_type;
}

guint64 maplibre_native_ffi_runtime_event_get_source(
  const MaplibreNativeFfiRuntimeEvent* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->source;
}

guint64 maplibre_native_ffi_runtime_event_get_generation(
  const MaplibreNativeFfiRuntimeEvent* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->generation;
}

const char* maplibre_native_ffi_runtime_event_get_message(
  const MaplibreNativeFfiRuntimeEvent* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->message;
}

MaplibreNativeFfiRuntimeEventPayload*
maplibre_native_ffi_runtime_event_get_payload(
  const MaplibreNativeFfiRuntimeEvent* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->payload;
}

gpointer maplibre_native_ffi_runtime_event_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_copy(
    (const MaplibreNativeFfiRuntimeEvent*)self
  );
}

void maplibre_native_ffi_runtime_event_to_native(
  const MaplibreNativeFfiRuntimeEvent* self, mln_runtime_event* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->type = (mln_runtime_event_type)(self->type);
  out->source_type = (mln_runtime_event_source_type)(self->source_type);
  out->source = (uint64_t)(self->source);
  out->generation = (uint64_t)(self->generation);
  out->message = maplibre_native_ffi_text_view(self->message);
  if (self->payload != NULL)
    maplibre_native_ffi_runtime_event_payload_to_native(
      self->payload, &out->payload_type, &out->payload, call
    );
}

MaplibreNativeFfiRuntimeEvent* maplibre_native_ffi_runtime_event_from_native(
  const mln_runtime_event* in
) {
  MaplibreNativeFfiRuntimeEvent* self = maplibre_native_ffi_runtime_event_new();
  self->type = (MaplibreNativeFfiRuntimeEventType)(in->type);
  self->source_type =
    (MaplibreNativeFfiRuntimeEventSourceType)(in->source_type);
  self->source = (guint64)(in->source);
  self->generation = (guint64)(in->generation);
  g_free(self->message);
  self->message = maplibre_native_ffi_text_new(in->message);
  g_clear_pointer(
    &self->payload, maplibre_native_ffi_runtime_event_payload_free
  );
  self->payload = maplibre_native_ffi_runtime_event_payload_from_native(
    (gint64)in->payload_type, &in->payload
  );
  return self;
}

struct _MaplibreNativeFfiRuntimeEventHandler {
  MaplibreNativeFfiClosure* callback;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiRuntimeEventHandler,
  maplibre_native_ffi_runtime_event_handler,
  maplibre_native_ffi_runtime_event_handler_copy,
  maplibre_native_ffi_runtime_event_handler_free
)

MaplibreNativeFfiRuntimeEventHandler*
maplibre_native_ffi_runtime_event_handler_new(void) {
  MaplibreNativeFfiRuntimeEventHandler* self =
    g_new0(MaplibreNativeFfiRuntimeEventHandler, 1);
  return self;
}

MaplibreNativeFfiRuntimeEventHandler*
maplibre_native_ffi_runtime_event_handler_copy(
  const MaplibreNativeFfiRuntimeEventHandler* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiRuntimeEventHandler* copy = g_memdup2(self, sizeof *self);
  maplibre_native_ffi_closure_ref(copy->callback);
  return copy;
}

void maplibre_native_ffi_runtime_event_handler_free(
  MaplibreNativeFfiRuntimeEventHandler* self
) {
  if (self == NULL) return;
  maplibre_native_ffi_closure_unref(self->callback);
  g_free(self);
}

void maplibre_native_ffi_runtime_event_handler_set_callback(
  MaplibreNativeFfiRuntimeEventHandler* self,
  MaplibreNativeFfiRuntimeEventCallback callback, gpointer user_data,
  GDestroyNotify notify
) {
  g_return_if_fail(self != NULL);
  MaplibreNativeFfiClosure* previous = self->callback;
  self->callback =
    callback != NULL
      ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify)
      : NULL;
  if (callback == NULL && notify != NULL) notify(user_data);
  maplibre_native_ffi_closure_unref(previous);
}

gpointer maplibre_native_ffi_runtime_event_handler_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_event_handler_copy(
    (const MaplibreNativeFfiRuntimeEventHandler*)self
  );
}

static void runtime_event_handler_callback_trampoline(
  void* user_data, const mln_event_batch_view* events
) {
  MaplibreNativeFfiClosure* closure =
    maplibre_native_ffi_registration_closure(user_data, 0);
  if (closure == NULL) return;
  GPtrArray* events_ = g_ptr_array_new_full(
    (guint)events->event_count,
    (GDestroyNotify)maplibre_native_ffi_runtime_event_free
  );
  for (size_t index = 0; index < events->event_count; index++) {
    g_ptr_array_add(
      events_,
      maplibre_native_ffi_runtime_event_from_native(
        (const mln_runtime_event*)(const void*)((const char*)events->events +
                                                index * events->event_size)
      )
    );
  }
  ((MaplibreNativeFfiRuntimeEventCallback)(void (*)(void))closure->func)(
    events_, closure->user_data
  );
  g_ptr_array_unref(events_);
}

void maplibre_native_ffi_runtime_event_handler_to_native(
  const MaplibreNativeFfiRuntimeEventHandler* self,
  mln_runtime_event_handler* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  if (self->callback != NULL) {
    MaplibreNativeFfiClosure* const closures[] = {self->callback};
    out->user_data =
      maplibre_native_ffi_call_register(call, closures, 1, &out->queue);
    out->release_user_data = maplibre_native_ffi_registration_release;
    out->callback =
      self->callback != NULL ? runtime_event_handler_callback_trampoline : NULL;
  }
}

MaplibreNativeFfiRuntimeEventHandler*
maplibre_native_ffi_runtime_event_handler_from_native(
  const mln_runtime_event_handler* in
) {
  MaplibreNativeFfiRuntimeEventHandler* self =
    maplibre_native_ffi_runtime_event_handler_new();
  (void)in;
  return self;
}

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
  MaplibreNativeFfiRuntimeOptions* copy = g_memdup2(self, sizeof *self);
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
  char* previous = self->asset_path;
  self->asset_path = g_strdup(value != NULL ? value : "");
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
  char* previous = self->cache_path;
  self->cache_path = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

gpointer maplibre_native_ffi_runtime_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_runtime_options_copy(
    (const MaplibreNativeFfiRuntimeOptions*)self
  );
}

void maplibre_native_ffi_runtime_options_to_native(
  const MaplibreNativeFfiRuntimeOptions* self, mln_runtime_options* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->asset_path = maplibre_native_ffi_text_view(self->asset_path);
  out->cache_path = maplibre_native_ffi_text_view(self->cache_path);
}

MaplibreNativeFfiRuntimeOptions*
maplibre_native_ffi_runtime_options_from_native(const mln_runtime_options* in) {
  MaplibreNativeFfiRuntimeOptions* self =
    maplibre_native_ffi_runtime_options_new();
  g_free(self->asset_path);
  self->asset_path = maplibre_native_ffi_text_new(in->asset_path);
  g_free(self->cache_path);
  self->cache_path = maplibre_native_ffi_text_new(in->cache_path);
  return self;
}

struct _MaplibreNativeFfiSourceFeature {
  char* feature;
  char* source_layer_id;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiSourceFeature, maplibre_native_ffi_source_feature,
  maplibre_native_ffi_source_feature_copy,
  maplibre_native_ffi_source_feature_free
)

MaplibreNativeFfiSourceFeature* maplibre_native_ffi_source_feature_new(void) {
  MaplibreNativeFfiSourceFeature* self =
    g_new0(MaplibreNativeFfiSourceFeature, 1);
  self->feature = g_strdup("");
  self->source_layer_id = g_strdup("");
  return self;
}

MaplibreNativeFfiSourceFeature* maplibre_native_ffi_source_feature_copy(
  const MaplibreNativeFfiSourceFeature* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiSourceFeature* copy = g_memdup2(self, sizeof *self);
  copy->feature = g_strdup(self->feature);
  copy->source_layer_id = g_strdup(self->source_layer_id);
  return copy;
}

void maplibre_native_ffi_source_feature_free(
  MaplibreNativeFfiSourceFeature* self
) {
  if (self == NULL) return;
  g_free(self->feature);
  g_free(self->source_layer_id);
  g_free(self);
}

const char* maplibre_native_ffi_source_feature_get_feature(
  const MaplibreNativeFfiSourceFeature* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->feature;
}

const char* maplibre_native_ffi_source_feature_get_source_layer_id(
  const MaplibreNativeFfiSourceFeature* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->source_layer_id;
}

gpointer maplibre_native_ffi_source_feature_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_source_feature_copy(
    (const MaplibreNativeFfiSourceFeature*)self
  );
}

void maplibre_native_ffi_source_feature_to_native(
  const MaplibreNativeFfiSourceFeature* self, mln_source_feature* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->feature = maplibre_native_ffi_text_view(self->feature);
  out->source_layer_id = maplibre_native_ffi_text_view(self->source_layer_id);
}

MaplibreNativeFfiSourceFeature* maplibre_native_ffi_source_feature_from_native(
  const mln_source_feature* in
) {
  MaplibreNativeFfiSourceFeature* self =
    maplibre_native_ffi_source_feature_new();
  g_free(self->feature);
  self->feature = maplibre_native_ffi_text_new(in->feature);
  g_free(self->source_layer_id);
  self->source_layer_id = maplibre_native_ffi_text_new(in->source_layer_id);
  return self;
}

struct _MaplibreNativeFfiSourceFeatureQueryOptions {
  guint64 fields;
  char** source_layer_ids;
  char* filter;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiSourceFeatureQueryOptions,
  maplibre_native_ffi_source_feature_query_options,
  maplibre_native_ffi_source_feature_query_options_copy,
  maplibre_native_ffi_source_feature_query_options_free
)

MaplibreNativeFfiSourceFeatureQueryOptions*
maplibre_native_ffi_source_feature_query_options_new(void) {
  MaplibreNativeFfiSourceFeatureQueryOptions* self =
    g_new0(MaplibreNativeFfiSourceFeatureQueryOptions, 1);
  self->source_layer_ids = g_new0(char*, 1);
  self->filter = g_strdup("");
  return self;
}

MaplibreNativeFfiSourceFeatureQueryOptions*
maplibre_native_ffi_source_feature_query_options_copy(
  const MaplibreNativeFfiSourceFeatureQueryOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiSourceFeatureQueryOptions* copy =
    g_memdup2(self, sizeof *self);
  copy->source_layer_ids = g_strdupv(self->source_layer_ids);
  copy->filter = g_strdup(self->filter);
  return copy;
}

void maplibre_native_ffi_source_feature_query_options_free(
  MaplibreNativeFfiSourceFeatureQueryOptions* self
) {
  if (self == NULL) return;
  g_strfreev(self->source_layer_ids);
  g_free(self->filter);
  g_free(self);
}

const char* const*
maplibre_native_ffi_source_feature_query_options_get_source_layer_ids(
  const MaplibreNativeFfiSourceFeatureQueryOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (const char* const*)self->source_layer_ids;
}

gboolean maplibre_native_ffi_source_feature_query_options_has_source_layer_ids(
  const MaplibreNativeFfiSourceFeatureQueryOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS) != 0;
}

void maplibre_native_ffi_source_feature_query_options_set_source_layer_ids(
  MaplibreNativeFfiSourceFeatureQueryOptions* self, const char* const* value
) {
  g_return_if_fail(self != NULL);
  char** previous = self->source_layer_ids;
  self->source_layer_ids =
    value != NULL ? g_strdupv((char**)value) : g_new0(char*, 1);
  g_strfreev(previous);
  self->fields |= MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
}

const char* maplibre_native_ffi_source_feature_query_options_get_filter(
  const MaplibreNativeFfiSourceFeatureQueryOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->filter;
}

void maplibre_native_ffi_source_feature_query_options_set_filter(
  MaplibreNativeFfiSourceFeatureQueryOptions* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->filter;
  self->filter = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

gpointer maplibre_native_ffi_source_feature_query_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_source_feature_query_options_copy(
    (const MaplibreNativeFfiSourceFeatureQueryOptions*)self
  );
}

void maplibre_native_ffi_source_feature_query_options_to_native(
  const MaplibreNativeFfiSourceFeatureQueryOptions* self,
  mln_source_feature_query_options* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_source_feature_query_option_fields)self->fields;
  if (self->source_layer_ids != NULL && self->source_layer_ids[0] != NULL) {
    guint count = g_strv_length(self->source_layer_ids);
    mln_buffer_view* items =
      maplibre_native_ffi_call_alloc(call, count, sizeof *items);
    for (guint index = 0; index < count; index++) {
      items[index] =
        maplibre_native_ffi_text_view(self->source_layer_ids[index]);
    }
    out->source_layer_ids = items;
    out->source_layer_id_count = count;
  }
  out->filter = maplibre_native_ffi_text_view(self->filter);
}

MaplibreNativeFfiSourceFeatureQueryOptions*
maplibre_native_ffi_source_feature_query_options_from_native(
  const mln_source_feature_query_options* in
) {
  MaplibreNativeFfiSourceFeatureQueryOptions* self =
    maplibre_native_ffi_source_feature_query_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS) != 0) {
    g_strfreev(self->source_layer_ids);
    self->source_layer_ids = g_new0(char*, in->source_layer_id_count + 1);
    for (size_t index = 0; index < in->source_layer_id_count; index++) {
      self->source_layer_ids[index] =
        maplibre_native_ffi_text_new(in->source_layer_ids[index]);
    }
  }
  g_free(self->filter);
  self->filter = maplibre_native_ffi_text_new(in->filter);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiStyleExpressionInfo,
  maplibre_native_ffi_style_expression_info,
  maplibre_native_ffi_style_expression_info_copy,
  maplibre_native_ffi_style_expression_info_free
)

MaplibreNativeFfiStyleExpressionInfo*
maplibre_native_ffi_style_expression_info_copy(
  const MaplibreNativeFfiStyleExpressionInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_style_expression_info_free(
  MaplibreNativeFfiStyleExpressionInfo* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_style_expression_info_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_style_expression_info_copy(
    (const MaplibreNativeFfiStyleExpressionInfo*)self
  );
}

void maplibre_native_ffi_style_expression_info_to_native(
  const MaplibreNativeFfiStyleExpressionInfo* self,
  mln_style_expression_info* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->is_feature_constant = (self->is_feature_constant) != FALSE;
  out->is_zoom_constant = (self->is_zoom_constant) != FALSE;
}

void maplibre_native_ffi_style_expression_info_read(
  MaplibreNativeFfiStyleExpressionInfo* out, const mln_style_expression_info* in
) {
  out->is_feature_constant = (in->is_feature_constant) ? TRUE : FALSE;
  out->is_zoom_constant = (in->is_zoom_constant) ? TRUE : FALSE;
}

MaplibreNativeFfiStyleExpressionInfo*
maplibre_native_ffi_style_expression_info_from_native(
  const mln_style_expression_info* in
) {
  MaplibreNativeFfiStyleExpressionInfo* self =
    g_new0(MaplibreNativeFfiStyleExpressionInfo, 1);
  maplibre_native_ffi_style_expression_info_read(self, in);
  return self;
}

struct _MaplibreNativeFfiStyleExpressionInputs {
  guint64 fields;
  gdouble zoom;
  gdouble color_ramp_parameter;
  char* feature;
  char* feature_state;
  char* global_state;
  MaplibreNativeFfiCanonicalTileId canonical_tile_id;
  char** available_images;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiStyleExpressionInputs,
  maplibre_native_ffi_style_expression_inputs,
  maplibre_native_ffi_style_expression_inputs_copy,
  maplibre_native_ffi_style_expression_inputs_free
)

MaplibreNativeFfiStyleExpressionInputs*
maplibre_native_ffi_style_expression_inputs_new(void) {
  MaplibreNativeFfiStyleExpressionInputs* self =
    g_new0(MaplibreNativeFfiStyleExpressionInputs, 1);
  self->feature = g_strdup("");
  self->feature_state = g_strdup("");
  self->global_state = g_strdup("");
  self->available_images = g_new0(char*, 1);
  return self;
}

MaplibreNativeFfiStyleExpressionInputs*
maplibre_native_ffi_style_expression_inputs_copy(
  const MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiStyleExpressionInputs* copy = g_memdup2(self, sizeof *self);
  copy->feature = g_strdup(self->feature);
  copy->feature_state = g_strdup(self->feature_state);
  copy->global_state = g_strdup(self->global_state);
  copy->available_images = g_strdupv(self->available_images);
  return copy;
}

void maplibre_native_ffi_style_expression_inputs_free(
  MaplibreNativeFfiStyleExpressionInputs* self
) {
  if (self == NULL) return;
  g_free(self->feature);
  g_free(self->feature_state);
  g_free(self->global_state);
  g_strfreev(self->available_images);
  g_free(self);
}

gdouble maplibre_native_ffi_style_expression_inputs_get_zoom(
  const MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->zoom;
}

gboolean maplibre_native_ffi_style_expression_inputs_has_zoom(
  const MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_EXPRESSION_INPUT_ZOOM) != 0;
}

void maplibre_native_ffi_style_expression_inputs_set_zoom(
  MaplibreNativeFfiStyleExpressionInputs* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->zoom = value;
  self->fields |= MLN_STYLE_EXPRESSION_INPUT_ZOOM;
}

gdouble maplibre_native_ffi_style_expression_inputs_get_color_ramp_parameter(
  const MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->color_ramp_parameter;
}

gboolean maplibre_native_ffi_style_expression_inputs_has_color_ramp_parameter(
  const MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER) != 0;
}

void maplibre_native_ffi_style_expression_inputs_set_color_ramp_parameter(
  MaplibreNativeFfiStyleExpressionInputs* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->color_ramp_parameter = value;
  self->fields |= MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER;
}

const char* maplibre_native_ffi_style_expression_inputs_get_feature(
  const MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->feature;
}

void maplibre_native_ffi_style_expression_inputs_set_feature(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->feature;
  self->feature = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

const char* maplibre_native_ffi_style_expression_inputs_get_feature_state(
  const MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->feature_state;
}

void maplibre_native_ffi_style_expression_inputs_set_feature_state(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->feature_state;
  self->feature_state = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

const char* maplibre_native_ffi_style_expression_inputs_get_global_state(
  const MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->global_state;
}

void maplibre_native_ffi_style_expression_inputs_set_global_state(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* value
) {
  g_return_if_fail(self != NULL);
  char* previous = self->global_state;
  self->global_state = g_strdup(value != NULL ? value : "");
  g_free(previous);
}

MaplibreNativeFfiCanonicalTileId*
maplibre_native_ffi_style_expression_inputs_get_canonical_tile_id(
  const MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiCanonicalTileId*)&self->canonical_tile_id;
}

gboolean maplibre_native_ffi_style_expression_inputs_has_canonical_tile_id(
  const MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID) != 0;
}

void maplibre_native_ffi_style_expression_inputs_set_canonical_tile_id(
  MaplibreNativeFfiStyleExpressionInputs* self,
  const MaplibreNativeFfiCanonicalTileId* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->canonical_tile_id = *value;
  self->fields |= MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID;
}

const char* const*
maplibre_native_ffi_style_expression_inputs_get_available_images(
  const MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (const char* const*)self->available_images;
}

void maplibre_native_ffi_style_expression_inputs_set_available_images(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* const* value
) {
  g_return_if_fail(self != NULL);
  char** previous = self->available_images;
  self->available_images =
    value != NULL ? g_strdupv((char**)value) : g_new0(char*, 1);
  g_strfreev(previous);
}

gpointer maplibre_native_ffi_style_expression_inputs_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_style_expression_inputs_copy(
    (const MaplibreNativeFfiStyleExpressionInputs*)self
  );
}

void maplibre_native_ffi_style_expression_inputs_to_native(
  const MaplibreNativeFfiStyleExpressionInputs* self,
  mln_style_expression_inputs* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_style_expression_input_fields)self->fields;
  out->zoom = (double)(self->zoom);
  out->color_ramp_parameter = (double)(self->color_ramp_parameter);
  out->feature = maplibre_native_ffi_text_view(self->feature);
  out->feature_state = maplibre_native_ffi_text_view(self->feature_state);
  out->global_state = maplibre_native_ffi_text_view(self->global_state);
  maplibre_native_ffi_canonical_tile_id_to_native(
    &self->canonical_tile_id, &out->canonical_tile_id, call
  );
  if (self->available_images != NULL && self->available_images[0] != NULL) {
    guint count = g_strv_length(self->available_images);
    mln_buffer_view* items =
      maplibre_native_ffi_call_alloc(call, count, sizeof *items);
    for (guint index = 0; index < count; index++) {
      items[index] =
        maplibre_native_ffi_text_view(self->available_images[index]);
    }
    out->available_images = items;
    out->available_image_count = count;
  }
}

MaplibreNativeFfiStyleExpressionInputs*
maplibre_native_ffi_style_expression_inputs_from_native(
  const mln_style_expression_inputs* in
) {
  MaplibreNativeFfiStyleExpressionInputs* self =
    maplibre_native_ffi_style_expression_inputs_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_STYLE_EXPRESSION_INPUT_ZOOM) != 0) {
    self->zoom = (gdouble)(in->zoom);
  }
  if ((in->fields & MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER) != 0) {
    self->color_ramp_parameter = (gdouble)(in->color_ramp_parameter);
  }
  g_free(self->feature);
  self->feature = maplibre_native_ffi_text_new(in->feature);
  g_free(self->feature_state);
  self->feature_state = maplibre_native_ffi_text_new(in->feature_state);
  g_free(self->global_state);
  self->global_state = maplibre_native_ffi_text_new(in->global_state);
  if ((in->fields & MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID) != 0) {
    maplibre_native_ffi_canonical_tile_id_read(
      &self->canonical_tile_id, &in->canonical_tile_id
    );
  }
  g_strfreev(self->available_images);
  self->available_images = g_new0(char*, in->available_image_count + 1);
  for (size_t index = 0; index < in->available_image_count; index++) {
    self->available_images[index] =
      maplibre_native_ffi_text_new(in->available_images[index]);
  }
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiStyleExpressionType,
  maplibre_native_ffi_style_expression_type,
  maplibre_native_ffi_style_expression_type_copy,
  maplibre_native_ffi_style_expression_type_free
)

MaplibreNativeFfiStyleExpressionType*
maplibre_native_ffi_style_expression_type_copy(
  const MaplibreNativeFfiStyleExpressionType* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_style_expression_type_free(
  MaplibreNativeFfiStyleExpressionType* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_style_expression_type_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_style_expression_type_copy(
    (const MaplibreNativeFfiStyleExpressionType*)self
  );
}

void maplibre_native_ffi_style_expression_type_to_native(
  const MaplibreNativeFfiStyleExpressionType* self,
  mln_style_expression_type* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->kind = (mln_style_expression_type_kind)(self->kind);
  out->item_kind = (mln_style_expression_type_kind)(self->item_kind);
  out->item_count = (uint32_t)(self->item_count);
}

void maplibre_native_ffi_style_expression_type_read(
  MaplibreNativeFfiStyleExpressionType* out, const mln_style_expression_type* in
) {
  out->kind = (MaplibreNativeFfiStyleExpressionTypeKind)(in->kind);
  out->item_kind = (MaplibreNativeFfiStyleExpressionTypeKind)(in->item_kind);
  out->item_count = (guint32)(in->item_count);
}

MaplibreNativeFfiStyleExpressionType*
maplibre_native_ffi_style_expression_type_from_native(
  const mln_style_expression_type* in
) {
  MaplibreNativeFfiStyleExpressionType* self =
    g_new0(MaplibreNativeFfiStyleExpressionType, 1);
  maplibre_native_ffi_style_expression_type_read(self, in);
  return self;
}

struct _MaplibreNativeFfiStyleExpressionOptions {
  guint64 fields;
  MaplibreNativeFfiStyleExpressionType expected_type;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiStyleExpressionOptions,
  maplibre_native_ffi_style_expression_options,
  maplibre_native_ffi_style_expression_options_copy,
  maplibre_native_ffi_style_expression_options_free
)

MaplibreNativeFfiStyleExpressionOptions*
maplibre_native_ffi_style_expression_options_new(void) {
  MaplibreNativeFfiStyleExpressionOptions* self =
    g_new0(MaplibreNativeFfiStyleExpressionOptions, 1);
  return self;
}

MaplibreNativeFfiStyleExpressionOptions*
maplibre_native_ffi_style_expression_options_copy(
  const MaplibreNativeFfiStyleExpressionOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiStyleExpressionOptions* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_style_expression_options_free(
  MaplibreNativeFfiStyleExpressionOptions* self
) {
  if (self == NULL) return;
  g_free(self);
}

MaplibreNativeFfiStyleExpressionType*
maplibre_native_ffi_style_expression_options_get_expected_type(
  const MaplibreNativeFfiStyleExpressionOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiStyleExpressionType*)&self->expected_type;
}

gboolean maplibre_native_ffi_style_expression_options_has_expected_type(
  const MaplibreNativeFfiStyleExpressionOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE) != 0;
}

void maplibre_native_ffi_style_expression_options_set_expected_type(
  MaplibreNativeFfiStyleExpressionOptions* self,
  const MaplibreNativeFfiStyleExpressionType* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->expected_type = *value;
  self->fields |= MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE;
}

gpointer maplibre_native_ffi_style_expression_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_style_expression_options_copy(
    (const MaplibreNativeFfiStyleExpressionOptions*)self
  );
}

void maplibre_native_ffi_style_expression_options_to_native(
  const MaplibreNativeFfiStyleExpressionOptions* self,
  mln_style_expression_options* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_style_expression_option_fields)self->fields;
  maplibre_native_ffi_style_expression_type_to_native(
    &self->expected_type, &out->expected_type, call
  );
}

MaplibreNativeFfiStyleExpressionOptions*
maplibre_native_ffi_style_expression_options_from_native(
  const mln_style_expression_options* in
) {
  MaplibreNativeFfiStyleExpressionOptions* self =
    maplibre_native_ffi_style_expression_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE) != 0) {
    maplibre_native_ffi_style_expression_type_read(
      &self->expected_type, &in->expected_type
    );
  }
  return self;
}

struct _MaplibreNativeFfiStyleImageInfo {
  guint64 fields;
  guint32 width;
  guint32 height;
  GBytes* pixels;
  GPtrArray* stretch_x;
  GPtrArray* stretch_y;
  MaplibreNativeFfiImageContent content;
  MaplibreNativeFfiStyleImageTextFit text_fit_width;
  MaplibreNativeFfiStyleImageTextFit text_fit_height;
  gfloat pixel_ratio;
  gboolean sdf;
  gboolean keep_unused;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiStyleImageInfo, maplibre_native_ffi_style_image_info,
  maplibre_native_ffi_style_image_info_copy,
  maplibre_native_ffi_style_image_info_free
)

MaplibreNativeFfiStyleImageInfo* maplibre_native_ffi_style_image_info_new(
  void
) {
  MaplibreNativeFfiStyleImageInfo* self =
    g_new0(MaplibreNativeFfiStyleImageInfo, 1);
  self->stretch_x = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_image_stretch_free
  );
  self->stretch_y = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_image_stretch_free
  );
  return self;
}

MaplibreNativeFfiStyleImageInfo* maplibre_native_ffi_style_image_info_copy(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiStyleImageInfo* copy = g_memdup2(self, sizeof *self);
  copy->pixels = self->pixels != NULL ? g_bytes_ref(self->pixels) : NULL;
  copy->stretch_x =
    self->stretch_x != NULL
      ? g_ptr_array_copy(
          self->stretch_x, maplibre_native_ffi_image_stretch_copy_func, NULL
        )
      : NULL;
  if (copy->stretch_x != NULL)
    g_ptr_array_set_free_func(
      copy->stretch_x, (GDestroyNotify)maplibre_native_ffi_image_stretch_free
    );
  copy->stretch_y =
    self->stretch_y != NULL
      ? g_ptr_array_copy(
          self->stretch_y, maplibre_native_ffi_image_stretch_copy_func, NULL
        )
      : NULL;
  if (copy->stretch_y != NULL)
    g_ptr_array_set_free_func(
      copy->stretch_y, (GDestroyNotify)maplibre_native_ffi_image_stretch_free
    );
  return copy;
}

void maplibre_native_ffi_style_image_info_free(
  MaplibreNativeFfiStyleImageInfo* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->pixels, g_bytes_unref);
  g_clear_pointer(&self->stretch_x, g_ptr_array_unref);
  g_clear_pointer(&self->stretch_y, g_ptr_array_unref);
  g_free(self);
}

guint32 maplibre_native_ffi_style_image_info_get_width(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->width;
}

guint32 maplibre_native_ffi_style_image_info_get_height(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->height;
}

GBytes* maplibre_native_ffi_style_image_info_get_pixels(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->pixels;
}

GPtrArray* maplibre_native_ffi_style_image_info_get_stretch_x(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->stretch_x;
}

GPtrArray* maplibre_native_ffi_style_image_info_get_stretch_y(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->stretch_y;
}

MaplibreNativeFfiImageContent* maplibre_native_ffi_style_image_info_get_content(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiImageContent*)&self->content;
}

gboolean maplibre_native_ffi_style_image_info_has_content(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_IMAGE_INFO_CONTENT) != 0;
}

MaplibreNativeFfiStyleImageTextFit
maplibre_native_ffi_style_image_info_get_text_fit_width(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiStyleImageTextFit)0);
  return self->text_fit_width;
}

gboolean maplibre_native_ffi_style_image_info_has_text_fit_width(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH) != 0;
}

MaplibreNativeFfiStyleImageTextFit
maplibre_native_ffi_style_image_info_get_text_fit_height(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiStyleImageTextFit)0);
  return self->text_fit_height;
}

gboolean maplibre_native_ffi_style_image_info_has_text_fit_height(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT) != 0;
}

gfloat maplibre_native_ffi_style_image_info_get_pixel_ratio(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->pixel_ratio;
}

gboolean maplibre_native_ffi_style_image_info_get_sdf(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->sdf;
}

gboolean maplibre_native_ffi_style_image_info_get_keep_unused(
  const MaplibreNativeFfiStyleImageInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->keep_unused;
}

gpointer maplibre_native_ffi_style_image_info_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_style_image_info_copy(
    (const MaplibreNativeFfiStyleImageInfo*)self
  );
}

void maplibre_native_ffi_style_image_info_to_native(
  const MaplibreNativeFfiStyleImageInfo* self, mln_style_image_info* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->fields = (mln_style_image_info_fields)self->fields;
  out->width = (uint32_t)(self->width);
  out->height = (uint32_t)(self->height);
  out->pixels = maplibre_native_ffi_bytes_view(self->pixels);
  if (self->stretch_x != NULL && self->stretch_x->len > 0) {
    mln_image_stretch* items =
      maplibre_native_ffi_call_alloc(call, self->stretch_x->len, sizeof *items);
    for (guint index = 0; index < self->stretch_x->len; index++) {
      maplibre_native_ffi_image_stretch_to_native(
        self->stretch_x->pdata[index], &items[index], call
      );
    }
    out->stretch_x = items;
    out->stretch_x_count = self->stretch_x->len;
  }
  if (self->stretch_y != NULL && self->stretch_y->len > 0) {
    mln_image_stretch* items =
      maplibre_native_ffi_call_alloc(call, self->stretch_y->len, sizeof *items);
    for (guint index = 0; index < self->stretch_y->len; index++) {
      maplibre_native_ffi_image_stretch_to_native(
        self->stretch_y->pdata[index], &items[index], call
      );
    }
    out->stretch_y = items;
    out->stretch_y_count = self->stretch_y->len;
  }
  maplibre_native_ffi_image_content_to_native(
    &self->content, &out->content, call
  );
  out->text_fit_width = (mln_style_image_text_fit)(self->text_fit_width);
  out->text_fit_height = (mln_style_image_text_fit)(self->text_fit_height);
  out->pixel_ratio = (float)(self->pixel_ratio);
  out->sdf = (self->sdf) != FALSE;
  out->keep_unused = (self->keep_unused) != FALSE;
}

MaplibreNativeFfiStyleImageInfo*
maplibre_native_ffi_style_image_info_from_native(
  const mln_style_image_info* in
) {
  MaplibreNativeFfiStyleImageInfo* self =
    maplibre_native_ffi_style_image_info_new();
  self->fields = (guint64)in->fields;
  self->width = (guint32)(in->width);
  self->height = (guint32)(in->height);
  g_clear_pointer(&self->pixels, g_bytes_unref);
  self->pixels =
    maplibre_native_ffi_bytes_new(in->pixels.data, in->pixels.size);
  g_clear_pointer(&self->stretch_x, g_ptr_array_unref);
  self->stretch_x = g_ptr_array_new_full(
    (guint)in->stretch_x_count,
    (GDestroyNotify)maplibre_native_ffi_image_stretch_free
  );
  for (size_t index = 0; index < in->stretch_x_count; index++) {
    g_ptr_array_add(
      self->stretch_x,
      maplibre_native_ffi_image_stretch_from_native(&in->stretch_x[index])
    );
  }
  g_clear_pointer(&self->stretch_y, g_ptr_array_unref);
  self->stretch_y = g_ptr_array_new_full(
    (guint)in->stretch_y_count,
    (GDestroyNotify)maplibre_native_ffi_image_stretch_free
  );
  for (size_t index = 0; index < in->stretch_y_count; index++) {
    g_ptr_array_add(
      self->stretch_y,
      maplibre_native_ffi_image_stretch_from_native(&in->stretch_y[index])
    );
  }
  if ((in->fields & MLN_STYLE_IMAGE_INFO_CONTENT) != 0) {
    maplibre_native_ffi_image_content_read(&self->content, &in->content);
  }
  if ((in->fields & MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH) != 0) {
    self->text_fit_width =
      (MaplibreNativeFfiStyleImageTextFit)(in->text_fit_width);
  }
  if ((in->fields & MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT) != 0) {
    self->text_fit_height =
      (MaplibreNativeFfiStyleImageTextFit)(in->text_fit_height);
  }
  self->pixel_ratio = (gfloat)(in->pixel_ratio);
  self->sdf = (in->sdf) ? TRUE : FALSE;
  self->keep_unused = (in->keep_unused) ? TRUE : FALSE;
  return self;
}

struct _MaplibreNativeFfiStyleImageOptions {
  guint64 fields;
  GPtrArray* stretch_x;
  GPtrArray* stretch_y;
  MaplibreNativeFfiImageContent content;
  MaplibreNativeFfiStyleImageTextFit text_fit_width;
  MaplibreNativeFfiStyleImageTextFit text_fit_height;
  gfloat pixel_ratio;
  gboolean sdf;
  gboolean keep_unused;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiStyleImageOptions, maplibre_native_ffi_style_image_options,
  maplibre_native_ffi_style_image_options_copy,
  maplibre_native_ffi_style_image_options_free
)

MaplibreNativeFfiStyleImageOptions* maplibre_native_ffi_style_image_options_new(
  void
) {
  MaplibreNativeFfiStyleImageOptions* self =
    g_new0(MaplibreNativeFfiStyleImageOptions, 1);
  self->stretch_x = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_image_stretch_free
  );
  self->stretch_y = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_image_stretch_free
  );
  return self;
}

MaplibreNativeFfiStyleImageOptions*
maplibre_native_ffi_style_image_options_copy(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiStyleImageOptions* copy = g_memdup2(self, sizeof *self);
  copy->stretch_x =
    self->stretch_x != NULL
      ? g_ptr_array_copy(
          self->stretch_x, maplibre_native_ffi_image_stretch_copy_func, NULL
        )
      : NULL;
  if (copy->stretch_x != NULL)
    g_ptr_array_set_free_func(
      copy->stretch_x, (GDestroyNotify)maplibre_native_ffi_image_stretch_free
    );
  copy->stretch_y =
    self->stretch_y != NULL
      ? g_ptr_array_copy(
          self->stretch_y, maplibre_native_ffi_image_stretch_copy_func, NULL
        )
      : NULL;
  if (copy->stretch_y != NULL)
    g_ptr_array_set_free_func(
      copy->stretch_y, (GDestroyNotify)maplibre_native_ffi_image_stretch_free
    );
  return copy;
}

void maplibre_native_ffi_style_image_options_free(
  MaplibreNativeFfiStyleImageOptions* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->stretch_x, g_ptr_array_unref);
  g_clear_pointer(&self->stretch_y, g_ptr_array_unref);
  g_free(self);
}

GPtrArray* maplibre_native_ffi_style_image_options_get_stretch_x(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->stretch_x;
}

gboolean maplibre_native_ffi_style_image_options_has_stretch_x(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_STRETCH_X) != 0;
}

void maplibre_native_ffi_style_image_options_set_stretch_x(
  MaplibreNativeFfiStyleImageOptions* self,
  const MaplibreNativeFfiImageStretch* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->stretch_x;
  self->stretch_x = g_ptr_array_new_full(
    (guint)value_count, (GDestroyNotify)maplibre_native_ffi_image_stretch_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->stretch_x, maplibre_native_ffi_image_stretch_copy(&value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
  self->fields |= MLN_STYLE_IMAGE_OPTION_STRETCH_X;
}

GPtrArray* maplibre_native_ffi_style_image_options_get_stretch_y(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->stretch_y;
}

gboolean maplibre_native_ffi_style_image_options_has_stretch_y(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_STRETCH_Y) != 0;
}

void maplibre_native_ffi_style_image_options_set_stretch_y(
  MaplibreNativeFfiStyleImageOptions* self,
  const MaplibreNativeFfiImageStretch* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->stretch_y;
  self->stretch_y = g_ptr_array_new_full(
    (guint)value_count, (GDestroyNotify)maplibre_native_ffi_image_stretch_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->stretch_y, maplibre_native_ffi_image_stretch_copy(&value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
  self->fields |= MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
}

MaplibreNativeFfiImageContent*
maplibre_native_ffi_style_image_options_get_content(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiImageContent*)&self->content;
}

gboolean maplibre_native_ffi_style_image_options_has_content(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_CONTENT) != 0;
}

void maplibre_native_ffi_style_image_options_set_content(
  MaplibreNativeFfiStyleImageOptions* self,
  const MaplibreNativeFfiImageContent* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->content = *value;
  self->fields |= MLN_STYLE_IMAGE_OPTION_CONTENT;
}

MaplibreNativeFfiStyleImageTextFit
maplibre_native_ffi_style_image_options_get_text_fit_width(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiStyleImageTextFit)0);
  return self->text_fit_width;
}

gboolean maplibre_native_ffi_style_image_options_has_text_fit_width(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH) != 0;
}

void maplibre_native_ffi_style_image_options_set_text_fit_width(
  MaplibreNativeFfiStyleImageOptions* self,
  MaplibreNativeFfiStyleImageTextFit value
) {
  g_return_if_fail(self != NULL);
  self->text_fit_width = value;
  self->fields |= MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
}

MaplibreNativeFfiStyleImageTextFit
maplibre_native_ffi_style_image_options_get_text_fit_height(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiStyleImageTextFit)0);
  return self->text_fit_height;
}

gboolean maplibre_native_ffi_style_image_options_has_text_fit_height(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT) != 0;
}

void maplibre_native_ffi_style_image_options_set_text_fit_height(
  MaplibreNativeFfiStyleImageOptions* self,
  MaplibreNativeFfiStyleImageTextFit value
) {
  g_return_if_fail(self != NULL);
  self->text_fit_height = value;
  self->fields |= MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
}

gfloat maplibre_native_ffi_style_image_options_get_pixel_ratio(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->pixel_ratio;
}

gboolean maplibre_native_ffi_style_image_options_has_pixel_ratio(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO) != 0;
}

void maplibre_native_ffi_style_image_options_set_pixel_ratio(
  MaplibreNativeFfiStyleImageOptions* self, gfloat value
) {
  g_return_if_fail(self != NULL);
  self->pixel_ratio = value;
  self->fields |= MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
}

gboolean maplibre_native_ffi_style_image_options_get_sdf(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->sdf;
}

gboolean maplibre_native_ffi_style_image_options_has_sdf(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_SDF) != 0;
}

void maplibre_native_ffi_style_image_options_set_sdf(
  MaplibreNativeFfiStyleImageOptions* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->sdf = value;
  self->fields |= MLN_STYLE_IMAGE_OPTION_SDF;
}

gboolean maplibre_native_ffi_style_image_options_get_keep_unused(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->keep_unused;
}

gboolean maplibre_native_ffi_style_image_options_has_keep_unused(
  const MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED) != 0;
}

void maplibre_native_ffi_style_image_options_set_keep_unused(
  MaplibreNativeFfiStyleImageOptions* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->keep_unused = value;
  self->fields |= MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED;
}

gpointer maplibre_native_ffi_style_image_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_style_image_options_copy(
    (const MaplibreNativeFfiStyleImageOptions*)self
  );
}

void maplibre_native_ffi_style_image_options_to_native(
  const MaplibreNativeFfiStyleImageOptions* self, mln_style_image_options* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_style_image_option_fields)self->fields;
  if (self->stretch_x != NULL && self->stretch_x->len > 0) {
    mln_image_stretch* items =
      maplibre_native_ffi_call_alloc(call, self->stretch_x->len, sizeof *items);
    for (guint index = 0; index < self->stretch_x->len; index++) {
      maplibre_native_ffi_image_stretch_to_native(
        self->stretch_x->pdata[index], &items[index], call
      );
    }
    out->stretch_x = items;
    out->stretch_x_count = self->stretch_x->len;
  }
  if (self->stretch_y != NULL && self->stretch_y->len > 0) {
    mln_image_stretch* items =
      maplibre_native_ffi_call_alloc(call, self->stretch_y->len, sizeof *items);
    for (guint index = 0; index < self->stretch_y->len; index++) {
      maplibre_native_ffi_image_stretch_to_native(
        self->stretch_y->pdata[index], &items[index], call
      );
    }
    out->stretch_y = items;
    out->stretch_y_count = self->stretch_y->len;
  }
  maplibre_native_ffi_image_content_to_native(
    &self->content, &out->content, call
  );
  out->text_fit_width = (mln_style_image_text_fit)(self->text_fit_width);
  out->text_fit_height = (mln_style_image_text_fit)(self->text_fit_height);
  out->pixel_ratio = (float)(self->pixel_ratio);
  out->sdf = (self->sdf) != FALSE;
  out->keep_unused = (self->keep_unused) != FALSE;
}

MaplibreNativeFfiStyleImageOptions*
maplibre_native_ffi_style_image_options_from_native(
  const mln_style_image_options* in
) {
  MaplibreNativeFfiStyleImageOptions* self =
    maplibre_native_ffi_style_image_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_STYLE_IMAGE_OPTION_STRETCH_X) != 0) {
    g_clear_pointer(&self->stretch_x, g_ptr_array_unref);
    self->stretch_x = g_ptr_array_new_full(
      (guint)in->stretch_x_count,
      (GDestroyNotify)maplibre_native_ffi_image_stretch_free
    );
    for (size_t index = 0; index < in->stretch_x_count; index++) {
      g_ptr_array_add(
        self->stretch_x,
        maplibre_native_ffi_image_stretch_from_native(&in->stretch_x[index])
      );
    }
  }
  if ((in->fields & MLN_STYLE_IMAGE_OPTION_STRETCH_Y) != 0) {
    g_clear_pointer(&self->stretch_y, g_ptr_array_unref);
    self->stretch_y = g_ptr_array_new_full(
      (guint)in->stretch_y_count,
      (GDestroyNotify)maplibre_native_ffi_image_stretch_free
    );
    for (size_t index = 0; index < in->stretch_y_count; index++) {
      g_ptr_array_add(
        self->stretch_y,
        maplibre_native_ffi_image_stretch_from_native(&in->stretch_y[index])
      );
    }
  }
  if ((in->fields & MLN_STYLE_IMAGE_OPTION_CONTENT) != 0) {
    maplibre_native_ffi_image_content_read(&self->content, &in->content);
  }
  if ((in->fields & MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH) != 0) {
    self->text_fit_width =
      (MaplibreNativeFfiStyleImageTextFit)(in->text_fit_width);
  }
  if ((in->fields & MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT) != 0) {
    self->text_fit_height =
      (MaplibreNativeFfiStyleImageTextFit)(in->text_fit_height);
  }
  if ((in->fields & MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO) != 0) {
    self->pixel_ratio = (gfloat)(in->pixel_ratio);
  }
  if ((in->fields & MLN_STYLE_IMAGE_OPTION_SDF) != 0) {
    self->sdf = (in->sdf) ? TRUE : FALSE;
  }
  if ((in->fields & MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED) != 0) {
    self->keep_unused = (in->keep_unused) ? TRUE : FALSE;
  }
  return self;
}

struct _MaplibreNativeFfiStyleLayerInfo {
  char* id;
  char* type;
  char* source_id;
  char* source_layer;
  gdouble min_zoom;
  gdouble max_zoom;
  MaplibreNativeFfiStyleLayerVisibility visibility;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiStyleLayerInfo, maplibre_native_ffi_style_layer_info,
  maplibre_native_ffi_style_layer_info_copy,
  maplibre_native_ffi_style_layer_info_free
)

MaplibreNativeFfiStyleLayerInfo* maplibre_native_ffi_style_layer_info_new(
  void
) {
  MaplibreNativeFfiStyleLayerInfo* self =
    g_new0(MaplibreNativeFfiStyleLayerInfo, 1);
  self->id = g_strdup("");
  self->type = g_strdup("");
  self->source_id = g_strdup("");
  self->source_layer = g_strdup("");
  return self;
}

MaplibreNativeFfiStyleLayerInfo* maplibre_native_ffi_style_layer_info_copy(
  const MaplibreNativeFfiStyleLayerInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiStyleLayerInfo* copy = g_memdup2(self, sizeof *self);
  copy->id = g_strdup(self->id);
  copy->type = g_strdup(self->type);
  copy->source_id = g_strdup(self->source_id);
  copy->source_layer = g_strdup(self->source_layer);
  return copy;
}

void maplibre_native_ffi_style_layer_info_free(
  MaplibreNativeFfiStyleLayerInfo* self
) {
  if (self == NULL) return;
  g_free(self->id);
  g_free(self->type);
  g_free(self->source_id);
  g_free(self->source_layer);
  g_free(self);
}

const char* maplibre_native_ffi_style_layer_info_get_id(
  const MaplibreNativeFfiStyleLayerInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->id;
}

const char* maplibre_native_ffi_style_layer_info_get_layer_type(
  const MaplibreNativeFfiStyleLayerInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->type;
}

const char* maplibre_native_ffi_style_layer_info_get_source_id(
  const MaplibreNativeFfiStyleLayerInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->source_id;
}

const char* maplibre_native_ffi_style_layer_info_get_source_layer(
  const MaplibreNativeFfiStyleLayerInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->source_layer;
}

gdouble maplibre_native_ffi_style_layer_info_get_min_zoom(
  const MaplibreNativeFfiStyleLayerInfo* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->min_zoom;
}

gdouble maplibre_native_ffi_style_layer_info_get_max_zoom(
  const MaplibreNativeFfiStyleLayerInfo* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->max_zoom;
}

MaplibreNativeFfiStyleLayerVisibility
maplibre_native_ffi_style_layer_info_get_visibility(
  const MaplibreNativeFfiStyleLayerInfo* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiStyleLayerVisibility)0);
  return self->visibility;
}

gpointer maplibre_native_ffi_style_layer_info_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_style_layer_info_copy(
    (const MaplibreNativeFfiStyleLayerInfo*)self
  );
}

void maplibre_native_ffi_style_layer_info_to_native(
  const MaplibreNativeFfiStyleLayerInfo* self, mln_style_layer_info* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->id = maplibre_native_ffi_text_view(self->id);
  out->type = maplibre_native_ffi_text_view(self->type);
  out->source_id = maplibre_native_ffi_text_view(self->source_id);
  out->source_layer = maplibre_native_ffi_text_view(self->source_layer);
  out->min_zoom = (double)(self->min_zoom);
  out->max_zoom = (double)(self->max_zoom);
  out->visibility = (mln_style_layer_visibility)(self->visibility);
}

MaplibreNativeFfiStyleLayerInfo*
maplibre_native_ffi_style_layer_info_from_native(
  const mln_style_layer_info* in
) {
  MaplibreNativeFfiStyleLayerInfo* self =
    maplibre_native_ffi_style_layer_info_new();
  g_free(self->id);
  self->id = maplibre_native_ffi_text_new(in->id);
  g_free(self->type);
  self->type = maplibre_native_ffi_text_new(in->type);
  g_free(self->source_id);
  self->source_id = maplibre_native_ffi_text_new(in->source_id);
  g_free(self->source_layer);
  self->source_layer = maplibre_native_ffi_text_new(in->source_layer);
  self->min_zoom = (gdouble)(in->min_zoom);
  self->max_zoom = (gdouble)(in->max_zoom);
  self->visibility = (MaplibreNativeFfiStyleLayerVisibility)(in->visibility);
  return self;
}

struct _MaplibreNativeFfiStyleSourceTileInfo {
  char** tile_urls;
  gdouble min_zoom;
  gdouble max_zoom;
  MaplibreNativeFfiStyleTileScheme scheme;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiStyleSourceTileInfo,
  maplibre_native_ffi_style_source_tile_info,
  maplibre_native_ffi_style_source_tile_info_copy,
  maplibre_native_ffi_style_source_tile_info_free
)

MaplibreNativeFfiStyleSourceTileInfo*
maplibre_native_ffi_style_source_tile_info_new(void) {
  MaplibreNativeFfiStyleSourceTileInfo* self =
    g_new0(MaplibreNativeFfiStyleSourceTileInfo, 1);
  self->tile_urls = g_new0(char*, 1);
  return self;
}

MaplibreNativeFfiStyleSourceTileInfo*
maplibre_native_ffi_style_source_tile_info_copy(
  const MaplibreNativeFfiStyleSourceTileInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiStyleSourceTileInfo* copy = g_memdup2(self, sizeof *self);
  copy->tile_urls = g_strdupv(self->tile_urls);
  return copy;
}

void maplibre_native_ffi_style_source_tile_info_free(
  MaplibreNativeFfiStyleSourceTileInfo* self
) {
  if (self == NULL) return;
  g_strfreev(self->tile_urls);
  g_free(self);
}

const char* const* maplibre_native_ffi_style_source_tile_info_get_tile_urls(
  const MaplibreNativeFfiStyleSourceTileInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (const char* const*)self->tile_urls;
}

gdouble maplibre_native_ffi_style_source_tile_info_get_min_zoom(
  const MaplibreNativeFfiStyleSourceTileInfo* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->min_zoom;
}

gdouble maplibre_native_ffi_style_source_tile_info_get_max_zoom(
  const MaplibreNativeFfiStyleSourceTileInfo* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->max_zoom;
}

MaplibreNativeFfiStyleTileScheme
maplibre_native_ffi_style_source_tile_info_get_scheme(
  const MaplibreNativeFfiStyleSourceTileInfo* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiStyleTileScheme)0);
  return self->scheme;
}

gpointer maplibre_native_ffi_style_source_tile_info_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_style_source_tile_info_copy(
    (const MaplibreNativeFfiStyleSourceTileInfo*)self
  );
}

void maplibre_native_ffi_style_source_tile_info_to_native(
  const MaplibreNativeFfiStyleSourceTileInfo* self,
  mln_style_source_tile_info* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  if (self->tile_urls != NULL && self->tile_urls[0] != NULL) {
    guint count = g_strv_length(self->tile_urls);
    mln_buffer_view* items =
      maplibre_native_ffi_call_alloc(call, count, sizeof *items);
    for (guint index = 0; index < count; index++) {
      items[index] = maplibre_native_ffi_text_view(self->tile_urls[index]);
    }
    out->tile_urls = items;
    out->tile_url_count = count;
  }
  out->min_zoom = (double)(self->min_zoom);
  out->max_zoom = (double)(self->max_zoom);
  out->scheme = (mln_style_tile_scheme)(self->scheme);
}

MaplibreNativeFfiStyleSourceTileInfo*
maplibre_native_ffi_style_source_tile_info_from_native(
  const mln_style_source_tile_info* in
) {
  MaplibreNativeFfiStyleSourceTileInfo* self =
    maplibre_native_ffi_style_source_tile_info_new();
  g_strfreev(self->tile_urls);
  self->tile_urls = g_new0(char*, in->tile_url_count + 1);
  for (size_t index = 0; index < in->tile_url_count; index++) {
    self->tile_urls[index] = maplibre_native_ffi_text_new(in->tile_urls[index]);
  }
  self->min_zoom = (gdouble)(in->min_zoom);
  self->max_zoom = (gdouble)(in->max_zoom);
  self->scheme = (MaplibreNativeFfiStyleTileScheme)(in->scheme);
  return self;
}

struct _MaplibreNativeFfiStyleSourceInfo {
  guint64 fields;
  char* id;
  MaplibreNativeFfiStyleSourceType type;
  gboolean is_volatile;
  char* attribution;
  char* url;
  MaplibreNativeFfiStyleSourceTileInfo* tile_json;
  MaplibreNativeFfiLatLngBounds bounds;
  guint32 tile_size;
  MaplibreNativeFfiStyleVectorTileEncoding vector_encoding;
  MaplibreNativeFfiStyleRasterDemEncoding raster_encoding;
  MaplibreNativeFfiGeoJsonSourceOptions* geo_json;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiStyleSourceInfo, maplibre_native_ffi_style_source_info,
  maplibre_native_ffi_style_source_info_copy,
  maplibre_native_ffi_style_source_info_free
)

MaplibreNativeFfiStyleSourceInfo* maplibre_native_ffi_style_source_info_new(
  void
) {
  MaplibreNativeFfiStyleSourceInfo* self =
    g_new0(MaplibreNativeFfiStyleSourceInfo, 1);
  self->id = g_strdup("");
  self->attribution = g_strdup("");
  self->url = g_strdup("");
  self->tile_json = maplibre_native_ffi_style_source_tile_info_new();
  self->geo_json = maplibre_native_ffi_geo_json_source_options_new();
  return self;
}

MaplibreNativeFfiStyleSourceInfo* maplibre_native_ffi_style_source_info_copy(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiStyleSourceInfo* copy = g_memdup2(self, sizeof *self);
  copy->id = g_strdup(self->id);
  copy->attribution = g_strdup(self->attribution);
  copy->url = g_strdup(self->url);
  copy->tile_json =
    self->tile_json != NULL
      ? maplibre_native_ffi_style_source_tile_info_copy(self->tile_json)
      : NULL;
  copy->geo_json =
    self->geo_json != NULL
      ? maplibre_native_ffi_geo_json_source_options_copy(self->geo_json)
      : NULL;
  return copy;
}

void maplibre_native_ffi_style_source_info_free(
  MaplibreNativeFfiStyleSourceInfo* self
) {
  if (self == NULL) return;
  g_free(self->id);
  g_free(self->attribution);
  g_free(self->url);
  g_clear_pointer(
    &self->tile_json, maplibre_native_ffi_style_source_tile_info_free
  );
  g_clear_pointer(
    &self->geo_json, maplibre_native_ffi_geo_json_source_options_free
  );
  g_free(self);
}

const char* maplibre_native_ffi_style_source_info_get_id(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->id;
}

MaplibreNativeFfiStyleSourceType
maplibre_native_ffi_style_source_info_get_source_type(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiStyleSourceType)0);
  return self->type;
}

gboolean maplibre_native_ffi_style_source_info_get_is_volatile(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->is_volatile;
}

const char* maplibre_native_ffi_style_source_info_get_attribution(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->attribution;
}

const char* maplibre_native_ffi_style_source_info_get_url(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->url;
}

MaplibreNativeFfiStyleSourceTileInfo*
maplibre_native_ffi_style_source_info_get_tile_json(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->tile_json;
}

gboolean maplibre_native_ffi_style_source_info_has_tile_json(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_SOURCE_INFO_TILE_JSON) != 0;
}

MaplibreNativeFfiLatLngBounds* maplibre_native_ffi_style_source_info_get_bounds(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLatLngBounds*)&self->bounds;
}

gboolean maplibre_native_ffi_style_source_info_has_bounds(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_SOURCE_INFO_BOUNDS) != 0;
}

guint32 maplibre_native_ffi_style_source_info_get_tile_size(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->tile_size;
}

gboolean maplibre_native_ffi_style_source_info_has_tile_size(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_SOURCE_INFO_TILE_SIZE) != 0;
}

MaplibreNativeFfiStyleVectorTileEncoding
maplibre_native_ffi_style_source_info_get_vector_encoding(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(
    self != NULL, (MaplibreNativeFfiStyleVectorTileEncoding)0
  );
  return self->vector_encoding;
}

gboolean maplibre_native_ffi_style_source_info_has_vector_encoding(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING) != 0;
}

MaplibreNativeFfiStyleRasterDemEncoding
maplibre_native_ffi_style_source_info_get_raster_encoding(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(
    self != NULL, (MaplibreNativeFfiStyleRasterDemEncoding)0
  );
  return self->raster_encoding;
}

gboolean maplibre_native_ffi_style_source_info_has_raster_encoding(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_SOURCE_INFO_RASTER_ENCODING) != 0;
}

MaplibreNativeFfiGeoJsonSourceOptions*
maplibre_native_ffi_style_source_info_get_geo_json(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->geo_json;
}

gboolean maplibre_native_ffi_style_source_info_has_geo_json(
  const MaplibreNativeFfiStyleSourceInfo* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_SOURCE_INFO_GEO_JSON) != 0;
}

gpointer maplibre_native_ffi_style_source_info_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_style_source_info_copy(
    (const MaplibreNativeFfiStyleSourceInfo*)self
  );
}

void maplibre_native_ffi_style_source_info_to_native(
  const MaplibreNativeFfiStyleSourceInfo* self, mln_style_source_info* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->fields = (mln_style_source_info_fields)self->fields;
  out->id = maplibre_native_ffi_text_view(self->id);
  out->type = (mln_style_source_type)(self->type);
  out->is_volatile = (self->is_volatile) != FALSE;
  out->attribution = maplibre_native_ffi_text_view(self->attribution);
  out->url = maplibre_native_ffi_text_view(self->url);
  if (self->tile_json != NULL)
    maplibre_native_ffi_style_source_tile_info_to_native(
      self->tile_json, &out->tile_json, call
    );
  maplibre_native_ffi_lat_lng_bounds_to_native(
    &self->bounds, &out->bounds, call
  );
  out->tile_size = (uint32_t)(self->tile_size);
  out->vector_encoding =
    (mln_style_vector_tile_encoding)(self->vector_encoding);
  out->raster_encoding = (mln_style_raster_dem_encoding)(self->raster_encoding);
  if (self->geo_json != NULL)
    maplibre_native_ffi_geo_json_source_options_to_native(
      self->geo_json, &out->geo_json, call
    );
}

MaplibreNativeFfiStyleSourceInfo*
maplibre_native_ffi_style_source_info_from_native(
  const mln_style_source_info* in
) {
  MaplibreNativeFfiStyleSourceInfo* self =
    maplibre_native_ffi_style_source_info_new();
  self->fields = (guint64)in->fields;
  g_free(self->id);
  self->id = maplibre_native_ffi_text_new(in->id);
  self->type = (MaplibreNativeFfiStyleSourceType)(in->type);
  self->is_volatile = (in->is_volatile) ? TRUE : FALSE;
  g_free(self->attribution);
  self->attribution = maplibre_native_ffi_text_new(in->attribution);
  g_free(self->url);
  self->url = maplibre_native_ffi_text_new(in->url);
  if ((in->fields & MLN_STYLE_SOURCE_INFO_TILE_JSON) != 0) {
    g_clear_pointer(
      &self->tile_json, maplibre_native_ffi_style_source_tile_info_free
    );
    self->tile_json =
      maplibre_native_ffi_style_source_tile_info_from_native(&in->tile_json);
  }
  if ((in->fields & MLN_STYLE_SOURCE_INFO_BOUNDS) != 0) {
    maplibre_native_ffi_lat_lng_bounds_read(&self->bounds, &in->bounds);
  }
  if ((in->fields & MLN_STYLE_SOURCE_INFO_TILE_SIZE) != 0) {
    self->tile_size = (guint32)(in->tile_size);
  }
  if ((in->fields & MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING) != 0) {
    self->vector_encoding =
      (MaplibreNativeFfiStyleVectorTileEncoding)(in->vector_encoding);
  }
  if ((in->fields & MLN_STYLE_SOURCE_INFO_RASTER_ENCODING) != 0) {
    self->raster_encoding =
      (MaplibreNativeFfiStyleRasterDemEncoding)(in->raster_encoding);
  }
  if ((in->fields & MLN_STYLE_SOURCE_INFO_GEO_JSON) != 0) {
    g_clear_pointer(
      &self->geo_json, maplibre_native_ffi_geo_json_source_options_free
    );
    self->geo_json =
      maplibre_native_ffi_geo_json_source_options_from_native(&in->geo_json);
  }
  return self;
}

struct _MaplibreNativeFfiStyleTransitionOptions {
  guint64 fields;
  gdouble duration_ms;
  gdouble delay_ms;
  gboolean enable_placement_transitions;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiStyleTransitionOptions,
  maplibre_native_ffi_style_transition_options,
  maplibre_native_ffi_style_transition_options_copy,
  maplibre_native_ffi_style_transition_options_free
)

MaplibreNativeFfiStyleTransitionOptions*
maplibre_native_ffi_style_transition_options_new(void) {
  MaplibreNativeFfiStyleTransitionOptions* self =
    g_new0(MaplibreNativeFfiStyleTransitionOptions, 1);
  return self;
}

MaplibreNativeFfiStyleTransitionOptions*
maplibre_native_ffi_style_transition_options_copy(
  const MaplibreNativeFfiStyleTransitionOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiStyleTransitionOptions* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_style_transition_options_free(
  MaplibreNativeFfiStyleTransitionOptions* self
) {
  if (self == NULL) return;
  g_free(self);
}

gdouble maplibre_native_ffi_style_transition_options_get_duration_ms(
  const MaplibreNativeFfiStyleTransitionOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->duration_ms;
}

gboolean maplibre_native_ffi_style_transition_options_has_duration_ms(
  const MaplibreNativeFfiStyleTransitionOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_TRANSITION_OPTION_DURATION) != 0;
}

void maplibre_native_ffi_style_transition_options_set_duration_ms(
  MaplibreNativeFfiStyleTransitionOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->duration_ms = value;
  self->fields |= MLN_STYLE_TRANSITION_OPTION_DURATION;
}

gdouble maplibre_native_ffi_style_transition_options_get_delay_ms(
  const MaplibreNativeFfiStyleTransitionOptions* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->delay_ms;
}

gboolean maplibre_native_ffi_style_transition_options_has_delay_ms(
  const MaplibreNativeFfiStyleTransitionOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_STYLE_TRANSITION_OPTION_DELAY) != 0;
}

void maplibre_native_ffi_style_transition_options_set_delay_ms(
  MaplibreNativeFfiStyleTransitionOptions* self, gdouble value
) {
  g_return_if_fail(self != NULL);
  self->delay_ms = value;
  self->fields |= MLN_STYLE_TRANSITION_OPTION_DELAY;
}

gboolean
maplibre_native_ffi_style_transition_options_get_enable_placement_transitions(
  const MaplibreNativeFfiStyleTransitionOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->enable_placement_transitions;
}

gboolean
maplibre_native_ffi_style_transition_options_has_enable_placement_transitions(
  const MaplibreNativeFfiStyleTransitionOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields &
          MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS) != 0;
}

void maplibre_native_ffi_style_transition_options_set_enable_placement_transitions(
  MaplibreNativeFfiStyleTransitionOptions* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->enable_placement_transitions = value;
  self->fields |= MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
}

gpointer maplibre_native_ffi_style_transition_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_style_transition_options_copy(
    (const MaplibreNativeFfiStyleTransitionOptions*)self
  );
}

void maplibre_native_ffi_style_transition_options_to_native(
  const MaplibreNativeFfiStyleTransitionOptions* self,
  mln_style_transition_options* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_style_transition_option_fields)self->fields;
  out->duration_ms = (double)(self->duration_ms);
  out->delay_ms = (double)(self->delay_ms);
  out->enable_placement_transitions =
    (self->enable_placement_transitions) != FALSE;
}

MaplibreNativeFfiStyleTransitionOptions*
maplibre_native_ffi_style_transition_options_from_native(
  const mln_style_transition_options* in
) {
  MaplibreNativeFfiStyleTransitionOptions* self =
    maplibre_native_ffi_style_transition_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_STYLE_TRANSITION_OPTION_DURATION) != 0) {
    self->duration_ms = (gdouble)(in->duration_ms);
  }
  if ((in->fields & MLN_STYLE_TRANSITION_OPTION_DELAY) != 0) {
    self->delay_ms = (gdouble)(in->delay_ms);
  }
  if (
    (in->fields & MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS) != 0
  ) {
    self->enable_placement_transitions =
      (in->enable_placement_transitions) ? TRUE : FALSE;
  }
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiTextureImageInfo, maplibre_native_ffi_texture_image_info,
  maplibre_native_ffi_texture_image_info_copy,
  maplibre_native_ffi_texture_image_info_free
)

MaplibreNativeFfiTextureImageInfo* maplibre_native_ffi_texture_image_info_copy(
  const MaplibreNativeFfiTextureImageInfo* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_texture_image_info_free(
  MaplibreNativeFfiTextureImageInfo* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_texture_image_info_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_texture_image_info_copy(
    (const MaplibreNativeFfiTextureImageInfo*)self
  );
}

void maplibre_native_ffi_texture_image_info_to_native(
  const MaplibreNativeFfiTextureImageInfo* self, mln_texture_image_info* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->width = (uint32_t)(self->width);
  out->height = (uint32_t)(self->height);
  out->stride = (uint32_t)(self->stride);
  out->byte_length = (size_t)(self->byte_length);
}

void maplibre_native_ffi_texture_image_info_read(
  MaplibreNativeFfiTextureImageInfo* out, const mln_texture_image_info* in
) {
  out->width = (guint32)(in->width);
  out->height = (guint32)(in->height);
  out->stride = (guint32)(in->stride);
  out->byte_length = (gsize)(in->byte_length);
}

MaplibreNativeFfiTextureImageInfo*
maplibre_native_ffi_texture_image_info_from_native(
  const mln_texture_image_info* in
) {
  MaplibreNativeFfiTextureImageInfo* self =
    g_new0(MaplibreNativeFfiTextureImageInfo, 1);
  maplibre_native_ffi_texture_image_info_read(self, in);
  return self;
}

struct _MaplibreNativeFfiTextureReadbackResult {
  GBytes* data;
  MaplibreNativeFfiTextureImageInfo info;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiTextureReadbackResult,
  maplibre_native_ffi_texture_readback_result,
  maplibre_native_ffi_texture_readback_result_copy,
  maplibre_native_ffi_texture_readback_result_free
)

MaplibreNativeFfiTextureReadbackResult*
maplibre_native_ffi_texture_readback_result_new(void) {
  MaplibreNativeFfiTextureReadbackResult* self =
    g_new0(MaplibreNativeFfiTextureReadbackResult, 1);
  return self;
}

MaplibreNativeFfiTextureReadbackResult*
maplibre_native_ffi_texture_readback_result_copy(
  const MaplibreNativeFfiTextureReadbackResult* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiTextureReadbackResult* copy = g_memdup2(self, sizeof *self);
  copy->data = self->data != NULL ? g_bytes_ref(self->data) : NULL;
  return copy;
}

void maplibre_native_ffi_texture_readback_result_free(
  MaplibreNativeFfiTextureReadbackResult* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->data, g_bytes_unref);
  g_free(self);
}

GBytes* maplibre_native_ffi_texture_readback_result_get_data(
  const MaplibreNativeFfiTextureReadbackResult* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->data;
}

MaplibreNativeFfiTextureImageInfo*
maplibre_native_ffi_texture_readback_result_get_info(
  const MaplibreNativeFfiTextureReadbackResult* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiTextureImageInfo*)&self->info;
}

gpointer maplibre_native_ffi_texture_readback_result_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_texture_readback_result_copy(
    (const MaplibreNativeFfiTextureReadbackResult*)self
  );
}

void maplibre_native_ffi_texture_readback_result_to_native(
  const MaplibreNativeFfiTextureReadbackResult* self,
  mln_texture_readback_result* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->data = maplibre_native_ffi_bytes_view(self->data);
  maplibre_native_ffi_texture_image_info_to_native(
    &self->info, &out->info, call
  );
}

MaplibreNativeFfiTextureReadbackResult*
maplibre_native_ffi_texture_readback_result_from_native(
  const mln_texture_readback_result* in
) {
  MaplibreNativeFfiTextureReadbackResult* self =
    maplibre_native_ffi_texture_readback_result_new();
  g_clear_pointer(&self->data, g_bytes_unref);
  self->data = maplibre_native_ffi_bytes_new(in->data.data, in->data.size);
  maplibre_native_ffi_texture_image_info_read(&self->info, &in->info);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiVulkanBorrowedTexture,
  maplibre_native_ffi_vulkan_borrowed_texture,
  maplibre_native_ffi_vulkan_borrowed_texture_copy,
  maplibre_native_ffi_vulkan_borrowed_texture_free
)

MaplibreNativeFfiVulkanBorrowedTexture*
maplibre_native_ffi_vulkan_borrowed_texture_copy(
  const MaplibreNativeFfiVulkanBorrowedTexture* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_vulkan_borrowed_texture_free(
  MaplibreNativeFfiVulkanBorrowedTexture* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_vulkan_borrowed_texture_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_vulkan_borrowed_texture_copy(
    (const MaplibreNativeFfiVulkanBorrowedTexture*)self
  );
}

void maplibre_native_ffi_vulkan_borrowed_texture_to_native(
  const MaplibreNativeFfiVulkanBorrowedTexture* self,
  mln_vulkan_borrowed_texture* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->image = (mln_vulkan_non_dispatchable_handle)(self->image);
  out->image_view = (mln_vulkan_non_dispatchable_handle)(self->image_view);
}

void maplibre_native_ffi_vulkan_borrowed_texture_read(
  MaplibreNativeFfiVulkanBorrowedTexture* out,
  const mln_vulkan_borrowed_texture* in
) {
  out->image = (guint64)(in->image);
  out->image_view = (guint64)(in->image_view);
}

MaplibreNativeFfiVulkanBorrowedTexture*
maplibre_native_ffi_vulkan_borrowed_texture_from_native(
  const mln_vulkan_borrowed_texture* in
) {
  MaplibreNativeFfiVulkanBorrowedTexture* self =
    g_new0(MaplibreNativeFfiVulkanBorrowedTexture, 1);
  maplibre_native_ffi_vulkan_borrowed_texture_read(self, in);
  return self;
}

struct _MaplibreNativeFfiVulkanContextDescriptor {
  gpointer instance;
  gpointer physical_device;
  gpointer device;
  gpointer graphics_queue;
  guint32 graphics_queue_family_index;
  gpointer get_instance_proc_addr;
  gpointer get_device_proc_addr;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiVulkanContextDescriptor,
  maplibre_native_ffi_vulkan_context_descriptor,
  maplibre_native_ffi_vulkan_context_descriptor_copy,
  maplibre_native_ffi_vulkan_context_descriptor_free
)

MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_context_descriptor_new(void) {
  MaplibreNativeFfiVulkanContextDescriptor* self =
    g_new0(MaplibreNativeFfiVulkanContextDescriptor, 1);
  return self;
}

MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_context_descriptor_copy(
  const MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiVulkanContextDescriptor* copy =
    g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_vulkan_context_descriptor_free(
  MaplibreNativeFfiVulkanContextDescriptor* self
) {
  if (self == NULL) return;
  g_free(self);
}

gpointer maplibre_native_ffi_vulkan_context_descriptor_get_instance(
  const MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->instance;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_instance(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->instance = value;
}

gpointer maplibre_native_ffi_vulkan_context_descriptor_get_physical_device(
  const MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->physical_device;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_physical_device(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->physical_device = value;
}

gpointer maplibre_native_ffi_vulkan_context_descriptor_get_device(
  const MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->device;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_device(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->device = value;
}

gpointer maplibre_native_ffi_vulkan_context_descriptor_get_graphics_queue(
  const MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->graphics_queue;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->graphics_queue = value;
}

guint32
maplibre_native_ffi_vulkan_context_descriptor_get_graphics_queue_family_index(
  const MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->graphics_queue_family_index;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue_family_index(
  MaplibreNativeFfiVulkanContextDescriptor* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->graphics_queue_family_index = value;
}

gpointer
maplibre_native_ffi_vulkan_context_descriptor_get_get_instance_proc_addr(
  const MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->get_instance_proc_addr;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_get_instance_proc_addr(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->get_instance_proc_addr = value;
}

gpointer maplibre_native_ffi_vulkan_context_descriptor_get_get_device_proc_addr(
  const MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->get_device_proc_addr;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_get_device_proc_addr(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->get_device_proc_addr = value;
}

gpointer maplibre_native_ffi_vulkan_context_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_vulkan_context_descriptor_copy(
    (const MaplibreNativeFfiVulkanContextDescriptor*)self
  );
}

void maplibre_native_ffi_vulkan_context_descriptor_to_native(
  const MaplibreNativeFfiVulkanContextDescriptor* self,
  mln_vulkan_context_descriptor* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->instance = (void*)(self->instance);
  out->physical_device = (void*)(self->physical_device);
  out->device = (void*)(self->device);
  out->graphics_queue = (void*)(self->graphics_queue);
  out->graphics_queue_family_index =
    (uint32_t)(self->graphics_queue_family_index);
  out->get_instance_proc_addr = (void*)(self->get_instance_proc_addr);
  out->get_device_proc_addr = (void*)(self->get_device_proc_addr);
}

MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_context_descriptor_from_native(
  const mln_vulkan_context_descriptor* in
) {
  MaplibreNativeFfiVulkanContextDescriptor* self =
    maplibre_native_ffi_vulkan_context_descriptor_new();
  self->instance = (gpointer)(in->instance);
  self->physical_device = (gpointer)(in->physical_device);
  self->device = (gpointer)(in->device);
  self->graphics_queue = (gpointer)(in->graphics_queue);
  self->graphics_queue_family_index =
    (guint32)(in->graphics_queue_family_index);
  self->get_instance_proc_addr = (gpointer)(in->get_instance_proc_addr);
  self->get_device_proc_addr = (gpointer)(in->get_device_proc_addr);
  return self;
}

struct _MaplibreNativeFfiVulkanBorrowedTextureDescriptor {
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 physical_width;
  guint32 physical_height;
  MaplibreNativeFfiVulkanContextDescriptor* context;
  GPtrArray* textures;
  guint32 format;
  guint32 final_layout;
  MaplibreNativeFfiVulkanImageOwner owner;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor,
  maplibre_native_ffi_vulkan_borrowed_texture_descriptor,
  maplibre_native_ffi_vulkan_borrowed_texture_descriptor_copy,
  maplibre_native_ffi_vulkan_borrowed_texture_descriptor_free
)

MaplibreNativeFfiVulkanBorrowedTextureDescriptor*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_new(void) {
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self =
    g_new0(MaplibreNativeFfiVulkanBorrowedTextureDescriptor, 1);
  self->context = maplibre_native_ffi_vulkan_context_descriptor_new();
  self->textures = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_vulkan_borrowed_texture_free
  );
  return self;
}

MaplibreNativeFfiVulkanBorrowedTextureDescriptor*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_copy(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* copy =
    g_memdup2(self, sizeof *self);
  copy->context =
    self->context != NULL
      ? maplibre_native_ffi_vulkan_context_descriptor_copy(self->context)
      : NULL;
  copy->textures =
    self->textures != NULL
      ? g_ptr_array_copy(
          self->textures, maplibre_native_ffi_vulkan_borrowed_texture_copy_func,
          NULL
        )
      : NULL;
  if (copy->textures != NULL)
    g_ptr_array_set_free_func(
      copy->textures,
      (GDestroyNotify)maplibre_native_ffi_vulkan_borrowed_texture_free
    );
  return copy;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_free(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  if (self == NULL) return;
  g_clear_pointer(
    &self->context, maplibre_native_ffi_vulkan_context_descriptor_free
  );
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_logical_size(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_logical_size(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

guint32
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_physical_width(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->physical_width;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_width(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->physical_width = value;
}

guint32
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_physical_height(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->physical_height;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_height(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->physical_height = value;
}

MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_context(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->context;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_context(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiVulkanContextDescriptor* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiVulkanContextDescriptor* previous = self->context;
  self->context = maplibre_native_ffi_vulkan_context_descriptor_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_vulkan_context_descriptor_free(previous);
}

GPtrArray* maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_textures(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->textures;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_textures(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiVulkanBorrowedTexture* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->textures;
  self->textures = g_ptr_array_new_full(
    (guint)value_count,
    (GDestroyNotify)maplibre_native_ffi_vulkan_borrowed_texture_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->textures,
      maplibre_native_ffi_vulkan_borrowed_texture_copy(&value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

guint32 maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_format(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->format;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_format(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->format = value;
}

guint32 maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_final_layout(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->final_layout;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_final_layout(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->final_layout = value;
}

MaplibreNativeFfiVulkanImageOwner
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_owner(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(self != NULL, (MaplibreNativeFfiVulkanImageOwner)0);
  return self->owner;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_owner(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  MaplibreNativeFfiVulkanImageOwner value
) {
  g_return_if_fail(self != NULL);
  self->owner = value;
}

gpointer maplibre_native_ffi_vulkan_borrowed_texture_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_vulkan_borrowed_texture_descriptor_copy(
    (const MaplibreNativeFfiVulkanBorrowedTextureDescriptor*)self
  );
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_to_native(
  const MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  mln_vulkan_borrowed_texture_descriptor* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  out->physical_width = (uint32_t)(self->physical_width);
  out->physical_height = (uint32_t)(self->physical_height);
  if (self->context != NULL)
    maplibre_native_ffi_vulkan_context_descriptor_to_native(
      self->context, &out->context, call
    );
  if (self->textures != NULL && self->textures->len > 0) {
    mln_vulkan_borrowed_texture* items =
      maplibre_native_ffi_call_alloc(call, self->textures->len, sizeof *items);
    for (guint index = 0; index < self->textures->len; index++) {
      maplibre_native_ffi_vulkan_borrowed_texture_to_native(
        self->textures->pdata[index], &items[index], call
      );
    }
    out->textures = items;
    out->texture_count = self->textures->len;
  }
  out->format = (uint32_t)(self->format);
  out->final_layout = (uint32_t)(self->final_layout);
  out->owner = (mln_vulkan_image_owner)(self->owner);
}

MaplibreNativeFfiVulkanBorrowedTextureDescriptor*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_from_native(
  const mln_vulkan_borrowed_texture_descriptor* in
) {
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self =
    maplibre_native_ffi_vulkan_borrowed_texture_descriptor_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  self->physical_width = (guint32)(in->physical_width);
  self->physical_height = (guint32)(in->physical_height);
  g_clear_pointer(
    &self->context, maplibre_native_ffi_vulkan_context_descriptor_free
  );
  self->context =
    maplibre_native_ffi_vulkan_context_descriptor_from_native(&in->context);
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  self->textures = g_ptr_array_new_full(
    (guint)in->texture_count,
    (GDestroyNotify)maplibre_native_ffi_vulkan_borrowed_texture_free
  );
  for (size_t index = 0; index < in->texture_count; index++) {
    g_ptr_array_add(
      self->textures, maplibre_native_ffi_vulkan_borrowed_texture_from_native(
                        &in->textures[index]
                      )
    );
  }
  self->format = (guint32)(in->format);
  self->final_layout = (guint32)(in->final_layout);
  self->owner = (MaplibreNativeFfiVulkanImageOwner)(in->owner);
  return self;
}

struct _MaplibreNativeFfiVulkanBorrowedTextureReplacement {
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 physical_width;
  guint32 physical_height;
  GPtrArray* textures;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement,
  maplibre_native_ffi_vulkan_borrowed_texture_replacement,
  maplibre_native_ffi_vulkan_borrowed_texture_replacement_copy,
  maplibre_native_ffi_vulkan_borrowed_texture_replacement_free
)

MaplibreNativeFfiVulkanBorrowedTextureReplacement*
maplibre_native_ffi_vulkan_borrowed_texture_replacement_new(void) {
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self =
    g_new0(MaplibreNativeFfiVulkanBorrowedTextureReplacement, 1);
  self->textures = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_vulkan_borrowed_texture_free
  );
  return self;
}

MaplibreNativeFfiVulkanBorrowedTextureReplacement*
maplibre_native_ffi_vulkan_borrowed_texture_replacement_copy(
  const MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* copy =
    g_memdup2(self, sizeof *self);
  copy->textures =
    self->textures != NULL
      ? g_ptr_array_copy(
          self->textures, maplibre_native_ffi_vulkan_borrowed_texture_copy_func,
          NULL
        )
      : NULL;
  if (copy->textures != NULL)
    g_ptr_array_set_free_func(
      copy->textures,
      (GDestroyNotify)maplibre_native_ffi_vulkan_borrowed_texture_free
    );
  return copy;
}

void maplibre_native_ffi_vulkan_borrowed_texture_replacement_free(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_logical_size(
  const MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_logical_size(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

guint32
maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_physical_width(
  const MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->physical_width;
}

void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_width(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->physical_width = value;
}

guint32
maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_physical_height(
  const MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->physical_height;
}

void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_height(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self, guint32 value
) {
  g_return_if_fail(self != NULL);
  self->physical_height = value;
}

GPtrArray* maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_textures(
  const MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->textures;
}

void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_textures(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self,
  const MaplibreNativeFfiVulkanBorrowedTexture* value, gsize value_count
) {
  g_return_if_fail(self != NULL);
  GPtrArray* previous = self->textures;
  self->textures = g_ptr_array_new_full(
    (guint)value_count,
    (GDestroyNotify)maplibre_native_ffi_vulkan_borrowed_texture_free
  );
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->textures,
      maplibre_native_ffi_vulkan_borrowed_texture_copy(&value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

gpointer maplibre_native_ffi_vulkan_borrowed_texture_replacement_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_vulkan_borrowed_texture_replacement_copy(
    (const MaplibreNativeFfiVulkanBorrowedTextureReplacement*)self
  );
}

void maplibre_native_ffi_vulkan_borrowed_texture_replacement_to_native(
  const MaplibreNativeFfiVulkanBorrowedTextureReplacement* self,
  mln_vulkan_borrowed_texture_replacement* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  out->physical_width = (uint32_t)(self->physical_width);
  out->physical_height = (uint32_t)(self->physical_height);
  if (self->textures != NULL && self->textures->len > 0) {
    mln_vulkan_borrowed_texture* items =
      maplibre_native_ffi_call_alloc(call, self->textures->len, sizeof *items);
    for (guint index = 0; index < self->textures->len; index++) {
      maplibre_native_ffi_vulkan_borrowed_texture_to_native(
        self->textures->pdata[index], &items[index], call
      );
    }
    out->textures = items;
    out->texture_count = self->textures->len;
  }
}

MaplibreNativeFfiVulkanBorrowedTextureReplacement*
maplibre_native_ffi_vulkan_borrowed_texture_replacement_from_native(
  const mln_vulkan_borrowed_texture_replacement* in
) {
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self =
    maplibre_native_ffi_vulkan_borrowed_texture_replacement_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  self->physical_width = (guint32)(in->physical_width);
  self->physical_height = (guint32)(in->physical_height);
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  self->textures = g_ptr_array_new_full(
    (guint)in->texture_count,
    (GDestroyNotify)maplibre_native_ffi_vulkan_borrowed_texture_free
  );
  for (size_t index = 0; index < in->texture_count; index++) {
    g_ptr_array_add(
      self->textures, maplibre_native_ffi_vulkan_borrowed_texture_from_native(
                        &in->textures[index]
                      )
    );
  }
  return self;
}

struct _MaplibreNativeFfiVulkanDeviceContextResult {
  MaplibreNativeFfiVulkanContextDescriptor* context;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiVulkanDeviceContextResult,
  maplibre_native_ffi_vulkan_device_context_result,
  maplibre_native_ffi_vulkan_device_context_result_copy,
  maplibre_native_ffi_vulkan_device_context_result_free
)

MaplibreNativeFfiVulkanDeviceContextResult*
maplibre_native_ffi_vulkan_device_context_result_new(void) {
  MaplibreNativeFfiVulkanDeviceContextResult* self =
    g_new0(MaplibreNativeFfiVulkanDeviceContextResult, 1);
  self->context = maplibre_native_ffi_vulkan_context_descriptor_new();
  return self;
}

MaplibreNativeFfiVulkanDeviceContextResult*
maplibre_native_ffi_vulkan_device_context_result_copy(
  const MaplibreNativeFfiVulkanDeviceContextResult* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiVulkanDeviceContextResult* copy =
    g_memdup2(self, sizeof *self);
  copy->context =
    self->context != NULL
      ? maplibre_native_ffi_vulkan_context_descriptor_copy(self->context)
      : NULL;
  return copy;
}

void maplibre_native_ffi_vulkan_device_context_result_free(
  MaplibreNativeFfiVulkanDeviceContextResult* self
) {
  if (self == NULL) return;
  g_clear_pointer(
    &self->context, maplibre_native_ffi_vulkan_context_descriptor_free
  );
  g_free(self);
}

MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_device_context_result_get_context(
  const MaplibreNativeFfiVulkanDeviceContextResult* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->context;
}

gpointer maplibre_native_ffi_vulkan_device_context_result_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_vulkan_device_context_result_copy(
    (const MaplibreNativeFfiVulkanDeviceContextResult*)self
  );
}

void maplibre_native_ffi_vulkan_device_context_result_to_native(
  const MaplibreNativeFfiVulkanDeviceContextResult* self,
  mln_vulkan_device_context_result* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  if (self->context != NULL)
    maplibre_native_ffi_vulkan_context_descriptor_to_native(
      self->context, &out->context, call
    );
}

MaplibreNativeFfiVulkanDeviceContextResult*
maplibre_native_ffi_vulkan_device_context_result_from_native(
  const mln_vulkan_device_context_result* in
) {
  MaplibreNativeFfiVulkanDeviceContextResult* self =
    maplibre_native_ffi_vulkan_device_context_result_new();
  g_clear_pointer(
    &self->context, maplibre_native_ffi_vulkan_context_descriptor_free
  );
  self->context =
    maplibre_native_ffi_vulkan_context_descriptor_from_native(&in->context);
  return self;
}

struct _MaplibreNativeFfiVulkanDeviceOptions {
  guint64 fields;
  gboolean presentation;
  GBytes* device_uuid;
  GBytes* device_luid;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiVulkanDeviceOptions,
  maplibre_native_ffi_vulkan_device_options,
  maplibre_native_ffi_vulkan_device_options_copy,
  maplibre_native_ffi_vulkan_device_options_free
)

MaplibreNativeFfiVulkanDeviceOptions*
maplibre_native_ffi_vulkan_device_options_new(void) {
  MaplibreNativeFfiVulkanDeviceOptions* self =
    g_new0(MaplibreNativeFfiVulkanDeviceOptions, 1);
  return self;
}

MaplibreNativeFfiVulkanDeviceOptions*
maplibre_native_ffi_vulkan_device_options_copy(
  const MaplibreNativeFfiVulkanDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiVulkanDeviceOptions* copy = g_memdup2(self, sizeof *self);
  copy->device_uuid =
    self->device_uuid != NULL ? g_bytes_ref(self->device_uuid) : NULL;
  copy->device_luid =
    self->device_luid != NULL ? g_bytes_ref(self->device_luid) : NULL;
  return copy;
}

void maplibre_native_ffi_vulkan_device_options_free(
  MaplibreNativeFfiVulkanDeviceOptions* self
) {
  if (self == NULL) return;
  g_clear_pointer(&self->device_uuid, g_bytes_unref);
  g_clear_pointer(&self->device_luid, g_bytes_unref);
  g_free(self);
}

gboolean maplibre_native_ffi_vulkan_device_options_get_presentation(
  const MaplibreNativeFfiVulkanDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return self->presentation;
}

gboolean maplibre_native_ffi_vulkan_device_options_has_presentation(
  const MaplibreNativeFfiVulkanDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_VULKAN_DEVICE_OPTION_PRESENTATION) != 0;
}

void maplibre_native_ffi_vulkan_device_options_set_presentation(
  MaplibreNativeFfiVulkanDeviceOptions* self, gboolean value
) {
  g_return_if_fail(self != NULL);
  self->presentation = value;
  self->fields |= MLN_VULKAN_DEVICE_OPTION_PRESENTATION;
}

GBytes* maplibre_native_ffi_vulkan_device_options_get_device_uuid(
  const MaplibreNativeFfiVulkanDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->device_uuid;
}

gboolean maplibre_native_ffi_vulkan_device_options_has_device_uuid(
  const MaplibreNativeFfiVulkanDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_VULKAN_DEVICE_OPTION_DEVICE_UUID) != 0;
}

void maplibre_native_ffi_vulkan_device_options_set_device_uuid(
  MaplibreNativeFfiVulkanDeviceOptions* self, GBytes* value
) {
  g_return_if_fail(self != NULL);
  GBytes* previous = self->device_uuid;
  self->device_uuid = value != NULL ? g_bytes_ref(value) : NULL;
  if (previous != NULL) g_bytes_unref(previous);
  self->fields |= MLN_VULKAN_DEVICE_OPTION_DEVICE_UUID;
}

GBytes* maplibre_native_ffi_vulkan_device_options_get_device_luid(
  const MaplibreNativeFfiVulkanDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->device_luid;
}

gboolean maplibre_native_ffi_vulkan_device_options_has_device_luid(
  const MaplibreNativeFfiVulkanDeviceOptions* self
) {
  g_return_val_if_fail(self != NULL, FALSE);
  return (self->fields & MLN_VULKAN_DEVICE_OPTION_DEVICE_LUID) != 0;
}

void maplibre_native_ffi_vulkan_device_options_set_device_luid(
  MaplibreNativeFfiVulkanDeviceOptions* self, GBytes* value
) {
  g_return_if_fail(self != NULL);
  GBytes* previous = self->device_luid;
  self->device_luid = value != NULL ? g_bytes_ref(value) : NULL;
  if (previous != NULL) g_bytes_unref(previous);
  self->fields |= MLN_VULKAN_DEVICE_OPTION_DEVICE_LUID;
}

gpointer maplibre_native_ffi_vulkan_device_options_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_vulkan_device_options_copy(
    (const MaplibreNativeFfiVulkanDeviceOptions*)self
  );
}

void maplibre_native_ffi_vulkan_device_options_to_native(
  const MaplibreNativeFfiVulkanDeviceOptions* self,
  mln_vulkan_device_options* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->fields = (mln_vulkan_device_option_fields)self->fields;
  out->presentation = (self->presentation) != FALSE;
  out->device_uuid = maplibre_native_ffi_bytes_view(self->device_uuid);
  out->device_luid = maplibre_native_ffi_bytes_view(self->device_luid);
}

MaplibreNativeFfiVulkanDeviceOptions*
maplibre_native_ffi_vulkan_device_options_from_native(
  const mln_vulkan_device_options* in
) {
  MaplibreNativeFfiVulkanDeviceOptions* self =
    maplibre_native_ffi_vulkan_device_options_new();
  self->fields = (guint64)in->fields;
  if ((in->fields & MLN_VULKAN_DEVICE_OPTION_PRESENTATION) != 0) {
    self->presentation = (in->presentation) ? TRUE : FALSE;
  }
  if ((in->fields & MLN_VULKAN_DEVICE_OPTION_DEVICE_UUID) != 0) {
    g_clear_pointer(&self->device_uuid, g_bytes_unref);
    self->device_uuid =
      maplibre_native_ffi_bytes_new(in->device_uuid.data, in->device_uuid.size);
  }
  if ((in->fields & MLN_VULKAN_DEVICE_OPTION_DEVICE_LUID) != 0) {
    g_clear_pointer(&self->device_luid, g_bytes_unref);
    self->device_luid =
      maplibre_native_ffi_bytes_new(in->device_luid.data, in->device_luid.size);
  }
  return self;
}

struct _MaplibreNativeFfiVulkanSurfaceDescriptor {
  MaplibreNativeFfiLogicalSize logical_size;
  MaplibreNativeFfiVulkanContextDescriptor* context;
  guint64 surface;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiVulkanSurfaceDescriptor,
  maplibre_native_ffi_vulkan_surface_descriptor,
  maplibre_native_ffi_vulkan_surface_descriptor_copy,
  maplibre_native_ffi_vulkan_surface_descriptor_free
)

MaplibreNativeFfiVulkanSurfaceDescriptor*
maplibre_native_ffi_vulkan_surface_descriptor_new(void) {
  MaplibreNativeFfiVulkanSurfaceDescriptor* self =
    g_new0(MaplibreNativeFfiVulkanSurfaceDescriptor, 1);
  self->context = maplibre_native_ffi_vulkan_context_descriptor_new();
  return self;
}

MaplibreNativeFfiVulkanSurfaceDescriptor*
maplibre_native_ffi_vulkan_surface_descriptor_copy(
  const MaplibreNativeFfiVulkanSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiVulkanSurfaceDescriptor* copy =
    g_memdup2(self, sizeof *self);
  copy->context =
    self->context != NULL
      ? maplibre_native_ffi_vulkan_context_descriptor_copy(self->context)
      : NULL;
  return copy;
}

void maplibre_native_ffi_vulkan_surface_descriptor_free(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self
) {
  if (self == NULL) return;
  g_clear_pointer(
    &self->context, maplibre_native_ffi_vulkan_context_descriptor_free
  );
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_vulkan_surface_descriptor_get_logical_size(
  const MaplibreNativeFfiVulkanSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_vulkan_surface_descriptor_set_logical_size(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_surface_descriptor_get_context(
  const MaplibreNativeFfiVulkanSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->context;
}

void maplibre_native_ffi_vulkan_surface_descriptor_set_context(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self,
  const MaplibreNativeFfiVulkanContextDescriptor* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiVulkanContextDescriptor* previous = self->context;
  self->context = maplibre_native_ffi_vulkan_context_descriptor_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_vulkan_context_descriptor_free(previous);
}

guint64 maplibre_native_ffi_vulkan_surface_descriptor_get_surface(
  const MaplibreNativeFfiVulkanSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->surface;
}

void maplibre_native_ffi_vulkan_surface_descriptor_set_surface(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self, guint64 value
) {
  g_return_if_fail(self != NULL);
  self->surface = value;
}

gpointer maplibre_native_ffi_vulkan_surface_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_vulkan_surface_descriptor_copy(
    (const MaplibreNativeFfiVulkanSurfaceDescriptor*)self
  );
}

void maplibre_native_ffi_vulkan_surface_descriptor_to_native(
  const MaplibreNativeFfiVulkanSurfaceDescriptor* self,
  mln_vulkan_surface_descriptor* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  if (self->context != NULL)
    maplibre_native_ffi_vulkan_context_descriptor_to_native(
      self->context, &out->context, call
    );
  out->surface = (mln_vulkan_non_dispatchable_handle)(self->surface);
}

MaplibreNativeFfiVulkanSurfaceDescriptor*
maplibre_native_ffi_vulkan_surface_descriptor_from_native(
  const mln_vulkan_surface_descriptor* in
) {
  MaplibreNativeFfiVulkanSurfaceDescriptor* self =
    maplibre_native_ffi_vulkan_surface_descriptor_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  g_clear_pointer(
    &self->context, maplibre_native_ffi_vulkan_context_descriptor_free
  );
  self->context =
    maplibre_native_ffi_vulkan_context_descriptor_from_native(&in->context);
  self->surface = (guint64)(in->surface);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiVulkanSurfaceReplacement,
  maplibre_native_ffi_vulkan_surface_replacement,
  maplibre_native_ffi_vulkan_surface_replacement_copy,
  maplibre_native_ffi_vulkan_surface_replacement_free
)

MaplibreNativeFfiVulkanSurfaceReplacement*
maplibre_native_ffi_vulkan_surface_replacement_copy(
  const MaplibreNativeFfiVulkanSurfaceReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_vulkan_surface_replacement_free(
  MaplibreNativeFfiVulkanSurfaceReplacement* self
) {
  g_free(self);
}

gpointer maplibre_native_ffi_vulkan_surface_replacement_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_vulkan_surface_replacement_copy(
    (const MaplibreNativeFfiVulkanSurfaceReplacement*)self
  );
}

void maplibre_native_ffi_vulkan_surface_replacement_to_native(
  const MaplibreNativeFfiVulkanSurfaceReplacement* self,
  mln_vulkan_surface_replacement* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  out->surface = (mln_vulkan_non_dispatchable_handle)(self->surface);
}

void maplibre_native_ffi_vulkan_surface_replacement_read(
  MaplibreNativeFfiVulkanSurfaceReplacement* out,
  const mln_vulkan_surface_replacement* in
) {
  maplibre_native_ffi_logical_size_read(&out->logical_size, &in->logical_size);
  out->surface = (guint64)(in->surface);
}

MaplibreNativeFfiVulkanSurfaceReplacement*
maplibre_native_ffi_vulkan_surface_replacement_from_native(
  const mln_vulkan_surface_replacement* in
) {
  MaplibreNativeFfiVulkanSurfaceReplacement* self =
    g_new0(MaplibreNativeFfiVulkanSurfaceReplacement, 1);
  maplibre_native_ffi_vulkan_surface_replacement_read(self, in);
  return self;
}

struct _MaplibreNativeFfiVulkanTextureFrame {
  guint32 width;
  guint32 height;
  gdouble scale_factor;
  guint32 slot;
  guint64 image;
  guint64 image_view;
  gpointer device;
  guint32 format;
  guint32 layout;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiVulkanTextureFrame, maplibre_native_ffi_vulkan_texture_frame,
  maplibre_native_ffi_vulkan_texture_frame_copy,
  maplibre_native_ffi_vulkan_texture_frame_free
)

MaplibreNativeFfiVulkanTextureFrame*
maplibre_native_ffi_vulkan_texture_frame_new(void) {
  MaplibreNativeFfiVulkanTextureFrame* self =
    g_new0(MaplibreNativeFfiVulkanTextureFrame, 1);
  return self;
}

MaplibreNativeFfiVulkanTextureFrame*
maplibre_native_ffi_vulkan_texture_frame_copy(
  const MaplibreNativeFfiVulkanTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiVulkanTextureFrame* copy = g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_vulkan_texture_frame_free(
  MaplibreNativeFfiVulkanTextureFrame* self
) {
  if (self == NULL) return;
  g_free(self);
}

guint32 maplibre_native_ffi_vulkan_texture_frame_get_width(
  const MaplibreNativeFfiVulkanTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->width;
}

guint32 maplibre_native_ffi_vulkan_texture_frame_get_height(
  const MaplibreNativeFfiVulkanTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->height;
}

gdouble maplibre_native_ffi_vulkan_texture_frame_get_scale_factor(
  const MaplibreNativeFfiVulkanTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->scale_factor;
}

guint32 maplibre_native_ffi_vulkan_texture_frame_get_slot(
  const MaplibreNativeFfiVulkanTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->slot;
}

guint64 maplibre_native_ffi_vulkan_texture_frame_get_image(
  const MaplibreNativeFfiVulkanTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->image;
}

guint64 maplibre_native_ffi_vulkan_texture_frame_get_image_view(
  const MaplibreNativeFfiVulkanTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->image_view;
}

gpointer maplibre_native_ffi_vulkan_texture_frame_get_device(
  const MaplibreNativeFfiVulkanTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->device;
}

guint32 maplibre_native_ffi_vulkan_texture_frame_get_format(
  const MaplibreNativeFfiVulkanTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->format;
}

guint32 maplibre_native_ffi_vulkan_texture_frame_get_layout(
  const MaplibreNativeFfiVulkanTextureFrame* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->layout;
}

gpointer maplibre_native_ffi_vulkan_texture_frame_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_vulkan_texture_frame_copy(
    (const MaplibreNativeFfiVulkanTextureFrame*)self
  );
}

void maplibre_native_ffi_vulkan_texture_frame_to_native(
  const MaplibreNativeFfiVulkanTextureFrame* self,
  mln_vulkan_texture_frame* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  out->width = (uint32_t)(self->width);
  out->height = (uint32_t)(self->height);
  out->scale_factor = (double)(self->scale_factor);
  out->slot = (uint32_t)(self->slot);
  out->image = (mln_vulkan_non_dispatchable_handle)(self->image);
  out->image_view = (mln_vulkan_non_dispatchable_handle)(self->image_view);
  out->device = (void*)(self->device);
  out->format = (uint32_t)(self->format);
  out->layout = (uint32_t)(self->layout);
}

MaplibreNativeFfiVulkanTextureFrame*
maplibre_native_ffi_vulkan_texture_frame_from_native(
  const mln_vulkan_texture_frame* in
) {
  MaplibreNativeFfiVulkanTextureFrame* self =
    maplibre_native_ffi_vulkan_texture_frame_new();
  self->width = (guint32)(in->width);
  self->height = (guint32)(in->height);
  self->scale_factor = (gdouble)(in->scale_factor);
  self->slot = (guint32)(in->slot);
  self->image = (guint64)(in->image);
  self->image_view = (guint64)(in->image_view);
  self->device = (gpointer)(in->device);
  self->format = (guint32)(in->format);
  self->layout = (guint32)(in->layout);
  return self;
}

struct _MaplibreNativeFfiWaylandSurfaceDescriptor {
  MaplibreNativeFfiLogicalSize logical_size;
  gpointer display;
  gpointer surface;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiWaylandSurfaceDescriptor,
  maplibre_native_ffi_wayland_surface_descriptor,
  maplibre_native_ffi_wayland_surface_descriptor_copy,
  maplibre_native_ffi_wayland_surface_descriptor_free
)

MaplibreNativeFfiWaylandSurfaceDescriptor*
maplibre_native_ffi_wayland_surface_descriptor_new(void) {
  MaplibreNativeFfiWaylandSurfaceDescriptor* self =
    g_new0(MaplibreNativeFfiWaylandSurfaceDescriptor, 1);
  return self;
}

MaplibreNativeFfiWaylandSurfaceDescriptor*
maplibre_native_ffi_wayland_surface_descriptor_copy(
  const MaplibreNativeFfiWaylandSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiWaylandSurfaceDescriptor* copy =
    g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_wayland_surface_descriptor_free(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self
) {
  if (self == NULL) return;
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_wayland_surface_descriptor_get_logical_size(
  const MaplibreNativeFfiWaylandSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_wayland_surface_descriptor_set_logical_size(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

gpointer maplibre_native_ffi_wayland_surface_descriptor_get_display(
  const MaplibreNativeFfiWaylandSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->display;
}

void maplibre_native_ffi_wayland_surface_descriptor_set_display(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->display = value;
}

gpointer maplibre_native_ffi_wayland_surface_descriptor_get_surface(
  const MaplibreNativeFfiWaylandSurfaceDescriptor* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->surface;
}

void maplibre_native_ffi_wayland_surface_descriptor_set_surface(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->surface = value;
}

gpointer maplibre_native_ffi_wayland_surface_descriptor_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_wayland_surface_descriptor_copy(
    (const MaplibreNativeFfiWaylandSurfaceDescriptor*)self
  );
}

void maplibre_native_ffi_wayland_surface_descriptor_to_native(
  const MaplibreNativeFfiWaylandSurfaceDescriptor* self,
  mln_wayland_surface_descriptor* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  out->display = (void*)(self->display);
  out->surface = (void*)(self->surface);
}

MaplibreNativeFfiWaylandSurfaceDescriptor*
maplibre_native_ffi_wayland_surface_descriptor_from_native(
  const mln_wayland_surface_descriptor* in
) {
  MaplibreNativeFfiWaylandSurfaceDescriptor* self =
    maplibre_native_ffi_wayland_surface_descriptor_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  self->display = (gpointer)(in->display);
  self->surface = (gpointer)(in->surface);
  return self;
}

struct _MaplibreNativeFfiWaylandSurfaceReplacement {
  MaplibreNativeFfiLogicalSize logical_size;
  gpointer surface;
};

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiWaylandSurfaceReplacement,
  maplibre_native_ffi_wayland_surface_replacement,
  maplibre_native_ffi_wayland_surface_replacement_copy,
  maplibre_native_ffi_wayland_surface_replacement_free
)

MaplibreNativeFfiWaylandSurfaceReplacement*
maplibre_native_ffi_wayland_surface_replacement_new(void) {
  MaplibreNativeFfiWaylandSurfaceReplacement* self =
    g_new0(MaplibreNativeFfiWaylandSurfaceReplacement, 1);
  return self;
}

MaplibreNativeFfiWaylandSurfaceReplacement*
maplibre_native_ffi_wayland_surface_replacement_copy(
  const MaplibreNativeFfiWaylandSurfaceReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  MaplibreNativeFfiWaylandSurfaceReplacement* copy =
    g_memdup2(self, sizeof *self);
  return copy;
}

void maplibre_native_ffi_wayland_surface_replacement_free(
  MaplibreNativeFfiWaylandSurfaceReplacement* self
) {
  if (self == NULL) return;
  g_free(self);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_wayland_surface_replacement_get_logical_size(
  const MaplibreNativeFfiWaylandSurfaceReplacement* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_wayland_surface_replacement_set_logical_size(
  MaplibreNativeFfiWaylandSurfaceReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(value != NULL);
  self->logical_size = *value;
}

gpointer maplibre_native_ffi_wayland_surface_replacement_get_surface(
  const MaplibreNativeFfiWaylandSurfaceReplacement* self
) {
  g_return_val_if_fail(self != NULL, 0);
  return self->surface;
}

void maplibre_native_ffi_wayland_surface_replacement_set_surface(
  MaplibreNativeFfiWaylandSurfaceReplacement* self, gpointer value
) {
  g_return_if_fail(self != NULL);
  self->surface = value;
}

gpointer maplibre_native_ffi_wayland_surface_replacement_copy_func(
  gconstpointer self, gpointer data
) {
  (void)data;
  return maplibre_native_ffi_wayland_surface_replacement_copy(
    (const MaplibreNativeFfiWaylandSurfaceReplacement*)self
  );
}

void maplibre_native_ffi_wayland_surface_replacement_to_native(
  const MaplibreNativeFfiWaylandSurfaceReplacement* self,
  mln_wayland_surface_replacement* out, MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->size = sizeof *out;
  maplibre_native_ffi_logical_size_to_native(
    &self->logical_size, &out->logical_size, call
  );
  out->surface = (void*)(self->surface);
}

MaplibreNativeFfiWaylandSurfaceReplacement*
maplibre_native_ffi_wayland_surface_replacement_from_native(
  const mln_wayland_surface_replacement* in
) {
  MaplibreNativeFfiWaylandSurfaceReplacement* self =
    maplibre_native_ffi_wayland_surface_replacement_new();
  maplibre_native_ffi_logical_size_read(&self->logical_size, &in->logical_size);
  self->surface = (gpointer)(in->surface);
  return self;
}
