// Generated from the C headers by tools/bindgen. Do not edit.
#include <maplibre_native_c.h>
#include <math.h>
#include <string.h>

#include "maplibre-native-ffi-values.h"

#include "maplibre-native-ffi-handles-private.h"
#include "maplibre-native-ffi-private.h"
#include "maplibre-native-ffi-values-private.h"

// An option bag's members start unset, so no property is
// G_PARAM_CONSTRUCT, and each notifies only when its setter changes it.
#define PROPERTY_FLAGS \
  (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS)

// Whether a struct's member has the offset and the size of the C record's.
#define SAME_MEMBER(type, record, member)                      \
  G_STATIC_ASSERT(                                             \
    offsetof(type, member) == offsetof(record, member) &&      \
    sizeof(((type*)0)->member) == sizeof(((record*)0)->member) \
  )

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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiScreenPoint) == sizeof(mln_screen_point)
);
SAME_MEMBER(MaplibreNativeFfiScreenPoint, mln_screen_point, x);
SAME_MEMBER(MaplibreNativeFfiScreenPoint, mln_screen_point, y);

void maplibre_native_ffi_screen_point_to_native(
  const MaplibreNativeFfiScreenPoint* self, mln_screen_point* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_screen_point_read(
  MaplibreNativeFfiScreenPoint* out, const mln_screen_point* in
) {
  memcpy(out, in, sizeof *out);
}

MaplibreNativeFfiScreenPoint* maplibre_native_ffi_screen_point_from_native(
  const mln_screen_point* in
) {
  MaplibreNativeFfiScreenPoint* self = g_new0(MaplibreNativeFfiScreenPoint, 1);
  maplibre_native_ffi_screen_point_read(self, in);
  return self;
}

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

G_STATIC_ASSERT(sizeof(MaplibreNativeFfiUnitBezier) == sizeof(mln_unit_bezier));
SAME_MEMBER(MaplibreNativeFfiUnitBezier, mln_unit_bezier, x1);
SAME_MEMBER(MaplibreNativeFfiUnitBezier, mln_unit_bezier, y1);
SAME_MEMBER(MaplibreNativeFfiUnitBezier, mln_unit_bezier, x2);
SAME_MEMBER(MaplibreNativeFfiUnitBezier, mln_unit_bezier, y2);

void maplibre_native_ffi_unit_bezier_to_native(
  const MaplibreNativeFfiUnitBezier* self, mln_unit_bezier* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

struct _MaplibreNativeFfiMomentumDecay {
  GObject parent_instance;
  guint64 fields;
  MaplibreNativeFfiMomentumDecayModel model;
  gdouble half_life_ms;
  gdouble stop_speed;
  gdouble distance;
  gdouble duration_ms;
  MaplibreNativeFfiUnitBezier easing;
  gdouble min_speed;
  gdouble max_speed;
  gdouble max_distance;
};

enum {
  MOMENTUM_DECAY_PROP_0,
  MOMENTUM_DECAY_PROP_MODEL,
  MOMENTUM_DECAY_PROP_HALF_LIFE_MS,
  MOMENTUM_DECAY_PROP_STOP_SPEED,
  MOMENTUM_DECAY_PROP_DISTANCE,
  MOMENTUM_DECAY_PROP_DURATION_MS,
  MOMENTUM_DECAY_PROP_EASING,
  MOMENTUM_DECAY_PROP_MIN_SPEED,
  MOMENTUM_DECAY_PROP_MAX_SPEED,
  MOMENTUM_DECAY_PROP_MAX_DISTANCE,
  MOMENTUM_DECAY_N_PROPERTIES
};

static GParamSpec* momentum_decay_properties[MOMENTUM_DECAY_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiMomentumDecay, maplibre_native_ffi_momentum_decay,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_momentum_decay_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMomentumDecay* self =
    MAPLIBRE_NATIVE_FFI_MOMENTUM_DECAY(object);
  switch (id) {
    case MOMENTUM_DECAY_PROP_MODEL:
      g_value_set_enum(value, self->model);
      break;
    case MOMENTUM_DECAY_PROP_HALF_LIFE_MS:
      g_value_set_double(value, self->half_life_ms);
      break;
    case MOMENTUM_DECAY_PROP_STOP_SPEED:
      g_value_set_double(value, self->stop_speed);
      break;
    case MOMENTUM_DECAY_PROP_DISTANCE:
      g_value_set_double(value, self->distance);
      break;
    case MOMENTUM_DECAY_PROP_DURATION_MS:
      g_value_set_double(value, self->duration_ms);
      break;
    case MOMENTUM_DECAY_PROP_EASING:
      g_value_set_boxed(value, &self->easing);
      break;
    case MOMENTUM_DECAY_PROP_MIN_SPEED:
      g_value_set_double(value, self->min_speed);
      break;
    case MOMENTUM_DECAY_PROP_MAX_SPEED:
      g_value_set_double(value, self->max_speed);
      break;
    case MOMENTUM_DECAY_PROP_MAX_DISTANCE:
      g_value_set_double(value, self->max_distance);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_momentum_decay_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMomentumDecay* self =
    MAPLIBRE_NATIVE_FFI_MOMENTUM_DECAY(object);
  switch (id) {
    case MOMENTUM_DECAY_PROP_MODEL:
      maplibre_native_ffi_momentum_decay_set_model(
        self, g_value_get_enum(value)
      );
      break;
    case MOMENTUM_DECAY_PROP_HALF_LIFE_MS:
      maplibre_native_ffi_momentum_decay_set_half_life_ms(
        self, g_value_get_double(value)
      );
      break;
    case MOMENTUM_DECAY_PROP_STOP_SPEED:
      maplibre_native_ffi_momentum_decay_set_stop_speed(
        self, g_value_get_double(value)
      );
      break;
    case MOMENTUM_DECAY_PROP_DISTANCE:
      maplibre_native_ffi_momentum_decay_set_distance(
        self, g_value_get_double(value)
      );
      break;
    case MOMENTUM_DECAY_PROP_DURATION_MS:
      maplibre_native_ffi_momentum_decay_set_duration_ms(
        self, g_value_get_double(value)
      );
      break;
    case MOMENTUM_DECAY_PROP_EASING:
      maplibre_native_ffi_momentum_decay_set_easing(
        self, g_value_get_boxed(value)
      );
      break;
    case MOMENTUM_DECAY_PROP_MIN_SPEED:
      maplibre_native_ffi_momentum_decay_set_min_speed(
        self, g_value_get_double(value)
      );
      break;
    case MOMENTUM_DECAY_PROP_MAX_SPEED:
      maplibre_native_ffi_momentum_decay_set_max_speed(
        self, g_value_get_double(value)
      );
      break;
    case MOMENTUM_DECAY_PROP_MAX_DISTANCE:
      maplibre_native_ffi_momentum_decay_set_max_distance(
        self, g_value_get_double(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_momentum_decay_class_init(
  MaplibreNativeFfiMomentumDecayClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property = maplibre_native_ffi_momentum_decay_get_property;
  object_class->set_property = maplibre_native_ffi_momentum_decay_set_property;
  momentum_decay_properties[MOMENTUM_DECAY_PROP_MODEL] = g_param_spec_enum(
    "model", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_MOMENTUM_DECAY_MODEL, 0,
    PROPERTY_FLAGS
  );
  momentum_decay_properties[MOMENTUM_DECAY_PROP_HALF_LIFE_MS] =
    g_param_spec_double(
      "half-life-ms", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  momentum_decay_properties[MOMENTUM_DECAY_PROP_STOP_SPEED] =
    g_param_spec_double(
      "stop-speed", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  momentum_decay_properties[MOMENTUM_DECAY_PROP_DISTANCE] = g_param_spec_double(
    "distance", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  momentum_decay_properties[MOMENTUM_DECAY_PROP_DURATION_MS] =
    g_param_spec_double(
      "duration-ms", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  momentum_decay_properties[MOMENTUM_DECAY_PROP_EASING] = g_param_spec_boxed(
    "easing", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_UNIT_BEZIER, PROPERTY_FLAGS
  );
  momentum_decay_properties[MOMENTUM_DECAY_PROP_MIN_SPEED] =
    g_param_spec_double(
      "min-speed", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  momentum_decay_properties[MOMENTUM_DECAY_PROP_MAX_SPEED] =
    g_param_spec_double(
      "max-speed", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  momentum_decay_properties[MOMENTUM_DECAY_PROP_MAX_DISTANCE] =
    g_param_spec_double(
      "max-distance", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, MOMENTUM_DECAY_N_PROPERTIES, momentum_decay_properties
  );
}

static void maplibre_native_ffi_momentum_decay_init(
  MaplibreNativeFfiMomentumDecay* self
) {
  (void)self;
}

MaplibreNativeFfiMomentumDecay* maplibre_native_ffi_momentum_decay_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_MOMENTUM_DECAY, NULL);
}

MaplibreNativeFfiMomentumDecayModel
maplibre_native_ffi_momentum_decay_get_model(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self),
    (MaplibreNativeFfiMomentumDecayModel)0
  );
  return self->model;
}

gboolean maplibre_native_ffi_momentum_decay_has_model(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), FALSE);
  return (self->fields & MLN_MOMENTUM_DECAY_MODEL) != 0;
}

void maplibre_native_ffi_momentum_decay_set_model(
  MaplibreNativeFfiMomentumDecay* self,
  MaplibreNativeFfiMomentumDecayModel value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self));
  gboolean was_set = (self->fields & MLN_MOMENTUM_DECAY_MODEL) != 0;
  gboolean changed = self->model != value;
  self->model = value;
  self->fields |= MLN_MOMENTUM_DECAY_MODEL;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), momentum_decay_properties[MOMENTUM_DECAY_PROP_MODEL]
    );
}

gdouble maplibre_native_ffi_momentum_decay_get_half_life_ms(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), 0);
  return self->half_life_ms;
}

gboolean maplibre_native_ffi_momentum_decay_has_half_life_ms(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), FALSE);
  return (self->fields & MLN_MOMENTUM_DECAY_HALF_LIFE_MS) != 0;
}

void maplibre_native_ffi_momentum_decay_set_half_life_ms(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self));
  gboolean was_set = (self->fields & MLN_MOMENTUM_DECAY_HALF_LIFE_MS) != 0;
  gboolean changed = self->half_life_ms != value;
  self->half_life_ms = value;
  self->fields |= MLN_MOMENTUM_DECAY_HALF_LIFE_MS;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      momentum_decay_properties[MOMENTUM_DECAY_PROP_HALF_LIFE_MS]
    );
}

gdouble maplibre_native_ffi_momentum_decay_get_stop_speed(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), 0);
  return self->stop_speed;
}

gboolean maplibre_native_ffi_momentum_decay_has_stop_speed(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), FALSE);
  return (self->fields & MLN_MOMENTUM_DECAY_STOP_SPEED) != 0;
}

void maplibre_native_ffi_momentum_decay_set_stop_speed(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self));
  gboolean was_set = (self->fields & MLN_MOMENTUM_DECAY_STOP_SPEED) != 0;
  gboolean changed = self->stop_speed != value;
  self->stop_speed = value;
  self->fields |= MLN_MOMENTUM_DECAY_STOP_SPEED;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), momentum_decay_properties[MOMENTUM_DECAY_PROP_STOP_SPEED]
    );
}

gdouble maplibre_native_ffi_momentum_decay_get_distance(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), 0);
  return self->distance;
}

gboolean maplibre_native_ffi_momentum_decay_has_distance(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), FALSE);
  return (self->fields & MLN_MOMENTUM_DECAY_DISTANCE) != 0;
}

void maplibre_native_ffi_momentum_decay_set_distance(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self));
  gboolean was_set = (self->fields & MLN_MOMENTUM_DECAY_DISTANCE) != 0;
  gboolean changed = self->distance != value;
  self->distance = value;
  self->fields |= MLN_MOMENTUM_DECAY_DISTANCE;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), momentum_decay_properties[MOMENTUM_DECAY_PROP_DISTANCE]
    );
}

gdouble maplibre_native_ffi_momentum_decay_get_duration_ms(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), 0);
  return self->duration_ms;
}

gboolean maplibre_native_ffi_momentum_decay_has_duration_ms(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), FALSE);
  return (self->fields & MLN_MOMENTUM_DECAY_DURATION_MS) != 0;
}

void maplibre_native_ffi_momentum_decay_set_duration_ms(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self));
  gboolean was_set = (self->fields & MLN_MOMENTUM_DECAY_DURATION_MS) != 0;
  gboolean changed = self->duration_ms != value;
  self->duration_ms = value;
  self->fields |= MLN_MOMENTUM_DECAY_DURATION_MS;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), momentum_decay_properties[MOMENTUM_DECAY_PROP_DURATION_MS]
    );
}

MaplibreNativeFfiUnitBezier* maplibre_native_ffi_momentum_decay_get_easing(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), NULL);
  return (MaplibreNativeFfiUnitBezier*)&self->easing;
}

gboolean maplibre_native_ffi_momentum_decay_has_easing(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), FALSE);
  return (self->fields & MLN_MOMENTUM_DECAY_EASING) != 0;
}

void maplibre_native_ffi_momentum_decay_set_easing(
  MaplibreNativeFfiMomentumDecay* self, const MaplibreNativeFfiUnitBezier* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_MOMENTUM_DECAY_EASING) != 0;
  gboolean changed = memcmp(&self->easing, value, sizeof self->easing) != 0;
  self->easing = *value;
  self->fields |= MLN_MOMENTUM_DECAY_EASING;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), momentum_decay_properties[MOMENTUM_DECAY_PROP_EASING]
    );
}

gdouble maplibre_native_ffi_momentum_decay_get_min_speed(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), 0);
  return self->min_speed;
}

gboolean maplibre_native_ffi_momentum_decay_has_min_speed(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), FALSE);
  return (self->fields & MLN_MOMENTUM_DECAY_MIN_SPEED) != 0;
}

void maplibre_native_ffi_momentum_decay_set_min_speed(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self));
  gboolean was_set = (self->fields & MLN_MOMENTUM_DECAY_MIN_SPEED) != 0;
  gboolean changed = self->min_speed != value;
  self->min_speed = value;
  self->fields |= MLN_MOMENTUM_DECAY_MIN_SPEED;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), momentum_decay_properties[MOMENTUM_DECAY_PROP_MIN_SPEED]
    );
}

gdouble maplibre_native_ffi_momentum_decay_get_max_speed(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), 0);
  return self->max_speed;
}

gboolean maplibre_native_ffi_momentum_decay_has_max_speed(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), FALSE);
  return (self->fields & MLN_MOMENTUM_DECAY_MAX_SPEED) != 0;
}

void maplibre_native_ffi_momentum_decay_set_max_speed(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self));
  gboolean was_set = (self->fields & MLN_MOMENTUM_DECAY_MAX_SPEED) != 0;
  gboolean changed = self->max_speed != value;
  self->max_speed = value;
  self->fields |= MLN_MOMENTUM_DECAY_MAX_SPEED;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), momentum_decay_properties[MOMENTUM_DECAY_PROP_MAX_SPEED]
    );
}

gdouble maplibre_native_ffi_momentum_decay_get_max_distance(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), 0);
  return self->max_distance;
}

gboolean maplibre_native_ffi_momentum_decay_has_max_distance(
  MaplibreNativeFfiMomentumDecay* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self), FALSE);
  return (self->fields & MLN_MOMENTUM_DECAY_MAX_DISTANCE) != 0;
}

void maplibre_native_ffi_momentum_decay_set_max_distance(
  MaplibreNativeFfiMomentumDecay* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(self));
  gboolean was_set = (self->fields & MLN_MOMENTUM_DECAY_MAX_DISTANCE) != 0;
  gboolean changed = self->max_distance != value;
  self->max_distance = value;
  self->fields |= MLN_MOMENTUM_DECAY_MAX_DISTANCE;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      momentum_decay_properties[MOMENTUM_DECAY_PROP_MAX_DISTANCE]
    );
}

MaplibreNativeFfiMomentumDecay* maplibre_native_ffi_momentum_decay_copy(
  const MaplibreNativeFfiMomentumDecay* self
) {
  MaplibreNativeFfiMomentumDecay* copy =
    maplibre_native_ffi_momentum_decay_new();
  copy->fields = self->fields;
  copy->model = self->model;
  copy->half_life_ms = self->half_life_ms;
  copy->stop_speed = self->stop_speed;
  copy->distance = self->distance;
  copy->duration_ms = self->duration_ms;
  copy->easing = self->easing;
  copy->min_speed = self->min_speed;
  copy->max_speed = self->max_speed;
  copy->max_distance = self->max_distance;
  return copy;
}

void maplibre_native_ffi_momentum_decay_to_native(
  const MaplibreNativeFfiMomentumDecay* self, mln_momentum_decay* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->fields = (mln_momentum_decay_fields)self->fields;
  out->model = (mln_momentum_decay_model)(self->model);
  out->half_life_ms = (double)(self->half_life_ms);
  out->stop_speed = (double)(self->stop_speed);
  out->distance = (double)(self->distance);
  out->duration_ms = (double)(self->duration_ms);
  maplibre_native_ffi_unit_bezier_to_native(&self->easing, &out->easing, call);
  out->min_speed = (double)(self->min_speed);
  out->max_speed = (double)(self->max_speed);
  out->max_distance = (double)(self->max_distance);
}

struct _MaplibreNativeFfiMomentumSnap {
  GObject parent_instance;
  guint64 fields;
  gdouble spacing;
  gdouble offset;
  gdouble tolerance;
  gdouble duration_ms;
};

enum {
  MOMENTUM_SNAP_PROP_0,
  MOMENTUM_SNAP_PROP_SPACING,
  MOMENTUM_SNAP_PROP_OFFSET,
  MOMENTUM_SNAP_PROP_TOLERANCE,
  MOMENTUM_SNAP_PROP_DURATION_MS,
  MOMENTUM_SNAP_N_PROPERTIES
};

static GParamSpec* momentum_snap_properties[MOMENTUM_SNAP_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiMomentumSnap, maplibre_native_ffi_momentum_snap,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_momentum_snap_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMomentumSnap* self =
    MAPLIBRE_NATIVE_FFI_MOMENTUM_SNAP(object);
  switch (id) {
    case MOMENTUM_SNAP_PROP_SPACING:
      g_value_set_double(value, self->spacing);
      break;
    case MOMENTUM_SNAP_PROP_OFFSET:
      g_value_set_double(value, self->offset);
      break;
    case MOMENTUM_SNAP_PROP_TOLERANCE:
      g_value_set_double(value, self->tolerance);
      break;
    case MOMENTUM_SNAP_PROP_DURATION_MS:
      g_value_set_double(value, self->duration_ms);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_momentum_snap_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMomentumSnap* self =
    MAPLIBRE_NATIVE_FFI_MOMENTUM_SNAP(object);
  switch (id) {
    case MOMENTUM_SNAP_PROP_SPACING:
      maplibre_native_ffi_momentum_snap_set_spacing(
        self, g_value_get_double(value)
      );
      break;
    case MOMENTUM_SNAP_PROP_OFFSET:
      maplibre_native_ffi_momentum_snap_set_offset(
        self, g_value_get_double(value)
      );
      break;
    case MOMENTUM_SNAP_PROP_TOLERANCE:
      maplibre_native_ffi_momentum_snap_set_tolerance(
        self, g_value_get_double(value)
      );
      break;
    case MOMENTUM_SNAP_PROP_DURATION_MS:
      maplibre_native_ffi_momentum_snap_set_duration_ms(
        self, g_value_get_double(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_momentum_snap_class_init(
  MaplibreNativeFfiMomentumSnapClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property = maplibre_native_ffi_momentum_snap_get_property;
  object_class->set_property = maplibre_native_ffi_momentum_snap_set_property;
  momentum_snap_properties[MOMENTUM_SNAP_PROP_SPACING] = g_param_spec_double(
    "spacing", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  momentum_snap_properties[MOMENTUM_SNAP_PROP_OFFSET] = g_param_spec_double(
    "offset", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  momentum_snap_properties[MOMENTUM_SNAP_PROP_TOLERANCE] = g_param_spec_double(
    "tolerance", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  momentum_snap_properties[MOMENTUM_SNAP_PROP_DURATION_MS] =
    g_param_spec_double(
      "duration-ms", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, MOMENTUM_SNAP_N_PROPERTIES, momentum_snap_properties
  );
}

static void maplibre_native_ffi_momentum_snap_init(
  MaplibreNativeFfiMomentumSnap* self
) {
  (void)self;
}

MaplibreNativeFfiMomentumSnap* maplibre_native_ffi_momentum_snap_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_MOMENTUM_SNAP, NULL);
}

gdouble maplibre_native_ffi_momentum_snap_get_spacing(
  MaplibreNativeFfiMomentumSnap* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_SNAP(self), 0);
  return self->spacing;
}

gboolean maplibre_native_ffi_momentum_snap_has_spacing(
  MaplibreNativeFfiMomentumSnap* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_SNAP(self), FALSE);
  return (self->fields & MLN_MOMENTUM_SNAP_SPACING) != 0;
}

void maplibre_native_ffi_momentum_snap_set_spacing(
  MaplibreNativeFfiMomentumSnap* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_SNAP(self));
  gboolean was_set = (self->fields & MLN_MOMENTUM_SNAP_SPACING) != 0;
  gboolean changed = self->spacing != value;
  self->spacing = value;
  self->fields |= MLN_MOMENTUM_SNAP_SPACING;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), momentum_snap_properties[MOMENTUM_SNAP_PROP_SPACING]
    );
}

gdouble maplibre_native_ffi_momentum_snap_get_offset(
  MaplibreNativeFfiMomentumSnap* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_SNAP(self), 0);
  return self->offset;
}

gboolean maplibre_native_ffi_momentum_snap_has_offset(
  MaplibreNativeFfiMomentumSnap* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_SNAP(self), FALSE);
  return (self->fields & MLN_MOMENTUM_SNAP_OFFSET) != 0;
}

void maplibre_native_ffi_momentum_snap_set_offset(
  MaplibreNativeFfiMomentumSnap* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_SNAP(self));
  gboolean was_set = (self->fields & MLN_MOMENTUM_SNAP_OFFSET) != 0;
  gboolean changed = self->offset != value;
  self->offset = value;
  self->fields |= MLN_MOMENTUM_SNAP_OFFSET;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), momentum_snap_properties[MOMENTUM_SNAP_PROP_OFFSET]
    );
}

gdouble maplibre_native_ffi_momentum_snap_get_tolerance(
  MaplibreNativeFfiMomentumSnap* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_SNAP(self), 0);
  return self->tolerance;
}

gboolean maplibre_native_ffi_momentum_snap_has_tolerance(
  MaplibreNativeFfiMomentumSnap* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_SNAP(self), FALSE);
  return (self->fields & MLN_MOMENTUM_SNAP_TOLERANCE) != 0;
}

void maplibre_native_ffi_momentum_snap_set_tolerance(
  MaplibreNativeFfiMomentumSnap* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_SNAP(self));
  gboolean was_set = (self->fields & MLN_MOMENTUM_SNAP_TOLERANCE) != 0;
  gboolean changed = self->tolerance != value;
  self->tolerance = value;
  self->fields |= MLN_MOMENTUM_SNAP_TOLERANCE;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), momentum_snap_properties[MOMENTUM_SNAP_PROP_TOLERANCE]
    );
}

gdouble maplibre_native_ffi_momentum_snap_get_duration_ms(
  MaplibreNativeFfiMomentumSnap* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_SNAP(self), 0);
  return self->duration_ms;
}

gboolean maplibre_native_ffi_momentum_snap_has_duration_ms(
  MaplibreNativeFfiMomentumSnap* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_SNAP(self), FALSE);
  return (self->fields & MLN_MOMENTUM_SNAP_DURATION_MS) != 0;
}

void maplibre_native_ffi_momentum_snap_set_duration_ms(
  MaplibreNativeFfiMomentumSnap* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_SNAP(self));
  gboolean was_set = (self->fields & MLN_MOMENTUM_SNAP_DURATION_MS) != 0;
  gboolean changed = self->duration_ms != value;
  self->duration_ms = value;
  self->fields |= MLN_MOMENTUM_SNAP_DURATION_MS;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), momentum_snap_properties[MOMENTUM_SNAP_PROP_DURATION_MS]
    );
}

MaplibreNativeFfiMomentumSnap* maplibre_native_ffi_momentum_snap_copy(
  const MaplibreNativeFfiMomentumSnap* self
) {
  MaplibreNativeFfiMomentumSnap* copy = maplibre_native_ffi_momentum_snap_new();
  copy->fields = self->fields;
  copy->spacing = self->spacing;
  copy->offset = self->offset;
  copy->tolerance = self->tolerance;
  copy->duration_ms = self->duration_ms;
  return copy;
}

void maplibre_native_ffi_momentum_snap_to_native(
  const MaplibreNativeFfiMomentumSnap* self, mln_momentum_snap* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->fields = (mln_momentum_snap_fields)self->fields;
  out->spacing = (double)(self->spacing);
  out->offset = (double)(self->offset);
  out->tolerance = (double)(self->tolerance);
  out->duration_ms = (double)(self->duration_ms);
}

struct _MaplibreNativeFfiAnchoredMomentum {
  GObject parent_instance;
  guint64 fields;
  gdouble velocity;
  MaplibreNativeFfiScreenPoint anchor;
  MaplibreNativeFfiMomentumDecay* decay;
  MaplibreNativeFfiMomentumSnap* snap;
};

enum {
  ANCHORED_MOMENTUM_PROP_0,
  ANCHORED_MOMENTUM_PROP_VELOCITY,
  ANCHORED_MOMENTUM_PROP_ANCHOR,
  ANCHORED_MOMENTUM_PROP_DECAY,
  ANCHORED_MOMENTUM_PROP_SNAP,
  ANCHORED_MOMENTUM_N_PROPERTIES
};

static GParamSpec* anchored_momentum_properties[ANCHORED_MOMENTUM_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiAnchoredMomentum, maplibre_native_ffi_anchored_momentum,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_anchored_momentum_finalize(GObject* object) {
  MaplibreNativeFfiAnchoredMomentum* self =
    MAPLIBRE_NATIVE_FFI_ANCHORED_MOMENTUM(object);
  g_clear_object(&self->decay);
  g_clear_object(&self->snap);
  G_OBJECT_CLASS(maplibre_native_ffi_anchored_momentum_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_anchored_momentum_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiAnchoredMomentum* self =
    MAPLIBRE_NATIVE_FFI_ANCHORED_MOMENTUM(object);
  switch (id) {
    case ANCHORED_MOMENTUM_PROP_VELOCITY:
      g_value_set_double(value, self->velocity);
      break;
    case ANCHORED_MOMENTUM_PROP_ANCHOR:
      g_value_set_boxed(value, &self->anchor);
      break;
    case ANCHORED_MOMENTUM_PROP_DECAY:
      g_value_set_object(value, self->decay);
      break;
    case ANCHORED_MOMENTUM_PROP_SNAP:
      g_value_set_object(value, self->snap);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_anchored_momentum_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiAnchoredMomentum* self =
    MAPLIBRE_NATIVE_FFI_ANCHORED_MOMENTUM(object);
  switch (id) {
    case ANCHORED_MOMENTUM_PROP_VELOCITY:
      maplibre_native_ffi_anchored_momentum_set_velocity(
        self, g_value_get_double(value)
      );
      break;
    case ANCHORED_MOMENTUM_PROP_ANCHOR:
      maplibre_native_ffi_anchored_momentum_set_anchor(
        self, g_value_get_boxed(value)
      );
      break;
    case ANCHORED_MOMENTUM_PROP_DECAY:
      maplibre_native_ffi_anchored_momentum_set_decay(
        self, g_value_get_object(value)
      );
      break;
    case ANCHORED_MOMENTUM_PROP_SNAP:
      maplibre_native_ffi_anchored_momentum_set_snap(
        self, g_value_get_object(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_anchored_momentum_class_init(
  MaplibreNativeFfiAnchoredMomentumClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_anchored_momentum_finalize;
  object_class->get_property =
    maplibre_native_ffi_anchored_momentum_get_property;
  object_class->set_property =
    maplibre_native_ffi_anchored_momentum_set_property;
  anchored_momentum_properties[ANCHORED_MOMENTUM_PROP_VELOCITY] =
    g_param_spec_double(
      "velocity", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  anchored_momentum_properties[ANCHORED_MOMENTUM_PROP_ANCHOR] =
    g_param_spec_boxed(
      "anchor", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_SCREEN_POINT,
      PROPERTY_FLAGS
    );
  anchored_momentum_properties[ANCHORED_MOMENTUM_PROP_DECAY] =
    g_param_spec_object(
      "decay", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_MOMENTUM_DECAY,
      PROPERTY_FLAGS
    );
  anchored_momentum_properties[ANCHORED_MOMENTUM_PROP_SNAP] =
    g_param_spec_object(
      "snap", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_MOMENTUM_SNAP, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, ANCHORED_MOMENTUM_N_PROPERTIES, anchored_momentum_properties
  );
}

static void maplibre_native_ffi_anchored_momentum_init(
  MaplibreNativeFfiAnchoredMomentum* self
) {
  self->decay = maplibre_native_ffi_momentum_decay_new();
  self->snap = maplibre_native_ffi_momentum_snap_new();
}

MaplibreNativeFfiAnchoredMomentum* maplibre_native_ffi_anchored_momentum_new(
  void
) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_ANCHORED_MOMENTUM, NULL);
}

gdouble maplibre_native_ffi_anchored_momentum_get_velocity(
  MaplibreNativeFfiAnchoredMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(self), 0);
  return self->velocity;
}

gboolean maplibre_native_ffi_anchored_momentum_has_velocity(
  MaplibreNativeFfiAnchoredMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(self), FALSE);
  return (self->fields & MLN_ANCHORED_MOMENTUM_VELOCITY) != 0;
}

void maplibre_native_ffi_anchored_momentum_set_velocity(
  MaplibreNativeFfiAnchoredMomentum* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(self));
  gboolean was_set = (self->fields & MLN_ANCHORED_MOMENTUM_VELOCITY) != 0;
  gboolean changed = self->velocity != value;
  self->velocity = value;
  self->fields |= MLN_ANCHORED_MOMENTUM_VELOCITY;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      anchored_momentum_properties[ANCHORED_MOMENTUM_PROP_VELOCITY]
    );
}

MaplibreNativeFfiScreenPoint* maplibre_native_ffi_anchored_momentum_get_anchor(
  MaplibreNativeFfiAnchoredMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(self), NULL);
  return (MaplibreNativeFfiScreenPoint*)&self->anchor;
}

gboolean maplibre_native_ffi_anchored_momentum_has_anchor(
  MaplibreNativeFfiAnchoredMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(self), FALSE);
  return (self->fields & MLN_ANCHORED_MOMENTUM_ANCHOR) != 0;
}

void maplibre_native_ffi_anchored_momentum_set_anchor(
  MaplibreNativeFfiAnchoredMomentum* self,
  const MaplibreNativeFfiScreenPoint* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_ANCHORED_MOMENTUM_ANCHOR) != 0;
  gboolean changed = memcmp(&self->anchor, value, sizeof self->anchor) != 0;
  self->anchor = *value;
  self->fields |= MLN_ANCHORED_MOMENTUM_ANCHOR;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      anchored_momentum_properties[ANCHORED_MOMENTUM_PROP_ANCHOR]
    );
}

MaplibreNativeFfiMomentumDecay* maplibre_native_ffi_anchored_momentum_get_decay(
  MaplibreNativeFfiAnchoredMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(self), NULL);
  return self->decay;
}

void maplibre_native_ffi_anchored_momentum_set_decay(
  MaplibreNativeFfiAnchoredMomentum* self, MaplibreNativeFfiMomentumDecay* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(value));
  MaplibreNativeFfiMomentumDecay* previous = self->decay;
  self->decay = maplibre_native_ffi_momentum_decay_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), anchored_momentum_properties[ANCHORED_MOMENTUM_PROP_DECAY]
  );
}

MaplibreNativeFfiMomentumSnap* maplibre_native_ffi_anchored_momentum_get_snap(
  MaplibreNativeFfiAnchoredMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(self), NULL);
  return self->snap;
}

gboolean maplibre_native_ffi_anchored_momentum_has_snap(
  MaplibreNativeFfiAnchoredMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(self), FALSE);
  return (self->fields & MLN_ANCHORED_MOMENTUM_SNAP) != 0;
}

void maplibre_native_ffi_anchored_momentum_set_snap(
  MaplibreNativeFfiAnchoredMomentum* self, MaplibreNativeFfiMomentumSnap* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_SNAP(value));
  MaplibreNativeFfiMomentumSnap* previous = self->snap;
  self->snap = maplibre_native_ffi_momentum_snap_copy(value);
  if (previous != NULL) g_object_unref(previous);
  self->fields |= MLN_ANCHORED_MOMENTUM_SNAP;
  g_object_notify_by_pspec(
    G_OBJECT(self), anchored_momentum_properties[ANCHORED_MOMENTUM_PROP_SNAP]
  );
}

MaplibreNativeFfiAnchoredMomentum* maplibre_native_ffi_anchored_momentum_copy(
  const MaplibreNativeFfiAnchoredMomentum* self
) {
  MaplibreNativeFfiAnchoredMomentum* copy =
    maplibre_native_ffi_anchored_momentum_new();
  copy->fields = self->fields;
  copy->velocity = self->velocity;
  copy->anchor = self->anchor;
  g_clear_object(&copy->decay);
  copy->decay = self->decay != NULL
                  ? maplibre_native_ffi_momentum_decay_copy(self->decay)
                  : NULL;
  g_clear_object(&copy->snap);
  copy->snap = self->snap != NULL
                 ? maplibre_native_ffi_momentum_snap_copy(self->snap)
                 : NULL;
  return copy;
}

void maplibre_native_ffi_anchored_momentum_to_native(
  const MaplibreNativeFfiAnchoredMomentum* self, mln_anchored_momentum* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->fields = (mln_anchored_momentum_fields)self->fields;
  out->velocity = (double)(self->velocity);
  maplibre_native_ffi_screen_point_to_native(&self->anchor, &out->anchor, call);
  if (self->decay != NULL)
    maplibre_native_ffi_momentum_decay_to_native(
      self->decay, &out->decay, call
    );
  if (self->snap != NULL)
    maplibre_native_ffi_momentum_snap_to_native(self->snap, &out->snap, call);
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

struct _MaplibreNativeFfiAnimationOptions {
  GObject parent_instance;
  guint64 fields;
  gdouble duration_ms;
  gdouble velocity;
  gdouble min_zoom;
  MaplibreNativeFfiUnitBezier easing;
  guint64 transition_id;
  MaplibreNativeFfiCameraTransitionHandler* end_handler;
};

enum {
  ANIMATION_OPTIONS_PROP_0,
  ANIMATION_OPTIONS_PROP_DURATION_MS,
  ANIMATION_OPTIONS_PROP_VELOCITY,
  ANIMATION_OPTIONS_PROP_MIN_ZOOM,
  ANIMATION_OPTIONS_PROP_EASING,
  ANIMATION_OPTIONS_PROP_TRANSITION_ID,
  ANIMATION_OPTIONS_PROP_END_HANDLER,
  ANIMATION_OPTIONS_N_PROPERTIES
};

static GParamSpec* animation_options_properties[ANIMATION_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiAnimationOptions, maplibre_native_ffi_animation_options,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_animation_options_finalize(GObject* object) {
  MaplibreNativeFfiAnimationOptions* self =
    MAPLIBRE_NATIVE_FFI_ANIMATION_OPTIONS(object);
  g_clear_pointer(
    &self->end_handler, maplibre_native_ffi_camera_transition_handler_free
  );
  G_OBJECT_CLASS(maplibre_native_ffi_animation_options_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_animation_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiAnimationOptions* self =
    MAPLIBRE_NATIVE_FFI_ANIMATION_OPTIONS(object);
  switch (id) {
    case ANIMATION_OPTIONS_PROP_DURATION_MS:
      g_value_set_double(value, self->duration_ms);
      break;
    case ANIMATION_OPTIONS_PROP_VELOCITY:
      g_value_set_double(value, self->velocity);
      break;
    case ANIMATION_OPTIONS_PROP_MIN_ZOOM:
      g_value_set_double(value, self->min_zoom);
      break;
    case ANIMATION_OPTIONS_PROP_EASING:
      g_value_set_boxed(value, &self->easing);
      break;
    case ANIMATION_OPTIONS_PROP_TRANSITION_ID:
      g_value_set_uint64(value, self->transition_id);
      break;
    case ANIMATION_OPTIONS_PROP_END_HANDLER:
      g_value_set_boxed(value, self->end_handler);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_animation_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiAnimationOptions* self =
    MAPLIBRE_NATIVE_FFI_ANIMATION_OPTIONS(object);
  switch (id) {
    case ANIMATION_OPTIONS_PROP_DURATION_MS:
      maplibre_native_ffi_animation_options_set_duration_ms(
        self, g_value_get_double(value)
      );
      break;
    case ANIMATION_OPTIONS_PROP_VELOCITY:
      maplibre_native_ffi_animation_options_set_velocity(
        self, g_value_get_double(value)
      );
      break;
    case ANIMATION_OPTIONS_PROP_MIN_ZOOM:
      maplibre_native_ffi_animation_options_set_min_zoom(
        self, g_value_get_double(value)
      );
      break;
    case ANIMATION_OPTIONS_PROP_EASING:
      maplibre_native_ffi_animation_options_set_easing(
        self, g_value_get_boxed(value)
      );
      break;
    case ANIMATION_OPTIONS_PROP_TRANSITION_ID:
      maplibre_native_ffi_animation_options_set_transition_id(
        self, g_value_get_uint64(value)
      );
      break;
    case ANIMATION_OPTIONS_PROP_END_HANDLER:
      maplibre_native_ffi_animation_options_set_end_handler(
        self, g_value_get_boxed(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_animation_options_class_init(
  MaplibreNativeFfiAnimationOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_animation_options_finalize;
  object_class->get_property =
    maplibre_native_ffi_animation_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_animation_options_set_property;
  animation_options_properties[ANIMATION_OPTIONS_PROP_DURATION_MS] =
    g_param_spec_double(
      "duration-ms", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  animation_options_properties[ANIMATION_OPTIONS_PROP_VELOCITY] =
    g_param_spec_double(
      "velocity", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  animation_options_properties[ANIMATION_OPTIONS_PROP_MIN_ZOOM] =
    g_param_spec_double(
      "min-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  animation_options_properties[ANIMATION_OPTIONS_PROP_EASING] =
    g_param_spec_boxed(
      "easing", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_UNIT_BEZIER, PROPERTY_FLAGS
    );
  animation_options_properties[ANIMATION_OPTIONS_PROP_TRANSITION_ID] =
    g_param_spec_uint64(
      "transition-id", NULL, NULL, 0, G_MAXUINT64, 0, PROPERTY_FLAGS
    );
  animation_options_properties[ANIMATION_OPTIONS_PROP_END_HANDLER] =
    g_param_spec_boxed(
      "end-handler", NULL, NULL,
      MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_TRANSITION_HANDLER, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, ANIMATION_OPTIONS_N_PROPERTIES, animation_options_properties
  );
}

static void maplibre_native_ffi_animation_options_init(
  MaplibreNativeFfiAnimationOptions* self
) {
  self->end_handler = maplibre_native_ffi_camera_transition_handler_new();
}

MaplibreNativeFfiAnimationOptions* maplibre_native_ffi_animation_options_new(
  void
) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_ANIMATION_OPTIONS, NULL);
}

gdouble maplibre_native_ffi_animation_options_get_duration_ms(
  MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self), 0);
  return self->duration_ms;
}

gboolean maplibre_native_ffi_animation_options_has_duration_ms(
  MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self), FALSE);
  return (self->fields & MLN_ANIMATION_OPTION_DURATION) != 0;
}

void maplibre_native_ffi_animation_options_set_duration_ms(
  MaplibreNativeFfiAnimationOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_ANIMATION_OPTION_DURATION) != 0;
  gboolean changed = self->duration_ms != value;
  self->duration_ms = value;
  self->fields |= MLN_ANIMATION_OPTION_DURATION;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      animation_options_properties[ANIMATION_OPTIONS_PROP_DURATION_MS]
    );
}

gdouble maplibre_native_ffi_animation_options_get_velocity(
  MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self), 0);
  return self->velocity;
}

gboolean maplibre_native_ffi_animation_options_has_velocity(
  MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self), FALSE);
  return (self->fields & MLN_ANIMATION_OPTION_VELOCITY) != 0;
}

void maplibre_native_ffi_animation_options_set_velocity(
  MaplibreNativeFfiAnimationOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_ANIMATION_OPTION_VELOCITY) != 0;
  gboolean changed = self->velocity != value;
  self->velocity = value;
  self->fields |= MLN_ANIMATION_OPTION_VELOCITY;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      animation_options_properties[ANIMATION_OPTIONS_PROP_VELOCITY]
    );
}

gdouble maplibre_native_ffi_animation_options_get_min_zoom(
  MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self), 0);
  return self->min_zoom;
}

gboolean maplibre_native_ffi_animation_options_has_min_zoom(
  MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self), FALSE);
  return (self->fields & MLN_ANIMATION_OPTION_MIN_ZOOM) != 0;
}

void maplibre_native_ffi_animation_options_set_min_zoom(
  MaplibreNativeFfiAnimationOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_ANIMATION_OPTION_MIN_ZOOM) != 0;
  gboolean changed = self->min_zoom != value;
  self->min_zoom = value;
  self->fields |= MLN_ANIMATION_OPTION_MIN_ZOOM;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      animation_options_properties[ANIMATION_OPTIONS_PROP_MIN_ZOOM]
    );
}

MaplibreNativeFfiUnitBezier* maplibre_native_ffi_animation_options_get_easing(
  MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self), NULL);
  return (MaplibreNativeFfiUnitBezier*)&self->easing;
}

gboolean maplibre_native_ffi_animation_options_has_easing(
  MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self), FALSE);
  return (self->fields & MLN_ANIMATION_OPTION_EASING) != 0;
}

void maplibre_native_ffi_animation_options_set_easing(
  MaplibreNativeFfiAnimationOptions* self,
  const MaplibreNativeFfiUnitBezier* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_ANIMATION_OPTION_EASING) != 0;
  gboolean changed = memcmp(&self->easing, value, sizeof self->easing) != 0;
  self->easing = *value;
  self->fields |= MLN_ANIMATION_OPTION_EASING;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      animation_options_properties[ANIMATION_OPTIONS_PROP_EASING]
    );
}

guint64 maplibre_native_ffi_animation_options_get_transition_id(
  MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self), 0);
  return self->transition_id;
}

gboolean maplibre_native_ffi_animation_options_has_transition_id(
  MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self), FALSE);
  return (self->fields & MLN_ANIMATION_OPTION_TRANSITION_ID) != 0;
}

void maplibre_native_ffi_animation_options_set_transition_id(
  MaplibreNativeFfiAnimationOptions* self, guint64 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_ANIMATION_OPTION_TRANSITION_ID) != 0;
  gboolean changed = self->transition_id != value;
  self->transition_id = value;
  self->fields |= MLN_ANIMATION_OPTION_TRANSITION_ID;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      animation_options_properties[ANIMATION_OPTIONS_PROP_TRANSITION_ID]
    );
}

MaplibreNativeFfiCameraTransitionHandler*
maplibre_native_ffi_animation_options_get_end_handler(
  MaplibreNativeFfiAnimationOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self), NULL);
  return self->end_handler;
}

void maplibre_native_ffi_animation_options_set_end_handler(
  MaplibreNativeFfiAnimationOptions* self,
  const MaplibreNativeFfiCameraTransitionHandler* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(self));
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiCameraTransitionHandler* previous = self->end_handler;
  self->end_handler = maplibre_native_ffi_camera_transition_handler_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_camera_transition_handler_free(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self),
    animation_options_properties[ANIMATION_OPTIONS_PROP_END_HANDLER]
  );
}

MaplibreNativeFfiAnimationOptions* maplibre_native_ffi_animation_options_copy(
  const MaplibreNativeFfiAnimationOptions* self
) {
  MaplibreNativeFfiAnimationOptions* copy =
    maplibre_native_ffi_animation_options_new();
  copy->fields = self->fields;
  copy->duration_ms = self->duration_ms;
  copy->velocity = self->velocity;
  copy->min_zoom = self->min_zoom;
  copy->easing = self->easing;
  copy->transition_id = self->transition_id;
  g_clear_pointer(
    &copy->end_handler, maplibre_native_ffi_camera_transition_handler_free
  );
  copy->end_handler =
    self->end_handler != NULL
      ? maplibre_native_ffi_camera_transition_handler_copy(self->end_handler)
      : NULL;
  return copy;
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

G_STATIC_ASSERT(sizeof(MaplibreNativeFfiLatLng) == sizeof(mln_lat_lng));
SAME_MEMBER(MaplibreNativeFfiLatLng, mln_lat_lng, latitude);
SAME_MEMBER(MaplibreNativeFfiLatLng, mln_lat_lng, longitude);

void maplibre_native_ffi_lat_lng_to_native(
  const MaplibreNativeFfiLatLng* self, mln_lat_lng* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_lat_lng_read(
  MaplibreNativeFfiLatLng* out, const mln_lat_lng* in
) {
  memcpy(out, in, sizeof *out);
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiLatLngBounds) == sizeof(mln_lat_lng_bounds)
);
SAME_MEMBER(MaplibreNativeFfiLatLngBounds, mln_lat_lng_bounds, southwest);
SAME_MEMBER(MaplibreNativeFfiLatLngBounds, mln_lat_lng_bounds, northeast);

void maplibre_native_ffi_lat_lng_bounds_to_native(
  const MaplibreNativeFfiLatLngBounds* self, mln_lat_lng_bounds* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_lat_lng_bounds_read(
  MaplibreNativeFfiLatLngBounds* out, const mln_lat_lng_bounds* in
) {
  memcpy(out, in, sizeof *out);
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
  GObject parent_instance;
  guint64 fields;
  MaplibreNativeFfiLatLngBounds bounds;
  gdouble min_zoom;
  gdouble max_zoom;
  gdouble min_pitch;
  gdouble max_pitch;
};

enum {
  BOUND_OPTIONS_PROP_0,
  BOUND_OPTIONS_PROP_BOUNDS,
  BOUND_OPTIONS_PROP_MIN_ZOOM,
  BOUND_OPTIONS_PROP_MAX_ZOOM,
  BOUND_OPTIONS_PROP_MIN_PITCH,
  BOUND_OPTIONS_PROP_MAX_PITCH,
  BOUND_OPTIONS_N_PROPERTIES
};

static GParamSpec* bound_options_properties[BOUND_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiBoundOptions, maplibre_native_ffi_bound_options,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_bound_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiBoundOptions* self =
    MAPLIBRE_NATIVE_FFI_BOUND_OPTIONS(object);
  switch (id) {
    case BOUND_OPTIONS_PROP_BOUNDS:
      g_value_set_boxed(value, &self->bounds);
      break;
    case BOUND_OPTIONS_PROP_MIN_ZOOM:
      g_value_set_double(value, self->min_zoom);
      break;
    case BOUND_OPTIONS_PROP_MAX_ZOOM:
      g_value_set_double(value, self->max_zoom);
      break;
    case BOUND_OPTIONS_PROP_MIN_PITCH:
      g_value_set_double(value, self->min_pitch);
      break;
    case BOUND_OPTIONS_PROP_MAX_PITCH:
      g_value_set_double(value, self->max_pitch);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_bound_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiBoundOptions* self =
    MAPLIBRE_NATIVE_FFI_BOUND_OPTIONS(object);
  switch (id) {
    case BOUND_OPTIONS_PROP_BOUNDS:
      maplibre_native_ffi_bound_options_set_bounds(
        self, g_value_get_boxed(value)
      );
      break;
    case BOUND_OPTIONS_PROP_MIN_ZOOM:
      maplibre_native_ffi_bound_options_set_min_zoom(
        self, g_value_get_double(value)
      );
      break;
    case BOUND_OPTIONS_PROP_MAX_ZOOM:
      maplibre_native_ffi_bound_options_set_max_zoom(
        self, g_value_get_double(value)
      );
      break;
    case BOUND_OPTIONS_PROP_MIN_PITCH:
      maplibre_native_ffi_bound_options_set_min_pitch(
        self, g_value_get_double(value)
      );
      break;
    case BOUND_OPTIONS_PROP_MAX_PITCH:
      maplibre_native_ffi_bound_options_set_max_pitch(
        self, g_value_get_double(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_bound_options_class_init(
  MaplibreNativeFfiBoundOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property = maplibre_native_ffi_bound_options_get_property;
  object_class->set_property = maplibre_native_ffi_bound_options_set_property;
  bound_options_properties[BOUND_OPTIONS_PROP_BOUNDS] = g_param_spec_boxed(
    "bounds", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LAT_LNG_BOUNDS,
    PROPERTY_FLAGS
  );
  bound_options_properties[BOUND_OPTIONS_PROP_MIN_ZOOM] = g_param_spec_double(
    "min-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  bound_options_properties[BOUND_OPTIONS_PROP_MAX_ZOOM] = g_param_spec_double(
    "max-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  bound_options_properties[BOUND_OPTIONS_PROP_MIN_PITCH] = g_param_spec_double(
    "min-pitch", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  bound_options_properties[BOUND_OPTIONS_PROP_MAX_PITCH] = g_param_spec_double(
    "max-pitch", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  g_object_class_install_properties(
    object_class, BOUND_OPTIONS_N_PROPERTIES, bound_options_properties
  );
}

static void maplibre_native_ffi_bound_options_init(
  MaplibreNativeFfiBoundOptions* self
) {
  (void)self;
}

MaplibreNativeFfiBoundOptions* maplibre_native_ffi_bound_options_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_BOUND_OPTIONS, NULL);
}

MaplibreNativeFfiLatLngBounds* maplibre_native_ffi_bound_options_get_bounds(
  MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self), NULL);
  return (MaplibreNativeFfiLatLngBounds*)&self->bounds;
}

gboolean maplibre_native_ffi_bound_options_has_bounds(
  MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self), FALSE);
  return (self->fields & MLN_BOUND_OPTION_BOUNDS) != 0;
}

void maplibre_native_ffi_bound_options_set_bounds(
  MaplibreNativeFfiBoundOptions* self,
  const MaplibreNativeFfiLatLngBounds* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_BOUND_OPTION_BOUNDS) != 0;
  gboolean changed = memcmp(&self->bounds, value, sizeof self->bounds) != 0;
  self->bounds = *value;
  self->fields |= MLN_BOUND_OPTION_BOUNDS;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), bound_options_properties[BOUND_OPTIONS_PROP_BOUNDS]
    );
}

gdouble maplibre_native_ffi_bound_options_get_min_zoom(
  MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self), 0);
  return self->min_zoom;
}

gboolean maplibre_native_ffi_bound_options_has_min_zoom(
  MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self), FALSE);
  return (self->fields & MLN_BOUND_OPTION_MIN_ZOOM) != 0;
}

void maplibre_native_ffi_bound_options_set_min_zoom(
  MaplibreNativeFfiBoundOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_BOUND_OPTION_MIN_ZOOM) != 0;
  gboolean changed = self->min_zoom != value;
  self->min_zoom = value;
  self->fields |= MLN_BOUND_OPTION_MIN_ZOOM;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), bound_options_properties[BOUND_OPTIONS_PROP_MIN_ZOOM]
    );
}

gdouble maplibre_native_ffi_bound_options_get_max_zoom(
  MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self), 0);
  return self->max_zoom;
}

gboolean maplibre_native_ffi_bound_options_has_max_zoom(
  MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self), FALSE);
  return (self->fields & MLN_BOUND_OPTION_MAX_ZOOM) != 0;
}

void maplibre_native_ffi_bound_options_set_max_zoom(
  MaplibreNativeFfiBoundOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_BOUND_OPTION_MAX_ZOOM) != 0;
  gboolean changed = self->max_zoom != value;
  self->max_zoom = value;
  self->fields |= MLN_BOUND_OPTION_MAX_ZOOM;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), bound_options_properties[BOUND_OPTIONS_PROP_MAX_ZOOM]
    );
}

gdouble maplibre_native_ffi_bound_options_get_min_pitch(
  MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self), 0);
  return self->min_pitch;
}

gboolean maplibre_native_ffi_bound_options_has_min_pitch(
  MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self), FALSE);
  return (self->fields & MLN_BOUND_OPTION_MIN_PITCH) != 0;
}

void maplibre_native_ffi_bound_options_set_min_pitch(
  MaplibreNativeFfiBoundOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_BOUND_OPTION_MIN_PITCH) != 0;
  gboolean changed = self->min_pitch != value;
  self->min_pitch = value;
  self->fields |= MLN_BOUND_OPTION_MIN_PITCH;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), bound_options_properties[BOUND_OPTIONS_PROP_MIN_PITCH]
    );
}

gdouble maplibre_native_ffi_bound_options_get_max_pitch(
  MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self), 0);
  return self->max_pitch;
}

gboolean maplibre_native_ffi_bound_options_has_max_pitch(
  MaplibreNativeFfiBoundOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self), FALSE);
  return (self->fields & MLN_BOUND_OPTION_MAX_PITCH) != 0;
}

void maplibre_native_ffi_bound_options_set_max_pitch(
  MaplibreNativeFfiBoundOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_BOUND_OPTION_MAX_PITCH) != 0;
  gboolean changed = self->max_pitch != value;
  self->max_pitch = value;
  self->fields |= MLN_BOUND_OPTION_MAX_PITCH;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), bound_options_properties[BOUND_OPTIONS_PROP_MAX_PITCH]
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

G_STATIC_ASSERT(sizeof(MaplibreNativeFfiEdgeInsets) == sizeof(mln_edge_insets));
SAME_MEMBER(MaplibreNativeFfiEdgeInsets, mln_edge_insets, top);
SAME_MEMBER(MaplibreNativeFfiEdgeInsets, mln_edge_insets, left);
SAME_MEMBER(MaplibreNativeFfiEdgeInsets, mln_edge_insets, bottom);
SAME_MEMBER(MaplibreNativeFfiEdgeInsets, mln_edge_insets, right);

void maplibre_native_ffi_edge_insets_to_native(
  const MaplibreNativeFfiEdgeInsets* self, mln_edge_insets* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_edge_insets_read(
  MaplibreNativeFfiEdgeInsets* out, const mln_edge_insets* in
) {
  memcpy(out, in, sizeof *out);
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

G_STATIC_ASSERT(sizeof(MaplibreNativeFfiCamera) == sizeof(mln_camera));
SAME_MEMBER(MaplibreNativeFfiCamera, mln_camera, center);
SAME_MEMBER(MaplibreNativeFfiCamera, mln_camera, center_altitude);
SAME_MEMBER(MaplibreNativeFfiCamera, mln_camera, padding);
SAME_MEMBER(MaplibreNativeFfiCamera, mln_camera, zoom);
SAME_MEMBER(MaplibreNativeFfiCamera, mln_camera, bearing);
SAME_MEMBER(MaplibreNativeFfiCamera, mln_camera, pitch);
SAME_MEMBER(MaplibreNativeFfiCamera, mln_camera, roll);
SAME_MEMBER(MaplibreNativeFfiCamera, mln_camera, field_of_view);

void maplibre_native_ffi_camera_to_native(
  const MaplibreNativeFfiCamera* self, mln_camera* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_camera_read(
  MaplibreNativeFfiCamera* out, const mln_camera* in
) {
  memcpy(out, in, sizeof *out);
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

struct _MaplibreNativeFfiPanMomentum {
  GObject parent_instance;
  guint64 fields;
  MaplibreNativeFfiScreenPoint velocity;
  MaplibreNativeFfiMomentumDecay* decay;
};

enum {
  PAN_MOMENTUM_PROP_0,
  PAN_MOMENTUM_PROP_VELOCITY,
  PAN_MOMENTUM_PROP_DECAY,
  PAN_MOMENTUM_N_PROPERTIES
};

static GParamSpec* pan_momentum_properties[PAN_MOMENTUM_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiPanMomentum, maplibre_native_ffi_pan_momentum, G_TYPE_OBJECT
)

static void maplibre_native_ffi_pan_momentum_finalize(GObject* object) {
  MaplibreNativeFfiPanMomentum* self = MAPLIBRE_NATIVE_FFI_PAN_MOMENTUM(object);
  g_clear_object(&self->decay);
  G_OBJECT_CLASS(maplibre_native_ffi_pan_momentum_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_pan_momentum_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiPanMomentum* self = MAPLIBRE_NATIVE_FFI_PAN_MOMENTUM(object);
  switch (id) {
    case PAN_MOMENTUM_PROP_VELOCITY:
      g_value_set_boxed(value, &self->velocity);
      break;
    case PAN_MOMENTUM_PROP_DECAY:
      g_value_set_object(value, self->decay);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_pan_momentum_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiPanMomentum* self = MAPLIBRE_NATIVE_FFI_PAN_MOMENTUM(object);
  switch (id) {
    case PAN_MOMENTUM_PROP_VELOCITY:
      maplibre_native_ffi_pan_momentum_set_velocity(
        self, g_value_get_boxed(value)
      );
      break;
    case PAN_MOMENTUM_PROP_DECAY:
      maplibre_native_ffi_pan_momentum_set_decay(
        self, g_value_get_object(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_pan_momentum_class_init(
  MaplibreNativeFfiPanMomentumClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_pan_momentum_finalize;
  object_class->get_property = maplibre_native_ffi_pan_momentum_get_property;
  object_class->set_property = maplibre_native_ffi_pan_momentum_set_property;
  pan_momentum_properties[PAN_MOMENTUM_PROP_VELOCITY] = g_param_spec_boxed(
    "velocity", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_SCREEN_POINT,
    PROPERTY_FLAGS
  );
  pan_momentum_properties[PAN_MOMENTUM_PROP_DECAY] = g_param_spec_object(
    "decay", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_MOMENTUM_DECAY, PROPERTY_FLAGS
  );
  g_object_class_install_properties(
    object_class, PAN_MOMENTUM_N_PROPERTIES, pan_momentum_properties
  );
}

static void maplibre_native_ffi_pan_momentum_init(
  MaplibreNativeFfiPanMomentum* self
) {
  self->decay = maplibre_native_ffi_momentum_decay_new();
}

MaplibreNativeFfiPanMomentum* maplibre_native_ffi_pan_momentum_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_PAN_MOMENTUM, NULL);
}

MaplibreNativeFfiScreenPoint* maplibre_native_ffi_pan_momentum_get_velocity(
  MaplibreNativeFfiPanMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_PAN_MOMENTUM(self), NULL);
  return (MaplibreNativeFfiScreenPoint*)&self->velocity;
}

gboolean maplibre_native_ffi_pan_momentum_has_velocity(
  MaplibreNativeFfiPanMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_PAN_MOMENTUM(self), FALSE);
  return (self->fields & MLN_PAN_MOMENTUM_VELOCITY) != 0;
}

void maplibre_native_ffi_pan_momentum_set_velocity(
  MaplibreNativeFfiPanMomentum* self, const MaplibreNativeFfiScreenPoint* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PAN_MOMENTUM(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_PAN_MOMENTUM_VELOCITY) != 0;
  gboolean changed = memcmp(&self->velocity, value, sizeof self->velocity) != 0;
  self->velocity = *value;
  self->fields |= MLN_PAN_MOMENTUM_VELOCITY;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), pan_momentum_properties[PAN_MOMENTUM_PROP_VELOCITY]
    );
}

MaplibreNativeFfiMomentumDecay* maplibre_native_ffi_pan_momentum_get_decay(
  MaplibreNativeFfiPanMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_PAN_MOMENTUM(self), NULL);
  return self->decay;
}

void maplibre_native_ffi_pan_momentum_set_decay(
  MaplibreNativeFfiPanMomentum* self, MaplibreNativeFfiMomentumDecay* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PAN_MOMENTUM(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MOMENTUM_DECAY(value));
  MaplibreNativeFfiMomentumDecay* previous = self->decay;
  self->decay = maplibre_native_ffi_momentum_decay_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), pan_momentum_properties[PAN_MOMENTUM_PROP_DECAY]
  );
}

MaplibreNativeFfiPanMomentum* maplibre_native_ffi_pan_momentum_copy(
  const MaplibreNativeFfiPanMomentum* self
) {
  MaplibreNativeFfiPanMomentum* copy = maplibre_native_ffi_pan_momentum_new();
  copy->fields = self->fields;
  copy->velocity = self->velocity;
  g_clear_object(&copy->decay);
  copy->decay = self->decay != NULL
                  ? maplibre_native_ffi_momentum_decay_copy(self->decay)
                  : NULL;
  return copy;
}

void maplibre_native_ffi_pan_momentum_to_native(
  const MaplibreNativeFfiPanMomentum* self, mln_pan_momentum* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  out->fields = (mln_pan_momentum_fields)self->fields;
  maplibre_native_ffi_screen_point_to_native(
    &self->velocity, &out->velocity, call
  );
  if (self->decay != NULL)
    maplibre_native_ffi_momentum_decay_to_native(
      self->decay, &out->decay, call
    );
}

struct _MaplibreNativeFfiGestureMomentum {
  GObject parent_instance;
  MaplibreNativeFfiPanMomentum* pan;
  MaplibreNativeFfiAnchoredMomentum* zoom;
  MaplibreNativeFfiAnchoredMomentum* bearing;
  MaplibreNativeFfiAnchoredMomentum* pitch;
};

enum {
  GESTURE_MOMENTUM_PROP_0,
  GESTURE_MOMENTUM_PROP_PAN,
  GESTURE_MOMENTUM_PROP_ZOOM,
  GESTURE_MOMENTUM_PROP_BEARING,
  GESTURE_MOMENTUM_PROP_PITCH,
  GESTURE_MOMENTUM_N_PROPERTIES
};

static GParamSpec* gesture_momentum_properties[GESTURE_MOMENTUM_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiGestureMomentum, maplibre_native_ffi_gesture_momentum,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_gesture_momentum_finalize(GObject* object) {
  MaplibreNativeFfiGestureMomentum* self =
    MAPLIBRE_NATIVE_FFI_GESTURE_MOMENTUM(object);
  g_clear_object(&self->pan);
  g_clear_object(&self->zoom);
  g_clear_object(&self->bearing);
  g_clear_object(&self->pitch);
  G_OBJECT_CLASS(maplibre_native_ffi_gesture_momentum_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_gesture_momentum_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiGestureMomentum* self =
    MAPLIBRE_NATIVE_FFI_GESTURE_MOMENTUM(object);
  switch (id) {
    case GESTURE_MOMENTUM_PROP_PAN:
      g_value_set_object(value, self->pan);
      break;
    case GESTURE_MOMENTUM_PROP_ZOOM:
      g_value_set_object(value, self->zoom);
      break;
    case GESTURE_MOMENTUM_PROP_BEARING:
      g_value_set_object(value, self->bearing);
      break;
    case GESTURE_MOMENTUM_PROP_PITCH:
      g_value_set_object(value, self->pitch);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_gesture_momentum_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiGestureMomentum* self =
    MAPLIBRE_NATIVE_FFI_GESTURE_MOMENTUM(object);
  switch (id) {
    case GESTURE_MOMENTUM_PROP_PAN:
      maplibre_native_ffi_gesture_momentum_set_pan(
        self, g_value_get_object(value)
      );
      break;
    case GESTURE_MOMENTUM_PROP_ZOOM:
      maplibre_native_ffi_gesture_momentum_set_zoom(
        self, g_value_get_object(value)
      );
      break;
    case GESTURE_MOMENTUM_PROP_BEARING:
      maplibre_native_ffi_gesture_momentum_set_bearing(
        self, g_value_get_object(value)
      );
      break;
    case GESTURE_MOMENTUM_PROP_PITCH:
      maplibre_native_ffi_gesture_momentum_set_pitch(
        self, g_value_get_object(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_gesture_momentum_class_init(
  MaplibreNativeFfiGestureMomentumClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_gesture_momentum_finalize;
  object_class->get_property =
    maplibre_native_ffi_gesture_momentum_get_property;
  object_class->set_property =
    maplibre_native_ffi_gesture_momentum_set_property;
  gesture_momentum_properties[GESTURE_MOMENTUM_PROP_PAN] = g_param_spec_object(
    "pan", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_PAN_MOMENTUM, PROPERTY_FLAGS
  );
  gesture_momentum_properties[GESTURE_MOMENTUM_PROP_ZOOM] = g_param_spec_object(
    "zoom", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_ANCHORED_MOMENTUM,
    PROPERTY_FLAGS
  );
  gesture_momentum_properties[GESTURE_MOMENTUM_PROP_BEARING] =
    g_param_spec_object(
      "bearing", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_ANCHORED_MOMENTUM,
      PROPERTY_FLAGS
    );
  gesture_momentum_properties[GESTURE_MOMENTUM_PROP_PITCH] =
    g_param_spec_object(
      "pitch", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_ANCHORED_MOMENTUM,
      PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, GESTURE_MOMENTUM_N_PROPERTIES, gesture_momentum_properties
  );
}

static void maplibre_native_ffi_gesture_momentum_init(
  MaplibreNativeFfiGestureMomentum* self
) {
  self->pan = maplibre_native_ffi_pan_momentum_new();
  self->zoom = maplibre_native_ffi_anchored_momentum_new();
  self->bearing = maplibre_native_ffi_anchored_momentum_new();
  self->pitch = maplibre_native_ffi_anchored_momentum_new();
}

MaplibreNativeFfiGestureMomentum* maplibre_native_ffi_gesture_momentum_new(
  void
) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_GESTURE_MOMENTUM, NULL);
}

MaplibreNativeFfiPanMomentum* maplibre_native_ffi_gesture_momentum_get_pan(
  MaplibreNativeFfiGestureMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_GESTURE_MOMENTUM(self), NULL);
  return self->pan;
}

void maplibre_native_ffi_gesture_momentum_set_pan(
  MaplibreNativeFfiGestureMomentum* self, MaplibreNativeFfiPanMomentum* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GESTURE_MOMENTUM(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PAN_MOMENTUM(value));
  MaplibreNativeFfiPanMomentum* previous = self->pan;
  self->pan = maplibre_native_ffi_pan_momentum_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), gesture_momentum_properties[GESTURE_MOMENTUM_PROP_PAN]
  );
}

MaplibreNativeFfiAnchoredMomentum*
maplibre_native_ffi_gesture_momentum_get_zoom(
  MaplibreNativeFfiGestureMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_GESTURE_MOMENTUM(self), NULL);
  return self->zoom;
}

void maplibre_native_ffi_gesture_momentum_set_zoom(
  MaplibreNativeFfiGestureMomentum* self,
  MaplibreNativeFfiAnchoredMomentum* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GESTURE_MOMENTUM(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(value));
  MaplibreNativeFfiAnchoredMomentum* previous = self->zoom;
  self->zoom = maplibre_native_ffi_anchored_momentum_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), gesture_momentum_properties[GESTURE_MOMENTUM_PROP_ZOOM]
  );
}

MaplibreNativeFfiAnchoredMomentum*
maplibre_native_ffi_gesture_momentum_get_bearing(
  MaplibreNativeFfiGestureMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_GESTURE_MOMENTUM(self), NULL);
  return self->bearing;
}

void maplibre_native_ffi_gesture_momentum_set_bearing(
  MaplibreNativeFfiGestureMomentum* self,
  MaplibreNativeFfiAnchoredMomentum* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GESTURE_MOMENTUM(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(value));
  MaplibreNativeFfiAnchoredMomentum* previous = self->bearing;
  self->bearing = maplibre_native_ffi_anchored_momentum_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), gesture_momentum_properties[GESTURE_MOMENTUM_PROP_BEARING]
  );
}

MaplibreNativeFfiAnchoredMomentum*
maplibre_native_ffi_gesture_momentum_get_pitch(
  MaplibreNativeFfiGestureMomentum* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_GESTURE_MOMENTUM(self), NULL);
  return self->pitch;
}

void maplibre_native_ffi_gesture_momentum_set_pitch(
  MaplibreNativeFfiGestureMomentum* self,
  MaplibreNativeFfiAnchoredMomentum* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GESTURE_MOMENTUM(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANCHORED_MOMENTUM(value));
  MaplibreNativeFfiAnchoredMomentum* previous = self->pitch;
  self->pitch = maplibre_native_ffi_anchored_momentum_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), gesture_momentum_properties[GESTURE_MOMENTUM_PROP_PITCH]
  );
}

MaplibreNativeFfiGestureMomentum* maplibre_native_ffi_gesture_momentum_copy(
  const MaplibreNativeFfiGestureMomentum* self
) {
  MaplibreNativeFfiGestureMomentum* copy =
    maplibre_native_ffi_gesture_momentum_new();
  g_clear_object(&copy->pan);
  copy->pan =
    self->pan != NULL ? maplibre_native_ffi_pan_momentum_copy(self->pan) : NULL;
  g_clear_object(&copy->zoom);
  copy->zoom = self->zoom != NULL
                 ? maplibre_native_ffi_anchored_momentum_copy(self->zoom)
                 : NULL;
  g_clear_object(&copy->bearing);
  copy->bearing = self->bearing != NULL
                    ? maplibre_native_ffi_anchored_momentum_copy(self->bearing)
                    : NULL;
  g_clear_object(&copy->pitch);
  copy->pitch = self->pitch != NULL
                  ? maplibre_native_ffi_anchored_momentum_copy(self->pitch)
                  : NULL;
  return copy;
}

void maplibre_native_ffi_gesture_momentum_to_native(
  const MaplibreNativeFfiGestureMomentum* self, mln_gesture_momentum* out,
  MaplibreNativeFfiCall* call
) {
  memset(out, 0, sizeof *out);
  if (self->pan != NULL)
    maplibre_native_ffi_pan_momentum_to_native(self->pan, &out->pan, call);
  if (self->zoom != NULL)
    maplibre_native_ffi_anchored_momentum_to_native(
      self->zoom, &out->zoom, call
    );
  if (self->bearing != NULL)
    maplibre_native_ffi_anchored_momentum_to_native(
      self->bearing, &out->bearing, call
    );
  if (self->pitch != NULL)
    maplibre_native_ffi_anchored_momentum_to_native(
      self->pitch, &out->pitch, call
    );
}

struct _MaplibreNativeFfiCameraDelta {
  GObject parent_instance;
  guint64 fields;
  MaplibreNativeFfiScreenPoint offset;
  gdouble scale;
  gdouble bearing;
  gdouble pitch;
  MaplibreNativeFfiScreenPoint anchor;
  MaplibreNativeFfiGestureMomentum* momentum;
  MaplibreNativeFfiAnimationOptions* animation;
  MaplibreNativeFfiGesturePhase gesture_phase;
};

enum {
  CAMERA_DELTA_PROP_0,
  CAMERA_DELTA_PROP_OFFSET,
  CAMERA_DELTA_PROP_SCALE,
  CAMERA_DELTA_PROP_BEARING,
  CAMERA_DELTA_PROP_PITCH,
  CAMERA_DELTA_PROP_ANCHOR,
  CAMERA_DELTA_PROP_MOMENTUM,
  CAMERA_DELTA_PROP_ANIMATION,
  CAMERA_DELTA_PROP_GESTURE_PHASE,
  CAMERA_DELTA_N_PROPERTIES
};

static GParamSpec* camera_delta_properties[CAMERA_DELTA_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiCameraDelta, maplibre_native_ffi_camera_delta, G_TYPE_OBJECT
)

static void maplibre_native_ffi_camera_delta_finalize(GObject* object) {
  MaplibreNativeFfiCameraDelta* self = MAPLIBRE_NATIVE_FFI_CAMERA_DELTA(object);
  g_clear_object(&self->momentum);
  g_clear_object(&self->animation);
  G_OBJECT_CLASS(maplibre_native_ffi_camera_delta_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_camera_delta_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCameraDelta* self = MAPLIBRE_NATIVE_FFI_CAMERA_DELTA(object);
  switch (id) {
    case CAMERA_DELTA_PROP_OFFSET:
      g_value_set_boxed(value, &self->offset);
      break;
    case CAMERA_DELTA_PROP_SCALE:
      g_value_set_double(value, self->scale);
      break;
    case CAMERA_DELTA_PROP_BEARING:
      g_value_set_double(value, self->bearing);
      break;
    case CAMERA_DELTA_PROP_PITCH:
      g_value_set_double(value, self->pitch);
      break;
    case CAMERA_DELTA_PROP_ANCHOR:
      g_value_set_boxed(value, &self->anchor);
      break;
    case CAMERA_DELTA_PROP_MOMENTUM:
      g_value_set_object(value, self->momentum);
      break;
    case CAMERA_DELTA_PROP_ANIMATION:
      g_value_set_object(value, self->animation);
      break;
    case CAMERA_DELTA_PROP_GESTURE_PHASE:
      g_value_set_enum(value, self->gesture_phase);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_camera_delta_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCameraDelta* self = MAPLIBRE_NATIVE_FFI_CAMERA_DELTA(object);
  switch (id) {
    case CAMERA_DELTA_PROP_OFFSET:
      maplibre_native_ffi_camera_delta_set_offset(
        self, g_value_get_boxed(value)
      );
      break;
    case CAMERA_DELTA_PROP_SCALE:
      maplibre_native_ffi_camera_delta_set_scale(
        self, g_value_get_double(value)
      );
      break;
    case CAMERA_DELTA_PROP_BEARING:
      maplibre_native_ffi_camera_delta_set_bearing(
        self, g_value_get_double(value)
      );
      break;
    case CAMERA_DELTA_PROP_PITCH:
      maplibre_native_ffi_camera_delta_set_pitch(
        self, g_value_get_double(value)
      );
      break;
    case CAMERA_DELTA_PROP_ANCHOR:
      maplibre_native_ffi_camera_delta_set_anchor(
        self, g_value_get_boxed(value)
      );
      break;
    case CAMERA_DELTA_PROP_MOMENTUM:
      maplibre_native_ffi_camera_delta_set_momentum(
        self, g_value_get_object(value)
      );
      break;
    case CAMERA_DELTA_PROP_ANIMATION:
      maplibre_native_ffi_camera_delta_set_animation(
        self, g_value_get_object(value)
      );
      break;
    case CAMERA_DELTA_PROP_GESTURE_PHASE:
      maplibre_native_ffi_camera_delta_set_gesture_phase(
        self, g_value_get_enum(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_camera_delta_class_init(
  MaplibreNativeFfiCameraDeltaClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_camera_delta_finalize;
  object_class->get_property = maplibre_native_ffi_camera_delta_get_property;
  object_class->set_property = maplibre_native_ffi_camera_delta_set_property;
  camera_delta_properties[CAMERA_DELTA_PROP_OFFSET] = g_param_spec_boxed(
    "offset", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_SCREEN_POINT, PROPERTY_FLAGS
  );
  camera_delta_properties[CAMERA_DELTA_PROP_SCALE] = g_param_spec_double(
    "scale", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  camera_delta_properties[CAMERA_DELTA_PROP_BEARING] = g_param_spec_double(
    "bearing", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  camera_delta_properties[CAMERA_DELTA_PROP_PITCH] = g_param_spec_double(
    "pitch", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  camera_delta_properties[CAMERA_DELTA_PROP_ANCHOR] = g_param_spec_boxed(
    "anchor", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_SCREEN_POINT, PROPERTY_FLAGS
  );
  camera_delta_properties[CAMERA_DELTA_PROP_MOMENTUM] = g_param_spec_object(
    "momentum", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_GESTURE_MOMENTUM,
    PROPERTY_FLAGS
  );
  camera_delta_properties[CAMERA_DELTA_PROP_ANIMATION] = g_param_spec_object(
    "animation", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_ANIMATION_OPTIONS,
    PROPERTY_FLAGS
  );
  camera_delta_properties[CAMERA_DELTA_PROP_GESTURE_PHASE] = g_param_spec_enum(
    "gesture-phase", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_GESTURE_PHASE, 0,
    PROPERTY_FLAGS
  );
  g_object_class_install_properties(
    object_class, CAMERA_DELTA_N_PROPERTIES, camera_delta_properties
  );
}

static void maplibre_native_ffi_camera_delta_init(
  MaplibreNativeFfiCameraDelta* self
) {
  self->momentum = maplibre_native_ffi_gesture_momentum_new();
  self->animation = maplibre_native_ffi_animation_options_new();
}

MaplibreNativeFfiCameraDelta* maplibre_native_ffi_camera_delta_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_DELTA, NULL);
}

MaplibreNativeFfiScreenPoint* maplibre_native_ffi_camera_delta_get_offset(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), NULL);
  return (MaplibreNativeFfiScreenPoint*)&self->offset;
}

gboolean maplibre_native_ffi_camera_delta_has_offset(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), FALSE);
  return (self->fields & MLN_CAMERA_DELTA_OFFSET) != 0;
}

void maplibre_native_ffi_camera_delta_set_offset(
  MaplibreNativeFfiCameraDelta* self, const MaplibreNativeFfiScreenPoint* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_CAMERA_DELTA_OFFSET) != 0;
  gboolean changed = memcmp(&self->offset, value, sizeof self->offset) != 0;
  self->offset = *value;
  self->fields |= MLN_CAMERA_DELTA_OFFSET;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_delta_properties[CAMERA_DELTA_PROP_OFFSET]
    );
}

gdouble maplibre_native_ffi_camera_delta_get_scale(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), 0);
  return self->scale;
}

gboolean maplibre_native_ffi_camera_delta_has_scale(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), FALSE);
  return (self->fields & MLN_CAMERA_DELTA_SCALE) != 0;
}

void maplibre_native_ffi_camera_delta_set_scale(
  MaplibreNativeFfiCameraDelta* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self));
  gboolean was_set = (self->fields & MLN_CAMERA_DELTA_SCALE) != 0;
  gboolean changed = self->scale != value;
  self->scale = value;
  self->fields |= MLN_CAMERA_DELTA_SCALE;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_delta_properties[CAMERA_DELTA_PROP_SCALE]
    );
}

gdouble maplibre_native_ffi_camera_delta_get_bearing(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), 0);
  return self->bearing;
}

gboolean maplibre_native_ffi_camera_delta_has_bearing(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), FALSE);
  return (self->fields & MLN_CAMERA_DELTA_BEARING) != 0;
}

void maplibre_native_ffi_camera_delta_set_bearing(
  MaplibreNativeFfiCameraDelta* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self));
  gboolean was_set = (self->fields & MLN_CAMERA_DELTA_BEARING) != 0;
  gboolean changed = self->bearing != value;
  self->bearing = value;
  self->fields |= MLN_CAMERA_DELTA_BEARING;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_delta_properties[CAMERA_DELTA_PROP_BEARING]
    );
}

gdouble maplibre_native_ffi_camera_delta_get_pitch(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), 0);
  return self->pitch;
}

gboolean maplibre_native_ffi_camera_delta_has_pitch(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), FALSE);
  return (self->fields & MLN_CAMERA_DELTA_PITCH) != 0;
}

void maplibre_native_ffi_camera_delta_set_pitch(
  MaplibreNativeFfiCameraDelta* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self));
  gboolean was_set = (self->fields & MLN_CAMERA_DELTA_PITCH) != 0;
  gboolean changed = self->pitch != value;
  self->pitch = value;
  self->fields |= MLN_CAMERA_DELTA_PITCH;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_delta_properties[CAMERA_DELTA_PROP_PITCH]
    );
}

MaplibreNativeFfiScreenPoint* maplibre_native_ffi_camera_delta_get_anchor(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), NULL);
  return (MaplibreNativeFfiScreenPoint*)&self->anchor;
}

gboolean maplibre_native_ffi_camera_delta_has_anchor(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), FALSE);
  return (self->fields & MLN_CAMERA_DELTA_ANCHOR) != 0;
}

void maplibre_native_ffi_camera_delta_set_anchor(
  MaplibreNativeFfiCameraDelta* self, const MaplibreNativeFfiScreenPoint* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_CAMERA_DELTA_ANCHOR) != 0;
  gboolean changed = memcmp(&self->anchor, value, sizeof self->anchor) != 0;
  self->anchor = *value;
  self->fields |= MLN_CAMERA_DELTA_ANCHOR;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_delta_properties[CAMERA_DELTA_PROP_ANCHOR]
    );
}

MaplibreNativeFfiGestureMomentum* maplibre_native_ffi_camera_delta_get_momentum(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), NULL);
  return self->momentum;
}

gboolean maplibre_native_ffi_camera_delta_has_momentum(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), FALSE);
  return (self->fields & MLN_CAMERA_DELTA_MOMENTUM) != 0;
}

void maplibre_native_ffi_camera_delta_set_momentum(
  MaplibreNativeFfiCameraDelta* self, MaplibreNativeFfiGestureMomentum* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GESTURE_MOMENTUM(value));
  MaplibreNativeFfiGestureMomentum* previous = self->momentum;
  self->momentum = maplibre_native_ffi_gesture_momentum_copy(value);
  if (previous != NULL) g_object_unref(previous);
  self->fields |= MLN_CAMERA_DELTA_MOMENTUM;
  g_object_notify_by_pspec(
    G_OBJECT(self), camera_delta_properties[CAMERA_DELTA_PROP_MOMENTUM]
  );
}

MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_camera_delta_get_animation(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), NULL);
  return self->animation;
}

void maplibre_native_ffi_camera_delta_set_animation(
  MaplibreNativeFfiCameraDelta* self, MaplibreNativeFfiAnimationOptions* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(value));
  MaplibreNativeFfiAnimationOptions* previous = self->animation;
  self->animation = maplibre_native_ffi_animation_options_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), camera_delta_properties[CAMERA_DELTA_PROP_ANIMATION]
  );
}

MaplibreNativeFfiGesturePhase
maplibre_native_ffi_camera_delta_get_gesture_phase(
  MaplibreNativeFfiCameraDelta* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self), (MaplibreNativeFfiGesturePhase)0
  );
  return self->gesture_phase;
}

void maplibre_native_ffi_camera_delta_set_gesture_phase(
  MaplibreNativeFfiCameraDelta* self, MaplibreNativeFfiGesturePhase value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(self));
  gboolean changed = self->gesture_phase != value;
  self->gesture_phase = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_delta_properties[CAMERA_DELTA_PROP_GESTURE_PHASE]
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
  if (self->momentum != NULL)
    maplibre_native_ffi_gesture_momentum_to_native(
      self->momentum, &out->momentum, call
    );
  if (self->animation != NULL)
    maplibre_native_ffi_animation_options_to_native(
      self->animation, &out->animation, call
    );
  out->gesture_phase = (mln_gesture_phase)(self->gesture_phase);
}

struct _MaplibreNativeFfiCameraFitOptions {
  GObject parent_instance;
  guint64 fields;
  MaplibreNativeFfiEdgeInsets padding;
  gdouble bearing;
  gdouble pitch;
};

enum {
  CAMERA_FIT_OPTIONS_PROP_0,
  CAMERA_FIT_OPTIONS_PROP_PADDING,
  CAMERA_FIT_OPTIONS_PROP_BEARING,
  CAMERA_FIT_OPTIONS_PROP_PITCH,
  CAMERA_FIT_OPTIONS_N_PROPERTIES
};

static GParamSpec*
  camera_fit_options_properties[CAMERA_FIT_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiCameraFitOptions, maplibre_native_ffi_camera_fit_options,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_camera_fit_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCameraFitOptions* self =
    MAPLIBRE_NATIVE_FFI_CAMERA_FIT_OPTIONS(object);
  switch (id) {
    case CAMERA_FIT_OPTIONS_PROP_PADDING:
      g_value_set_boxed(value, &self->padding);
      break;
    case CAMERA_FIT_OPTIONS_PROP_BEARING:
      g_value_set_double(value, self->bearing);
      break;
    case CAMERA_FIT_OPTIONS_PROP_PITCH:
      g_value_set_double(value, self->pitch);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_camera_fit_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCameraFitOptions* self =
    MAPLIBRE_NATIVE_FFI_CAMERA_FIT_OPTIONS(object);
  switch (id) {
    case CAMERA_FIT_OPTIONS_PROP_PADDING:
      maplibre_native_ffi_camera_fit_options_set_padding(
        self, g_value_get_boxed(value)
      );
      break;
    case CAMERA_FIT_OPTIONS_PROP_BEARING:
      maplibre_native_ffi_camera_fit_options_set_bearing(
        self, g_value_get_double(value)
      );
      break;
    case CAMERA_FIT_OPTIONS_PROP_PITCH:
      maplibre_native_ffi_camera_fit_options_set_pitch(
        self, g_value_get_double(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_camera_fit_options_class_init(
  MaplibreNativeFfiCameraFitOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_camera_fit_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_camera_fit_options_set_property;
  camera_fit_options_properties[CAMERA_FIT_OPTIONS_PROP_PADDING] =
    g_param_spec_boxed(
      "padding", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_EDGE_INSETS,
      PROPERTY_FLAGS
    );
  camera_fit_options_properties[CAMERA_FIT_OPTIONS_PROP_BEARING] =
    g_param_spec_double(
      "bearing", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  camera_fit_options_properties[CAMERA_FIT_OPTIONS_PROP_PITCH] =
    g_param_spec_double(
      "pitch", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, CAMERA_FIT_OPTIONS_N_PROPERTIES, camera_fit_options_properties
  );
}

static void maplibre_native_ffi_camera_fit_options_init(
  MaplibreNativeFfiCameraFitOptions* self
) {
  (void)self;
}

MaplibreNativeFfiCameraFitOptions* maplibre_native_ffi_camera_fit_options_new(
  void
) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_FIT_OPTIONS, NULL);
}

MaplibreNativeFfiEdgeInsets* maplibre_native_ffi_camera_fit_options_get_padding(
  MaplibreNativeFfiCameraFitOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_OPTIONS(self), NULL);
  return (MaplibreNativeFfiEdgeInsets*)&self->padding;
}

gboolean maplibre_native_ffi_camera_fit_options_has_padding(
  MaplibreNativeFfiCameraFitOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_OPTIONS(self), FALSE);
  return (self->fields & MLN_CAMERA_FIT_OPTION_PADDING) != 0;
}

void maplibre_native_ffi_camera_fit_options_set_padding(
  MaplibreNativeFfiCameraFitOptions* self,
  const MaplibreNativeFfiEdgeInsets* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_OPTIONS(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_CAMERA_FIT_OPTION_PADDING) != 0;
  gboolean changed = memcmp(&self->padding, value, sizeof self->padding) != 0;
  self->padding = *value;
  self->fields |= MLN_CAMERA_FIT_OPTION_PADDING;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      camera_fit_options_properties[CAMERA_FIT_OPTIONS_PROP_PADDING]
    );
}

gdouble maplibre_native_ffi_camera_fit_options_get_bearing(
  MaplibreNativeFfiCameraFitOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_OPTIONS(self), 0);
  return self->bearing;
}

gboolean maplibre_native_ffi_camera_fit_options_has_bearing(
  MaplibreNativeFfiCameraFitOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_OPTIONS(self), FALSE);
  return (self->fields & MLN_CAMERA_FIT_OPTION_BEARING) != 0;
}

void maplibre_native_ffi_camera_fit_options_set_bearing(
  MaplibreNativeFfiCameraFitOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_CAMERA_FIT_OPTION_BEARING) != 0;
  gboolean changed = self->bearing != value;
  self->bearing = value;
  self->fields |= MLN_CAMERA_FIT_OPTION_BEARING;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      camera_fit_options_properties[CAMERA_FIT_OPTIONS_PROP_BEARING]
    );
}

gdouble maplibre_native_ffi_camera_fit_options_get_pitch(
  MaplibreNativeFfiCameraFitOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_OPTIONS(self), 0);
  return self->pitch;
}

gboolean maplibre_native_ffi_camera_fit_options_has_pitch(
  MaplibreNativeFfiCameraFitOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_OPTIONS(self), FALSE);
  return (self->fields & MLN_CAMERA_FIT_OPTION_PITCH) != 0;
}

void maplibre_native_ffi_camera_fit_options_set_pitch(
  MaplibreNativeFfiCameraFitOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_CAMERA_FIT_OPTION_PITCH) != 0;
  gboolean changed = self->pitch != value;
  self->pitch = value;
  self->fields |= MLN_CAMERA_FIT_OPTION_PITCH;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      camera_fit_options_properties[CAMERA_FIT_OPTIONS_PROP_PITCH]
    );
}

MaplibreNativeFfiCameraFitOptions* maplibre_native_ffi_camera_fit_options_copy(
  const MaplibreNativeFfiCameraFitOptions* self
) {
  MaplibreNativeFfiCameraFitOptions* copy =
    maplibre_native_ffi_camera_fit_options_new();
  copy->fields = self->fields;
  copy->padding = self->padding;
  copy->bearing = self->bearing;
  copy->pitch = self->pitch;
  return copy;
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

struct _MaplibreNativeFfiCameraFitUpdate {
  GObject parent_instance;
  MaplibreNativeFfiCameraUpdateMode mode;
  MaplibreNativeFfiCameraFitOptions* fit;
  MaplibreNativeFfiAnimationOptions* animation;
};

enum {
  CAMERA_FIT_UPDATE_PROP_0,
  CAMERA_FIT_UPDATE_PROP_MODE,
  CAMERA_FIT_UPDATE_PROP_FIT,
  CAMERA_FIT_UPDATE_PROP_ANIMATION,
  CAMERA_FIT_UPDATE_N_PROPERTIES
};

static GParamSpec* camera_fit_update_properties[CAMERA_FIT_UPDATE_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiCameraFitUpdate, maplibre_native_ffi_camera_fit_update,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_camera_fit_update_finalize(GObject* object) {
  MaplibreNativeFfiCameraFitUpdate* self =
    MAPLIBRE_NATIVE_FFI_CAMERA_FIT_UPDATE(object);
  g_clear_object(&self->fit);
  g_clear_object(&self->animation);
  G_OBJECT_CLASS(maplibre_native_ffi_camera_fit_update_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_camera_fit_update_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCameraFitUpdate* self =
    MAPLIBRE_NATIVE_FFI_CAMERA_FIT_UPDATE(object);
  switch (id) {
    case CAMERA_FIT_UPDATE_PROP_MODE:
      g_value_set_enum(value, self->mode);
      break;
    case CAMERA_FIT_UPDATE_PROP_FIT:
      g_value_set_object(value, self->fit);
      break;
    case CAMERA_FIT_UPDATE_PROP_ANIMATION:
      g_value_set_object(value, self->animation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_camera_fit_update_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCameraFitUpdate* self =
    MAPLIBRE_NATIVE_FFI_CAMERA_FIT_UPDATE(object);
  switch (id) {
    case CAMERA_FIT_UPDATE_PROP_MODE:
      maplibre_native_ffi_camera_fit_update_set_mode(
        self, g_value_get_enum(value)
      );
      break;
    case CAMERA_FIT_UPDATE_PROP_FIT:
      maplibre_native_ffi_camera_fit_update_set_fit(
        self, g_value_get_object(value)
      );
      break;
    case CAMERA_FIT_UPDATE_PROP_ANIMATION:
      maplibre_native_ffi_camera_fit_update_set_animation(
        self, g_value_get_object(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_camera_fit_update_class_init(
  MaplibreNativeFfiCameraFitUpdateClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_camera_fit_update_finalize;
  object_class->get_property =
    maplibre_native_ffi_camera_fit_update_get_property;
  object_class->set_property =
    maplibre_native_ffi_camera_fit_update_set_property;
  camera_fit_update_properties[CAMERA_FIT_UPDATE_PROP_MODE] = g_param_spec_enum(
    "mode", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_UPDATE_MODE, 0,
    PROPERTY_FLAGS
  );
  camera_fit_update_properties[CAMERA_FIT_UPDATE_PROP_FIT] =
    g_param_spec_object(
      "fit", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_FIT_OPTIONS,
      PROPERTY_FLAGS
    );
  camera_fit_update_properties[CAMERA_FIT_UPDATE_PROP_ANIMATION] =
    g_param_spec_object(
      "animation", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_ANIMATION_OPTIONS,
      PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, CAMERA_FIT_UPDATE_N_PROPERTIES, camera_fit_update_properties
  );
}

static void maplibre_native_ffi_camera_fit_update_init(
  MaplibreNativeFfiCameraFitUpdate* self
) {
  self->fit = maplibre_native_ffi_camera_fit_options_new();
  self->animation = maplibre_native_ffi_animation_options_new();
}

MaplibreNativeFfiCameraFitUpdate* maplibre_native_ffi_camera_fit_update_new(
  void
) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_FIT_UPDATE, NULL);
}

MaplibreNativeFfiCameraUpdateMode
maplibre_native_ffi_camera_fit_update_get_mode(
  MaplibreNativeFfiCameraFitUpdate* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_UPDATE(self),
    (MaplibreNativeFfiCameraUpdateMode)0
  );
  return self->mode;
}

void maplibre_native_ffi_camera_fit_update_set_mode(
  MaplibreNativeFfiCameraFitUpdate* self,
  MaplibreNativeFfiCameraUpdateMode value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_UPDATE(self));
  gboolean changed = self->mode != value;
  self->mode = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_fit_update_properties[CAMERA_FIT_UPDATE_PROP_MODE]
    );
}

MaplibreNativeFfiCameraFitOptions*
maplibre_native_ffi_camera_fit_update_get_fit(
  MaplibreNativeFfiCameraFitUpdate* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_UPDATE(self), NULL);
  return self->fit;
}

void maplibre_native_ffi_camera_fit_update_set_fit(
  MaplibreNativeFfiCameraFitUpdate* self,
  MaplibreNativeFfiCameraFitOptions* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_UPDATE(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_OPTIONS(value));
  MaplibreNativeFfiCameraFitOptions* previous = self->fit;
  self->fit = maplibre_native_ffi_camera_fit_options_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), camera_fit_update_properties[CAMERA_FIT_UPDATE_PROP_FIT]
  );
}

MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_camera_fit_update_get_animation(
  MaplibreNativeFfiCameraFitUpdate* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_UPDATE(self), NULL);
  return self->animation;
}

void maplibre_native_ffi_camera_fit_update_set_animation(
  MaplibreNativeFfiCameraFitUpdate* self,
  MaplibreNativeFfiAnimationOptions* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_UPDATE(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(value));
  MaplibreNativeFfiAnimationOptions* previous = self->animation;
  self->animation = maplibre_native_ffi_animation_options_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self),
    camera_fit_update_properties[CAMERA_FIT_UPDATE_PROP_ANIMATION]
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

struct _MaplibreNativeFfiCameraOptions {
  GObject parent_instance;
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

enum {
  CAMERA_OPTIONS_PROP_0,
  CAMERA_OPTIONS_PROP_CENTER,
  CAMERA_OPTIONS_PROP_CENTER_ALTITUDE,
  CAMERA_OPTIONS_PROP_PADDING,
  CAMERA_OPTIONS_PROP_ANCHOR,
  CAMERA_OPTIONS_PROP_ZOOM,
  CAMERA_OPTIONS_PROP_BEARING,
  CAMERA_OPTIONS_PROP_PITCH,
  CAMERA_OPTIONS_PROP_ROLL,
  CAMERA_OPTIONS_PROP_FIELD_OF_VIEW,
  CAMERA_OPTIONS_N_PROPERTIES
};

static GParamSpec* camera_options_properties[CAMERA_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiCameraOptions, maplibre_native_ffi_camera_options,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_camera_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCameraOptions* self =
    MAPLIBRE_NATIVE_FFI_CAMERA_OPTIONS(object);
  switch (id) {
    case CAMERA_OPTIONS_PROP_CENTER:
      g_value_set_boxed(value, &self->center);
      break;
    case CAMERA_OPTIONS_PROP_CENTER_ALTITUDE:
      g_value_set_double(value, self->center_altitude);
      break;
    case CAMERA_OPTIONS_PROP_PADDING:
      g_value_set_boxed(value, &self->padding);
      break;
    case CAMERA_OPTIONS_PROP_ANCHOR:
      g_value_set_boxed(value, &self->anchor);
      break;
    case CAMERA_OPTIONS_PROP_ZOOM:
      g_value_set_double(value, self->zoom);
      break;
    case CAMERA_OPTIONS_PROP_BEARING:
      g_value_set_double(value, self->bearing);
      break;
    case CAMERA_OPTIONS_PROP_PITCH:
      g_value_set_double(value, self->pitch);
      break;
    case CAMERA_OPTIONS_PROP_ROLL:
      g_value_set_double(value, self->roll);
      break;
    case CAMERA_OPTIONS_PROP_FIELD_OF_VIEW:
      g_value_set_double(value, self->field_of_view);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_camera_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCameraOptions* self =
    MAPLIBRE_NATIVE_FFI_CAMERA_OPTIONS(object);
  switch (id) {
    case CAMERA_OPTIONS_PROP_CENTER:
      maplibre_native_ffi_camera_options_set_center(
        self, g_value_get_boxed(value)
      );
      break;
    case CAMERA_OPTIONS_PROP_CENTER_ALTITUDE:
      maplibre_native_ffi_camera_options_set_center_altitude(
        self, g_value_get_double(value)
      );
      break;
    case CAMERA_OPTIONS_PROP_PADDING:
      maplibre_native_ffi_camera_options_set_padding(
        self, g_value_get_boxed(value)
      );
      break;
    case CAMERA_OPTIONS_PROP_ANCHOR:
      maplibre_native_ffi_camera_options_set_anchor(
        self, g_value_get_boxed(value)
      );
      break;
    case CAMERA_OPTIONS_PROP_ZOOM:
      maplibre_native_ffi_camera_options_set_zoom(
        self, g_value_get_double(value)
      );
      break;
    case CAMERA_OPTIONS_PROP_BEARING:
      maplibre_native_ffi_camera_options_set_bearing(
        self, g_value_get_double(value)
      );
      break;
    case CAMERA_OPTIONS_PROP_PITCH:
      maplibre_native_ffi_camera_options_set_pitch(
        self, g_value_get_double(value)
      );
      break;
    case CAMERA_OPTIONS_PROP_ROLL:
      maplibre_native_ffi_camera_options_set_roll(
        self, g_value_get_double(value)
      );
      break;
    case CAMERA_OPTIONS_PROP_FIELD_OF_VIEW:
      maplibre_native_ffi_camera_options_set_field_of_view(
        self, g_value_get_double(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_camera_options_class_init(
  MaplibreNativeFfiCameraOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property = maplibre_native_ffi_camera_options_get_property;
  object_class->set_property = maplibre_native_ffi_camera_options_set_property;
  camera_options_properties[CAMERA_OPTIONS_PROP_CENTER] = g_param_spec_boxed(
    "center", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LAT_LNG, PROPERTY_FLAGS
  );
  camera_options_properties[CAMERA_OPTIONS_PROP_CENTER_ALTITUDE] =
    g_param_spec_double(
      "center-altitude", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  camera_options_properties[CAMERA_OPTIONS_PROP_PADDING] = g_param_spec_boxed(
    "padding", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_EDGE_INSETS, PROPERTY_FLAGS
  );
  camera_options_properties[CAMERA_OPTIONS_PROP_ANCHOR] = g_param_spec_boxed(
    "anchor", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_SCREEN_POINT, PROPERTY_FLAGS
  );
  camera_options_properties[CAMERA_OPTIONS_PROP_ZOOM] = g_param_spec_double(
    "zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  camera_options_properties[CAMERA_OPTIONS_PROP_BEARING] = g_param_spec_double(
    "bearing", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  camera_options_properties[CAMERA_OPTIONS_PROP_PITCH] = g_param_spec_double(
    "pitch", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  camera_options_properties[CAMERA_OPTIONS_PROP_ROLL] = g_param_spec_double(
    "roll", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
  );
  camera_options_properties[CAMERA_OPTIONS_PROP_FIELD_OF_VIEW] =
    g_param_spec_double(
      "field-of-view", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, CAMERA_OPTIONS_N_PROPERTIES, camera_options_properties
  );
}

static void maplibre_native_ffi_camera_options_init(
  MaplibreNativeFfiCameraOptions* self
) {
  (void)self;
}

MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_options_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_OPTIONS, NULL);
}

MaplibreNativeFfiLatLng* maplibre_native_ffi_camera_options_get_center(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), NULL);
  return (MaplibreNativeFfiLatLng*)&self->center;
}

gboolean maplibre_native_ffi_camera_options_has_center(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), FALSE);
  return (self->fields & MLN_CAMERA_OPTION_CENTER) != 0;
}

void maplibre_native_ffi_camera_options_set_center(
  MaplibreNativeFfiCameraOptions* self, const MaplibreNativeFfiLatLng* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_CAMERA_OPTION_CENTER) != 0;
  gboolean changed = memcmp(&self->center, value, sizeof self->center) != 0;
  self->center = *value;
  self->fields |= MLN_CAMERA_OPTION_CENTER;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_options_properties[CAMERA_OPTIONS_PROP_CENTER]
    );
}

gdouble maplibre_native_ffi_camera_options_get_center_altitude(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), 0);
  return self->center_altitude;
}

gboolean maplibre_native_ffi_camera_options_has_center_altitude(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), FALSE);
  return (self->fields & MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0;
}

void maplibre_native_ffi_camera_options_set_center_altitude(
  MaplibreNativeFfiCameraOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0;
  gboolean changed = self->center_altitude != value;
  self->center_altitude = value;
  self->fields |= MLN_CAMERA_OPTION_CENTER_ALTITUDE;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      camera_options_properties[CAMERA_OPTIONS_PROP_CENTER_ALTITUDE]
    );
}

MaplibreNativeFfiEdgeInsets* maplibre_native_ffi_camera_options_get_padding(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), NULL);
  return (MaplibreNativeFfiEdgeInsets*)&self->padding;
}

gboolean maplibre_native_ffi_camera_options_has_padding(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), FALSE);
  return (self->fields & MLN_CAMERA_OPTION_PADDING) != 0;
}

void maplibre_native_ffi_camera_options_set_padding(
  MaplibreNativeFfiCameraOptions* self, const MaplibreNativeFfiEdgeInsets* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_CAMERA_OPTION_PADDING) != 0;
  gboolean changed = memcmp(&self->padding, value, sizeof self->padding) != 0;
  self->padding = *value;
  self->fields |= MLN_CAMERA_OPTION_PADDING;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_options_properties[CAMERA_OPTIONS_PROP_PADDING]
    );
}

MaplibreNativeFfiScreenPoint* maplibre_native_ffi_camera_options_get_anchor(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), NULL);
  return (MaplibreNativeFfiScreenPoint*)&self->anchor;
}

gboolean maplibre_native_ffi_camera_options_has_anchor(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), FALSE);
  return (self->fields & MLN_CAMERA_OPTION_ANCHOR) != 0;
}

void maplibre_native_ffi_camera_options_set_anchor(
  MaplibreNativeFfiCameraOptions* self,
  const MaplibreNativeFfiScreenPoint* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_CAMERA_OPTION_ANCHOR) != 0;
  gboolean changed = memcmp(&self->anchor, value, sizeof self->anchor) != 0;
  self->anchor = *value;
  self->fields |= MLN_CAMERA_OPTION_ANCHOR;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_options_properties[CAMERA_OPTIONS_PROP_ANCHOR]
    );
}

gdouble maplibre_native_ffi_camera_options_get_zoom(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), 0);
  return self->zoom;
}

gboolean maplibre_native_ffi_camera_options_has_zoom(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), FALSE);
  return (self->fields & MLN_CAMERA_OPTION_ZOOM) != 0;
}

void maplibre_native_ffi_camera_options_set_zoom(
  MaplibreNativeFfiCameraOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_CAMERA_OPTION_ZOOM) != 0;
  gboolean changed = self->zoom != value;
  self->zoom = value;
  self->fields |= MLN_CAMERA_OPTION_ZOOM;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_options_properties[CAMERA_OPTIONS_PROP_ZOOM]
    );
}

gdouble maplibre_native_ffi_camera_options_get_bearing(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), 0);
  return self->bearing;
}

gboolean maplibre_native_ffi_camera_options_has_bearing(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), FALSE);
  return (self->fields & MLN_CAMERA_OPTION_BEARING) != 0;
}

void maplibre_native_ffi_camera_options_set_bearing(
  MaplibreNativeFfiCameraOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_CAMERA_OPTION_BEARING) != 0;
  gboolean changed = self->bearing != value;
  self->bearing = value;
  self->fields |= MLN_CAMERA_OPTION_BEARING;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_options_properties[CAMERA_OPTIONS_PROP_BEARING]
    );
}

gdouble maplibre_native_ffi_camera_options_get_pitch(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), 0);
  return self->pitch;
}

gboolean maplibre_native_ffi_camera_options_has_pitch(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), FALSE);
  return (self->fields & MLN_CAMERA_OPTION_PITCH) != 0;
}

void maplibre_native_ffi_camera_options_set_pitch(
  MaplibreNativeFfiCameraOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_CAMERA_OPTION_PITCH) != 0;
  gboolean changed = self->pitch != value;
  self->pitch = value;
  self->fields |= MLN_CAMERA_OPTION_PITCH;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_options_properties[CAMERA_OPTIONS_PROP_PITCH]
    );
}

gdouble maplibre_native_ffi_camera_options_get_roll(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), 0);
  return self->roll;
}

gboolean maplibre_native_ffi_camera_options_has_roll(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), FALSE);
  return (self->fields & MLN_CAMERA_OPTION_ROLL) != 0;
}

void maplibre_native_ffi_camera_options_set_roll(
  MaplibreNativeFfiCameraOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_CAMERA_OPTION_ROLL) != 0;
  gboolean changed = self->roll != value;
  self->roll = value;
  self->fields |= MLN_CAMERA_OPTION_ROLL;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_options_properties[CAMERA_OPTIONS_PROP_ROLL]
    );
}

gdouble maplibre_native_ffi_camera_options_get_field_of_view(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), 0);
  return self->field_of_view;
}

gboolean maplibre_native_ffi_camera_options_has_field_of_view(
  MaplibreNativeFfiCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self), FALSE);
  return (self->fields & MLN_CAMERA_OPTION_FOV) != 0;
}

void maplibre_native_ffi_camera_options_set_field_of_view(
  MaplibreNativeFfiCameraOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_CAMERA_OPTION_FOV) != 0;
  gboolean changed = self->field_of_view != value;
  self->field_of_view = value;
  self->fields |= MLN_CAMERA_OPTION_FOV;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      camera_options_properties[CAMERA_OPTIONS_PROP_FIELD_OF_VIEW]
    );
}

MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_options_copy(
  const MaplibreNativeFfiCameraOptions* self
) {
  MaplibreNativeFfiCameraOptions* copy =
    maplibre_native_ffi_camera_options_new();
  copy->fields = self->fields;
  copy->center = self->center;
  copy->center_altitude = self->center_altitude;
  copy->padding = self->padding;
  copy->anchor = self->anchor;
  copy->zoom = self->zoom;
  copy->bearing = self->bearing;
  copy->pitch = self->pitch;
  copy->roll = self->roll;
  copy->field_of_view = self->field_of_view;
  return copy;
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiCameraQueryResult) == sizeof(mln_camera_query_result)
);
SAME_MEMBER(
  MaplibreNativeFfiCameraQueryResult, mln_camera_query_result, generation
);
SAME_MEMBER(
  MaplibreNativeFfiCameraQueryResult, mln_camera_query_result, camera
);

void maplibre_native_ffi_camera_query_result_read(
  MaplibreNativeFfiCameraQueryResult* out, const mln_camera_query_result* in
) {
  memcpy(out, in, sizeof *out);
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
  MaplibreNativeFfiCameraVelocity, maplibre_native_ffi_camera_velocity,
  maplibre_native_ffi_camera_velocity_copy,
  maplibre_native_ffi_camera_velocity_free
)

MaplibreNativeFfiCameraVelocity* maplibre_native_ffi_camera_velocity_copy(
  const MaplibreNativeFfiCameraVelocity* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_camera_velocity_free(
  MaplibreNativeFfiCameraVelocity* self
) {
  g_free(self);
}

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiCameraVelocity) == sizeof(mln_camera_velocity)
);
SAME_MEMBER(MaplibreNativeFfiCameraVelocity, mln_camera_velocity, pan);
SAME_MEMBER(MaplibreNativeFfiCameraVelocity, mln_camera_velocity, zoom);
SAME_MEMBER(MaplibreNativeFfiCameraVelocity, mln_camera_velocity, bearing);
SAME_MEMBER(MaplibreNativeFfiCameraVelocity, mln_camera_velocity, pitch);

void maplibre_native_ffi_camera_velocity_read(
  MaplibreNativeFfiCameraVelocity* out, const mln_camera_velocity* in
) {
  memcpy(out, in, sizeof *out);
}

MaplibreNativeFfiCameraVelocity*
maplibre_native_ffi_camera_velocity_from_native(const mln_camera_velocity* in) {
  MaplibreNativeFfiCameraVelocity* self =
    g_new0(MaplibreNativeFfiCameraVelocity, 1);
  maplibre_native_ffi_camera_velocity_read(self, in);
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

void maplibre_native_ffi_camera_transition_end_read(
  MaplibreNativeFfiCameraTransitionEnd* out, const mln_camera_transition_end* in
) {
  out->outcome = (MaplibreNativeFfiCameraTransitionOutcome)(in->outcome);
  out->generation = (guint64)(in->generation);
  maplibre_native_ffi_camera_velocity_read(&out->velocity, &in->velocity);
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
  GObject parent_instance;
  MaplibreNativeFfiCameraUpdateMode mode;
  MaplibreNativeFfiCameraOptions* camera;
  MaplibreNativeFfiAnimationOptions* animation;
  MaplibreNativeFfiGesturePhase gesture_phase;
};

enum {
  CAMERA_UPDATE_PROP_0,
  CAMERA_UPDATE_PROP_MODE,
  CAMERA_UPDATE_PROP_CAMERA,
  CAMERA_UPDATE_PROP_ANIMATION,
  CAMERA_UPDATE_PROP_GESTURE_PHASE,
  CAMERA_UPDATE_N_PROPERTIES
};

static GParamSpec* camera_update_properties[CAMERA_UPDATE_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiCameraUpdate, maplibre_native_ffi_camera_update,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_camera_update_finalize(GObject* object) {
  MaplibreNativeFfiCameraUpdate* self =
    MAPLIBRE_NATIVE_FFI_CAMERA_UPDATE(object);
  g_clear_object(&self->camera);
  g_clear_object(&self->animation);
  G_OBJECT_CLASS(maplibre_native_ffi_camera_update_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_camera_update_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCameraUpdate* self =
    MAPLIBRE_NATIVE_FFI_CAMERA_UPDATE(object);
  switch (id) {
    case CAMERA_UPDATE_PROP_MODE:
      g_value_set_enum(value, self->mode);
      break;
    case CAMERA_UPDATE_PROP_CAMERA:
      g_value_set_object(value, self->camera);
      break;
    case CAMERA_UPDATE_PROP_ANIMATION:
      g_value_set_object(value, self->animation);
      break;
    case CAMERA_UPDATE_PROP_GESTURE_PHASE:
      g_value_set_enum(value, self->gesture_phase);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_camera_update_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCameraUpdate* self =
    MAPLIBRE_NATIVE_FFI_CAMERA_UPDATE(object);
  switch (id) {
    case CAMERA_UPDATE_PROP_MODE:
      maplibre_native_ffi_camera_update_set_mode(self, g_value_get_enum(value));
      break;
    case CAMERA_UPDATE_PROP_CAMERA:
      maplibre_native_ffi_camera_update_set_camera(
        self, g_value_get_object(value)
      );
      break;
    case CAMERA_UPDATE_PROP_ANIMATION:
      maplibre_native_ffi_camera_update_set_animation(
        self, g_value_get_object(value)
      );
      break;
    case CAMERA_UPDATE_PROP_GESTURE_PHASE:
      maplibre_native_ffi_camera_update_set_gesture_phase(
        self, g_value_get_enum(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_camera_update_class_init(
  MaplibreNativeFfiCameraUpdateClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_camera_update_finalize;
  object_class->get_property = maplibre_native_ffi_camera_update_get_property;
  object_class->set_property = maplibre_native_ffi_camera_update_set_property;
  camera_update_properties[CAMERA_UPDATE_PROP_MODE] = g_param_spec_enum(
    "mode", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_UPDATE_MODE, 0,
    PROPERTY_FLAGS
  );
  camera_update_properties[CAMERA_UPDATE_PROP_CAMERA] = g_param_spec_object(
    "camera", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_OPTIONS,
    PROPERTY_FLAGS
  );
  camera_update_properties[CAMERA_UPDATE_PROP_ANIMATION] = g_param_spec_object(
    "animation", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_ANIMATION_OPTIONS,
    PROPERTY_FLAGS
  );
  camera_update_properties[CAMERA_UPDATE_PROP_GESTURE_PHASE] =
    g_param_spec_enum(
      "gesture-phase", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_GESTURE_PHASE, 0,
      PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, CAMERA_UPDATE_N_PROPERTIES, camera_update_properties
  );
}

static void maplibre_native_ffi_camera_update_init(
  MaplibreNativeFfiCameraUpdate* self
) {
  self->camera = maplibre_native_ffi_camera_options_new();
  self->animation = maplibre_native_ffi_animation_options_new();
}

MaplibreNativeFfiCameraUpdate* maplibre_native_ffi_camera_update_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_UPDATE, NULL);
}

MaplibreNativeFfiCameraUpdateMode maplibre_native_ffi_camera_update_get_mode(
  MaplibreNativeFfiCameraUpdate* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CAMERA_UPDATE(self),
    (MaplibreNativeFfiCameraUpdateMode)0
  );
  return self->mode;
}

void maplibre_native_ffi_camera_update_set_mode(
  MaplibreNativeFfiCameraUpdate* self, MaplibreNativeFfiCameraUpdateMode value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_UPDATE(self));
  gboolean changed = self->mode != value;
  self->mode = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_update_properties[CAMERA_UPDATE_PROP_MODE]
    );
}

MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_update_get_camera(
  MaplibreNativeFfiCameraUpdate* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_UPDATE(self), NULL);
  return self->camera;
}

void maplibre_native_ffi_camera_update_set_camera(
  MaplibreNativeFfiCameraUpdate* self, MaplibreNativeFfiCameraOptions* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_UPDATE(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(value));
  MaplibreNativeFfiCameraOptions* previous = self->camera;
  self->camera = maplibre_native_ffi_camera_options_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), camera_update_properties[CAMERA_UPDATE_PROP_CAMERA]
  );
}

MaplibreNativeFfiAnimationOptions*
maplibre_native_ffi_camera_update_get_animation(
  MaplibreNativeFfiCameraUpdate* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_UPDATE(self), NULL);
  return self->animation;
}

void maplibre_native_ffi_camera_update_set_animation(
  MaplibreNativeFfiCameraUpdate* self, MaplibreNativeFfiAnimationOptions* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_UPDATE(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_ANIMATION_OPTIONS(value));
  MaplibreNativeFfiAnimationOptions* previous = self->animation;
  self->animation = maplibre_native_ffi_animation_options_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), camera_update_properties[CAMERA_UPDATE_PROP_ANIMATION]
  );
}

MaplibreNativeFfiGesturePhase
maplibre_native_ffi_camera_update_get_gesture_phase(
  MaplibreNativeFfiCameraUpdate* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CAMERA_UPDATE(self), (MaplibreNativeFfiGesturePhase)0
  );
  return self->gesture_phase;
}

void maplibre_native_ffi_camera_update_set_gesture_phase(
  MaplibreNativeFfiCameraUpdate* self, MaplibreNativeFfiGesturePhase value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_UPDATE(self));
  gboolean changed = self->gesture_phase != value;
  self->gesture_phase = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), camera_update_properties[CAMERA_UPDATE_PROP_GESTURE_PHASE]
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiCanonicalTileId) == sizeof(mln_canonical_tile_id)
);
SAME_MEMBER(MaplibreNativeFfiCanonicalTileId, mln_canonical_tile_id, z);
SAME_MEMBER(MaplibreNativeFfiCanonicalTileId, mln_canonical_tile_id, x);
SAME_MEMBER(MaplibreNativeFfiCanonicalTileId, mln_canonical_tile_id, y);

void maplibre_native_ffi_canonical_tile_id_to_native(
  const MaplibreNativeFfiCanonicalTileId* self, mln_canonical_tile_id* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_canonical_tile_id_read(
  MaplibreNativeFfiCanonicalTileId* out, const mln_canonical_tile_id* in
) {
  memcpy(out, in, sizeof *out);
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
  GObject parent_instance;
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

enum {
  CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_0,
  CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_MIN_ZOOM,
  CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_MAX_ZOOM,
  CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_TOLERANCE,
  CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_TILE_SIZE,
  CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_BUFFER,
  CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_CLIP,
  CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_WRAP,
  CUSTOM_GEOMETRY_SOURCE_OPTIONS_N_PROPERTIES
};

static GParamSpec* custom_geometry_source_options_properties
  [CUSTOM_GEOMETRY_SOURCE_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiCustomGeometrySourceOptions,
  maplibre_native_ffi_custom_geometry_source_options, G_TYPE_OBJECT
)

static void maplibre_native_ffi_custom_geometry_source_options_finalize(
  GObject* object
) {
  MaplibreNativeFfiCustomGeometrySourceOptions* self =
    MAPLIBRE_NATIVE_FFI_CUSTOM_GEOMETRY_SOURCE_OPTIONS(object);
  maplibre_native_ffi_closure_unref(self->fetch_tile);
  maplibre_native_ffi_closure_unref(self->cancel_tile);
  G_OBJECT_CLASS(
    maplibre_native_ffi_custom_geometry_source_options_parent_class
  )
    ->finalize(object);
}

static void maplibre_native_ffi_custom_geometry_source_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCustomGeometrySourceOptions* self =
    MAPLIBRE_NATIVE_FFI_CUSTOM_GEOMETRY_SOURCE_OPTIONS(object);
  switch (id) {
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_MIN_ZOOM:
      g_value_set_double(value, self->min_zoom);
      break;
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_MAX_ZOOM:
      g_value_set_double(value, self->max_zoom);
      break;
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_TOLERANCE:
      g_value_set_double(value, self->tolerance);
      break;
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_TILE_SIZE:
      g_value_set_uint(value, self->tile_size);
      break;
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_BUFFER:
      g_value_set_uint(value, self->buffer);
      break;
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_CLIP:
      g_value_set_boolean(value, self->clip);
      break;
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_WRAP:
      g_value_set_boolean(value, self->wrap);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_custom_geometry_source_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCustomGeometrySourceOptions* self =
    MAPLIBRE_NATIVE_FFI_CUSTOM_GEOMETRY_SOURCE_OPTIONS(object);
  switch (id) {
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_MIN_ZOOM:
      maplibre_native_ffi_custom_geometry_source_options_set_min_zoom(
        self, g_value_get_double(value)
      );
      break;
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_MAX_ZOOM:
      maplibre_native_ffi_custom_geometry_source_options_set_max_zoom(
        self, g_value_get_double(value)
      );
      break;
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_TOLERANCE:
      maplibre_native_ffi_custom_geometry_source_options_set_tolerance(
        self, g_value_get_double(value)
      );
      break;
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_TILE_SIZE:
      maplibre_native_ffi_custom_geometry_source_options_set_tile_size(
        self, g_value_get_uint(value)
      );
      break;
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_BUFFER:
      maplibre_native_ffi_custom_geometry_source_options_set_buffer(
        self, g_value_get_uint(value)
      );
      break;
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_CLIP:
      maplibre_native_ffi_custom_geometry_source_options_set_clip(
        self, g_value_get_boolean(value)
      );
      break;
    case CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_WRAP:
      maplibre_native_ffi_custom_geometry_source_options_set_wrap(
        self, g_value_get_boolean(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_custom_geometry_source_options_class_init(
  MaplibreNativeFfiCustomGeometrySourceOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_custom_geometry_source_options_finalize;
  object_class->get_property =
    maplibre_native_ffi_custom_geometry_source_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_custom_geometry_source_options_set_property;
  custom_geometry_source_options_properties
    [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_MIN_ZOOM] = g_param_spec_double(
      "min-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  custom_geometry_source_options_properties
    [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_MAX_ZOOM] = g_param_spec_double(
      "max-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  custom_geometry_source_options_properties
    [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_TOLERANCE] = g_param_spec_double(
      "tolerance", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  custom_geometry_source_options_properties
    [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_TILE_SIZE] = g_param_spec_uint(
      "tile-size", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
    );
  custom_geometry_source_options_properties
    [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_BUFFER] = g_param_spec_uint(
      "buffer", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
    );
  custom_geometry_source_options_properties
    [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_CLIP] =
      g_param_spec_boolean("clip", NULL, NULL, FALSE, PROPERTY_FLAGS);
  custom_geometry_source_options_properties
    [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_WRAP] =
      g_param_spec_boolean("wrap", NULL, NULL, FALSE, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, CUSTOM_GEOMETRY_SOURCE_OPTIONS_N_PROPERTIES,
    custom_geometry_source_options_properties
  );
}

static void maplibre_native_ffi_custom_geometry_source_options_init(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  (void)self;
}

MaplibreNativeFfiCustomGeometrySourceOptions*
maplibre_native_ffi_custom_geometry_source_options_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_CUSTOM_GEOMETRY_SOURCE_OPTIONS, NULL
  );
}

gdouble maplibre_native_ffi_custom_geometry_source_options_get_min_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), 0
  );
  return self->min_zoom;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_min_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_min_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM) != 0;
  gboolean changed = self->min_zoom != value;
  self->min_zoom = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), custom_geometry_source_options_properties
                        [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_MIN_ZOOM]
    );
}

gdouble maplibre_native_ffi_custom_geometry_source_options_get_max_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), 0
  );
  return self->max_zoom;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_max_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_max_zoom(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM) != 0;
  gboolean changed = self->max_zoom != value;
  self->max_zoom = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), custom_geometry_source_options_properties
                        [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_MAX_ZOOM]
    );
}

gdouble maplibre_native_ffi_custom_geometry_source_options_get_tolerance(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), 0
  );
  return self->tolerance;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_tolerance(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_tolerance(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE) != 0;
  gboolean changed = self->tolerance != value;
  self->tolerance = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), custom_geometry_source_options_properties
                        [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_TOLERANCE]
    );
}

guint32 maplibre_native_ffi_custom_geometry_source_options_get_tile_size(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), 0
  );
  return self->tile_size;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_tile_size(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_tile_size(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, guint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE) != 0;
  gboolean changed = self->tile_size != value;
  self->tile_size = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), custom_geometry_source_options_properties
                        [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_TILE_SIZE]
    );
}

guint32 maplibre_native_ffi_custom_geometry_source_options_get_buffer(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), 0
  );
  return self->buffer;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_buffer(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_buffer(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, guint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER) != 0;
  gboolean changed = self->buffer != value;
  self->buffer = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), custom_geometry_source_options_properties
                        [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_BUFFER]
    );
}

gboolean maplibre_native_ffi_custom_geometry_source_options_get_clip(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), FALSE
  );
  return self->clip;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_clip(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_clip(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gboolean value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP) != 0;
  gboolean changed = self->clip != value;
  self->clip = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), custom_geometry_source_options_properties
                        [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_CLIP]
    );
}

gboolean maplibre_native_ffi_custom_geometry_source_options_get_wrap(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), FALSE
  );
  return self->wrap;
}

gboolean maplibre_native_ffi_custom_geometry_source_options_has_wrap(
  MaplibreNativeFfiCustomGeometrySourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP) != 0;
}

void maplibre_native_ffi_custom_geometry_source_options_set_wrap(
  MaplibreNativeFfiCustomGeometrySourceOptions* self, gboolean value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP) != 0;
  gboolean changed = self->wrap != value;
  self->wrap = value;
  self->fields |= MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), custom_geometry_source_options_properties
                        [CUSTOM_GEOMETRY_SOURCE_OPTIONS_PROP_WRAP]
    );
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

struct _MaplibreNativeFfiCustomVectorSourceOptions {
  GObject parent_instance;
  guint64 fields;
  gdouble min_zoom;
  gdouble max_zoom;
  MaplibreNativeFfiClosure* fetch_tile;
  MaplibreNativeFfiClosure* cancel_tile;
};

enum {
  CUSTOM_VECTOR_SOURCE_OPTIONS_PROP_0,
  CUSTOM_VECTOR_SOURCE_OPTIONS_PROP_MIN_ZOOM,
  CUSTOM_VECTOR_SOURCE_OPTIONS_PROP_MAX_ZOOM,
  CUSTOM_VECTOR_SOURCE_OPTIONS_N_PROPERTIES
};

static GParamSpec* custom_vector_source_options_properties
  [CUSTOM_VECTOR_SOURCE_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiCustomVectorSourceOptions,
  maplibre_native_ffi_custom_vector_source_options, G_TYPE_OBJECT
)

static void maplibre_native_ffi_custom_vector_source_options_finalize(
  GObject* object
) {
  MaplibreNativeFfiCustomVectorSourceOptions* self =
    MAPLIBRE_NATIVE_FFI_CUSTOM_VECTOR_SOURCE_OPTIONS(object);
  maplibre_native_ffi_closure_unref(self->fetch_tile);
  maplibre_native_ffi_closure_unref(self->cancel_tile);
  G_OBJECT_CLASS(maplibre_native_ffi_custom_vector_source_options_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_custom_vector_source_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCustomVectorSourceOptions* self =
    MAPLIBRE_NATIVE_FFI_CUSTOM_VECTOR_SOURCE_OPTIONS(object);
  switch (id) {
    case CUSTOM_VECTOR_SOURCE_OPTIONS_PROP_MIN_ZOOM:
      g_value_set_double(value, self->min_zoom);
      break;
    case CUSTOM_VECTOR_SOURCE_OPTIONS_PROP_MAX_ZOOM:
      g_value_set_double(value, self->max_zoom);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_custom_vector_source_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiCustomVectorSourceOptions* self =
    MAPLIBRE_NATIVE_FFI_CUSTOM_VECTOR_SOURCE_OPTIONS(object);
  switch (id) {
    case CUSTOM_VECTOR_SOURCE_OPTIONS_PROP_MIN_ZOOM:
      maplibre_native_ffi_custom_vector_source_options_set_min_zoom(
        self, g_value_get_double(value)
      );
      break;
    case CUSTOM_VECTOR_SOURCE_OPTIONS_PROP_MAX_ZOOM:
      maplibre_native_ffi_custom_vector_source_options_set_max_zoom(
        self, g_value_get_double(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_custom_vector_source_options_class_init(
  MaplibreNativeFfiCustomVectorSourceOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_custom_vector_source_options_finalize;
  object_class->get_property =
    maplibre_native_ffi_custom_vector_source_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_custom_vector_source_options_set_property;
  custom_vector_source_options_properties
    [CUSTOM_VECTOR_SOURCE_OPTIONS_PROP_MIN_ZOOM] = g_param_spec_double(
      "min-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  custom_vector_source_options_properties
    [CUSTOM_VECTOR_SOURCE_OPTIONS_PROP_MAX_ZOOM] = g_param_spec_double(
      "max-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, CUSTOM_VECTOR_SOURCE_OPTIONS_N_PROPERTIES,
    custom_vector_source_options_properties
  );
}

static void maplibre_native_ffi_custom_vector_source_options_init(
  MaplibreNativeFfiCustomVectorSourceOptions* self
) {
  (void)self;
}

MaplibreNativeFfiCustomVectorSourceOptions*
maplibre_native_ffi_custom_vector_source_options_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_CUSTOM_VECTOR_SOURCE_OPTIONS, NULL
  );
}

gdouble maplibre_native_ffi_custom_vector_source_options_get_min_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_VECTOR_SOURCE_OPTIONS(self), 0
  );
  return self->min_zoom;
}

gboolean maplibre_native_ffi_custom_vector_source_options_has_min_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_VECTOR_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM) != 0;
}

void maplibre_native_ffi_custom_vector_source_options_set_min_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CUSTOM_VECTOR_SOURCE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM) != 0;
  gboolean changed = self->min_zoom != value;
  self->min_zoom = value;
  self->fields |= MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), custom_vector_source_options_properties
                        [CUSTOM_VECTOR_SOURCE_OPTIONS_PROP_MIN_ZOOM]
    );
}

gdouble maplibre_native_ffi_custom_vector_source_options_get_max_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_VECTOR_SOURCE_OPTIONS(self), 0
  );
  return self->max_zoom;
}

gboolean maplibre_native_ffi_custom_vector_source_options_has_max_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_VECTOR_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM) != 0;
}

void maplibre_native_ffi_custom_vector_source_options_set_max_zoom(
  MaplibreNativeFfiCustomVectorSourceOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CUSTOM_VECTOR_SOURCE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM) != 0;
  gboolean changed = self->max_zoom != value;
  self->max_zoom = value;
  self->fields |= MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), custom_vector_source_options_properties
                        [CUSTOM_VECTOR_SOURCE_OPTIONS_PROP_MAX_ZOOM]
    );
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiDmaBufFormat) == sizeof(mln_dma_buf_format)
);
SAME_MEMBER(MaplibreNativeFfiDmaBufFormat, mln_dma_buf_format, fourcc);
SAME_MEMBER(MaplibreNativeFfiDmaBufFormat, mln_dma_buf_format, modifier);

void maplibre_native_ffi_dma_buf_format_to_native(
  const MaplibreNativeFfiDmaBufFormat* self, mln_dma_buf_format* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiDmaBufPlane) == sizeof(mln_dma_buf_plane)
);
SAME_MEMBER(MaplibreNativeFfiDmaBufPlane, mln_dma_buf_plane, fd);
SAME_MEMBER(MaplibreNativeFfiDmaBufPlane, mln_dma_buf_plane, offset);
SAME_MEMBER(MaplibreNativeFfiDmaBufPlane, mln_dma_buf_plane, stride);

void maplibre_native_ffi_dma_buf_plane_read(
  MaplibreNativeFfiDmaBufPlane* out, const mln_dma_buf_plane* in
) {
  memcpy(out, in, sizeof *out);
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiLogicalSize) == sizeof(mln_logical_size)
);
SAME_MEMBER(MaplibreNativeFfiLogicalSize, mln_logical_size, width);
SAME_MEMBER(MaplibreNativeFfiLogicalSize, mln_logical_size, height);

void maplibre_native_ffi_logical_size_to_native(
  const MaplibreNativeFfiLogicalSize* self, mln_logical_size* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

struct _MaplibreNativeFfiDmaBufRingDescriptor {
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 buffer_count;
  GPtrArray* formats;
};

enum {
  DMA_BUF_RING_DESCRIPTOR_PROP_0,
  DMA_BUF_RING_DESCRIPTOR_PROP_LOGICAL_SIZE,
  DMA_BUF_RING_DESCRIPTOR_PROP_BUFFER_COUNT,
  DMA_BUF_RING_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec*
  dma_buf_ring_descriptor_properties[DMA_BUF_RING_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiDmaBufRingDescriptor,
  maplibre_native_ffi_dma_buf_ring_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_dma_buf_ring_descriptor_finalize(
  GObject* object
) {
  MaplibreNativeFfiDmaBufRingDescriptor* self =
    MAPLIBRE_NATIVE_FFI_DMA_BUF_RING_DESCRIPTOR(object);
  g_clear_pointer(&self->formats, g_ptr_array_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_dma_buf_ring_descriptor_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_dma_buf_ring_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiDmaBufRingDescriptor* self =
    MAPLIBRE_NATIVE_FFI_DMA_BUF_RING_DESCRIPTOR(object);
  switch (id) {
    case DMA_BUF_RING_DESCRIPTOR_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case DMA_BUF_RING_DESCRIPTOR_PROP_BUFFER_COUNT:
      g_value_set_uint(value, self->buffer_count);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_dma_buf_ring_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiDmaBufRingDescriptor* self =
    MAPLIBRE_NATIVE_FFI_DMA_BUF_RING_DESCRIPTOR(object);
  switch (id) {
    case DMA_BUF_RING_DESCRIPTOR_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_dma_buf_ring_descriptor_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case DMA_BUF_RING_DESCRIPTOR_PROP_BUFFER_COUNT:
      maplibre_native_ffi_dma_buf_ring_descriptor_set_buffer_count(
        self, g_value_get_uint(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_dma_buf_ring_descriptor_class_init(
  MaplibreNativeFfiDmaBufRingDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_dma_buf_ring_descriptor_finalize;
  object_class->get_property =
    maplibre_native_ffi_dma_buf_ring_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_dma_buf_ring_descriptor_set_property;
  dma_buf_ring_descriptor_properties
    [DMA_BUF_RING_DESCRIPTOR_PROP_LOGICAL_SIZE] = g_param_spec_boxed(
      "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
      PROPERTY_FLAGS
    );
  dma_buf_ring_descriptor_properties
    [DMA_BUF_RING_DESCRIPTOR_PROP_BUFFER_COUNT] = g_param_spec_uint(
      "buffer-count", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, DMA_BUF_RING_DESCRIPTOR_N_PROPERTIES,
    dma_buf_ring_descriptor_properties
  );
}

static void maplibre_native_ffi_dma_buf_ring_descriptor_init(
  MaplibreNativeFfiDmaBufRingDescriptor* self
) {
  self->formats = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_dma_buf_format_free
  );
}

MaplibreNativeFfiDmaBufRingDescriptor*
maplibre_native_ffi_dma_buf_ring_descriptor_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_DMA_BUF_RING_DESCRIPTOR, NULL);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_dma_buf_ring_descriptor_get_logical_size(
  MaplibreNativeFfiDmaBufRingDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_DMA_BUF_RING_DESCRIPTOR(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_dma_buf_ring_descriptor_set_logical_size(
  MaplibreNativeFfiDmaBufRingDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_DMA_BUF_RING_DESCRIPTOR(self));
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), dma_buf_ring_descriptor_properties
                        [DMA_BUF_RING_DESCRIPTOR_PROP_LOGICAL_SIZE]
    );
}

guint32 maplibre_native_ffi_dma_buf_ring_descriptor_get_buffer_count(
  MaplibreNativeFfiDmaBufRingDescriptor* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_DMA_BUF_RING_DESCRIPTOR(self), 0);
  return self->buffer_count;
}

void maplibre_native_ffi_dma_buf_ring_descriptor_set_buffer_count(
  MaplibreNativeFfiDmaBufRingDescriptor* self, guint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_DMA_BUF_RING_DESCRIPTOR(self));
  gboolean changed = self->buffer_count != value;
  self->buffer_count = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), dma_buf_ring_descriptor_properties
                        [DMA_BUF_RING_DESCRIPTOR_PROP_BUFFER_COUNT]
    );
}

GPtrArray* maplibre_native_ffi_dma_buf_ring_descriptor_get_formats(
  MaplibreNativeFfiDmaBufRingDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_DMA_BUF_RING_DESCRIPTOR(self), NULL
  );
  return self->formats;
}

void maplibre_native_ffi_dma_buf_ring_descriptor_set_formats(
  MaplibreNativeFfiDmaBufRingDescriptor* self,
  const MaplibreNativeFfiDmaBufFormat* value, gsize value_count
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_DMA_BUF_RING_DESCRIPTOR(self));
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

struct _MaplibreNativeFfiEglContextDescriptor {
  GObject parent_instance;
  gpointer display;
  gpointer config;
  gpointer share_context;
  MaplibreNativeFfiOpenGlClientApi client_api;
  gpointer get_proc_address;
};

enum {
  EGL_CONTEXT_DESCRIPTOR_PROP_0,
  EGL_CONTEXT_DESCRIPTOR_PROP_DISPLAY,
  EGL_CONTEXT_DESCRIPTOR_PROP_CONFIG,
  EGL_CONTEXT_DESCRIPTOR_PROP_SHARE_CONTEXT,
  EGL_CONTEXT_DESCRIPTOR_PROP_CLIENT_API,
  EGL_CONTEXT_DESCRIPTOR_PROP_GET_PROC_ADDRESS,
  EGL_CONTEXT_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec*
  egl_context_descriptor_properties[EGL_CONTEXT_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiEglContextDescriptor,
  maplibre_native_ffi_egl_context_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_egl_context_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiEglContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_EGL_CONTEXT_DESCRIPTOR(object);
  switch (id) {
    case EGL_CONTEXT_DESCRIPTOR_PROP_DISPLAY:
      g_value_set_pointer(value, self->display);
      break;
    case EGL_CONTEXT_DESCRIPTOR_PROP_CONFIG:
      g_value_set_pointer(value, self->config);
      break;
    case EGL_CONTEXT_DESCRIPTOR_PROP_SHARE_CONTEXT:
      g_value_set_pointer(value, self->share_context);
      break;
    case EGL_CONTEXT_DESCRIPTOR_PROP_CLIENT_API:
      g_value_set_enum(value, self->client_api);
      break;
    case EGL_CONTEXT_DESCRIPTOR_PROP_GET_PROC_ADDRESS:
      g_value_set_pointer(value, self->get_proc_address);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_egl_context_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiEglContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_EGL_CONTEXT_DESCRIPTOR(object);
  switch (id) {
    case EGL_CONTEXT_DESCRIPTOR_PROP_DISPLAY:
      maplibre_native_ffi_egl_context_descriptor_set_display(
        self, g_value_get_pointer(value)
      );
      break;
    case EGL_CONTEXT_DESCRIPTOR_PROP_CONFIG:
      maplibre_native_ffi_egl_context_descriptor_set_config(
        self, g_value_get_pointer(value)
      );
      break;
    case EGL_CONTEXT_DESCRIPTOR_PROP_SHARE_CONTEXT:
      maplibre_native_ffi_egl_context_descriptor_set_share_context(
        self, g_value_get_pointer(value)
      );
      break;
    case EGL_CONTEXT_DESCRIPTOR_PROP_CLIENT_API:
      maplibre_native_ffi_egl_context_descriptor_set_client_api(
        self, g_value_get_enum(value)
      );
      break;
    case EGL_CONTEXT_DESCRIPTOR_PROP_GET_PROC_ADDRESS:
      maplibre_native_ffi_egl_context_descriptor_set_get_proc_address(
        self, g_value_get_pointer(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_egl_context_descriptor_class_init(
  MaplibreNativeFfiEglContextDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_egl_context_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_egl_context_descriptor_set_property;
  egl_context_descriptor_properties[EGL_CONTEXT_DESCRIPTOR_PROP_DISPLAY] =
    g_param_spec_pointer("display", NULL, NULL, PROPERTY_FLAGS);
  egl_context_descriptor_properties[EGL_CONTEXT_DESCRIPTOR_PROP_CONFIG] =
    g_param_spec_pointer("config", NULL, NULL, PROPERTY_FLAGS);
  egl_context_descriptor_properties[EGL_CONTEXT_DESCRIPTOR_PROP_SHARE_CONTEXT] =
    g_param_spec_pointer("share-context", NULL, NULL, PROPERTY_FLAGS);
  egl_context_descriptor_properties[EGL_CONTEXT_DESCRIPTOR_PROP_CLIENT_API] =
    g_param_spec_enum(
      "client-api", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_CLIENT_API, 0,
      PROPERTY_FLAGS
    );
  egl_context_descriptor_properties
    [EGL_CONTEXT_DESCRIPTOR_PROP_GET_PROC_ADDRESS] =
      g_param_spec_pointer("get-proc-address", NULL, NULL, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, EGL_CONTEXT_DESCRIPTOR_N_PROPERTIES,
    egl_context_descriptor_properties
  );
}

static void maplibre_native_ffi_egl_context_descriptor_init(
  MaplibreNativeFfiEglContextDescriptor* self
) {
  (void)self;
}

MaplibreNativeFfiEglContextDescriptor*
maplibre_native_ffi_egl_context_descriptor_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_EGL_CONTEXT_DESCRIPTOR, NULL);
}

gpointer maplibre_native_ffi_egl_context_descriptor_get_display(
  MaplibreNativeFfiEglContextDescriptor* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_EGL_CONTEXT_DESCRIPTOR(self), 0);
  return self->display;
}

void maplibre_native_ffi_egl_context_descriptor_set_display(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_EGL_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->display != value;
  self->display = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      egl_context_descriptor_properties[EGL_CONTEXT_DESCRIPTOR_PROP_DISPLAY]
    );
}

gpointer maplibre_native_ffi_egl_context_descriptor_get_config(
  MaplibreNativeFfiEglContextDescriptor* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_EGL_CONTEXT_DESCRIPTOR(self), 0);
  return self->config;
}

void maplibre_native_ffi_egl_context_descriptor_set_config(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_EGL_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->config != value;
  self->config = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      egl_context_descriptor_properties[EGL_CONTEXT_DESCRIPTOR_PROP_CONFIG]
    );
}

gpointer maplibre_native_ffi_egl_context_descriptor_get_share_context(
  MaplibreNativeFfiEglContextDescriptor* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_EGL_CONTEXT_DESCRIPTOR(self), 0);
  return self->share_context;
}

void maplibre_native_ffi_egl_context_descriptor_set_share_context(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_EGL_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->share_context != value;
  self->share_context = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), egl_context_descriptor_properties
                        [EGL_CONTEXT_DESCRIPTOR_PROP_SHARE_CONTEXT]
    );
}

MaplibreNativeFfiOpenGlClientApi
maplibre_native_ffi_egl_context_descriptor_get_client_api(
  MaplibreNativeFfiEglContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_EGL_CONTEXT_DESCRIPTOR(self),
    (MaplibreNativeFfiOpenGlClientApi)0
  );
  return self->client_api;
}

void maplibre_native_ffi_egl_context_descriptor_set_client_api(
  MaplibreNativeFfiEglContextDescriptor* self,
  MaplibreNativeFfiOpenGlClientApi value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_EGL_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->client_api != value;
  self->client_api = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      egl_context_descriptor_properties[EGL_CONTEXT_DESCRIPTOR_PROP_CLIENT_API]
    );
}

gpointer maplibre_native_ffi_egl_context_descriptor_get_get_proc_address(
  MaplibreNativeFfiEglContextDescriptor* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_EGL_CONTEXT_DESCRIPTOR(self), 0);
  return self->get_proc_address;
}

void maplibre_native_ffi_egl_context_descriptor_set_get_proc_address(
  MaplibreNativeFfiEglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_EGL_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->get_proc_address != value;
  self->get_proc_address = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), egl_context_descriptor_properties
                        [EGL_CONTEXT_DESCRIPTOR_PROP_GET_PROC_ADDRESS]
    );
}

MaplibreNativeFfiEglContextDescriptor*
maplibre_native_ffi_egl_context_descriptor_copy(
  const MaplibreNativeFfiEglContextDescriptor* self
) {
  MaplibreNativeFfiEglContextDescriptor* copy =
    maplibre_native_ffi_egl_context_descriptor_new();
  copy->display = self->display;
  copy->config = self->config;
  copy->share_context = self->share_context;
  copy->client_api = self->client_api;
  copy->get_proc_address = self->get_proc_address;
  return copy;
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

struct _MaplibreNativeFfiFeatureStateSelector {
  GObject parent_instance;
  guint64 fields;
  char* source_id;
  char* source_layer_id;
  MaplibreNativeFfiFeatureId* feature_id;
  char* state_key;
};

enum {
  FEATURE_STATE_SELECTOR_PROP_0,
  FEATURE_STATE_SELECTOR_PROP_SOURCE_ID,
  FEATURE_STATE_SELECTOR_PROP_SOURCE_LAYER_ID,
  FEATURE_STATE_SELECTOR_PROP_FEATURE_ID,
  FEATURE_STATE_SELECTOR_PROP_STATE_KEY,
  FEATURE_STATE_SELECTOR_N_PROPERTIES
};

static GParamSpec*
  feature_state_selector_properties[FEATURE_STATE_SELECTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiFeatureStateSelector,
  maplibre_native_ffi_feature_state_selector, G_TYPE_OBJECT
)

static void maplibre_native_ffi_feature_state_selector_finalize(
  GObject* object
) {
  MaplibreNativeFfiFeatureStateSelector* self =
    MAPLIBRE_NATIVE_FFI_FEATURE_STATE_SELECTOR(object);
  g_free(self->source_id);
  g_free(self->source_layer_id);
  g_clear_pointer(&self->feature_id, maplibre_native_ffi_feature_id_free);
  g_free(self->state_key);
  G_OBJECT_CLASS(maplibre_native_ffi_feature_state_selector_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_feature_state_selector_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiFeatureStateSelector* self =
    MAPLIBRE_NATIVE_FFI_FEATURE_STATE_SELECTOR(object);
  switch (id) {
    case FEATURE_STATE_SELECTOR_PROP_SOURCE_ID:
      g_value_set_string(value, self->source_id);
      break;
    case FEATURE_STATE_SELECTOR_PROP_SOURCE_LAYER_ID:
      g_value_set_string(value, self->source_layer_id);
      break;
    case FEATURE_STATE_SELECTOR_PROP_FEATURE_ID:
      g_value_set_boxed(value, self->feature_id);
      break;
    case FEATURE_STATE_SELECTOR_PROP_STATE_KEY:
      g_value_set_string(value, self->state_key);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_feature_state_selector_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiFeatureStateSelector* self =
    MAPLIBRE_NATIVE_FFI_FEATURE_STATE_SELECTOR(object);
  switch (id) {
    case FEATURE_STATE_SELECTOR_PROP_SOURCE_ID:
      maplibre_native_ffi_feature_state_selector_set_source_id(
        self, g_value_get_string(value)
      );
      break;
    case FEATURE_STATE_SELECTOR_PROP_SOURCE_LAYER_ID:
      maplibre_native_ffi_feature_state_selector_set_source_layer_id(
        self, g_value_get_string(value)
      );
      break;
    case FEATURE_STATE_SELECTOR_PROP_FEATURE_ID:
      maplibre_native_ffi_feature_state_selector_set_feature_id(
        self, g_value_get_boxed(value)
      );
      break;
    case FEATURE_STATE_SELECTOR_PROP_STATE_KEY:
      maplibre_native_ffi_feature_state_selector_set_state_key(
        self, g_value_get_string(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_feature_state_selector_class_init(
  MaplibreNativeFfiFeatureStateSelectorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_feature_state_selector_finalize;
  object_class->get_property =
    maplibre_native_ffi_feature_state_selector_get_property;
  object_class->set_property =
    maplibre_native_ffi_feature_state_selector_set_property;
  feature_state_selector_properties[FEATURE_STATE_SELECTOR_PROP_SOURCE_ID] =
    g_param_spec_string("source-id", NULL, NULL, "", PROPERTY_FLAGS);
  feature_state_selector_properties
    [FEATURE_STATE_SELECTOR_PROP_SOURCE_LAYER_ID] =
      g_param_spec_string("source-layer-id", NULL, NULL, "", PROPERTY_FLAGS);
  feature_state_selector_properties[FEATURE_STATE_SELECTOR_PROP_FEATURE_ID] =
    g_param_spec_boxed(
      "feature-id", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_FEATURE_ID,
      PROPERTY_FLAGS
    );
  feature_state_selector_properties[FEATURE_STATE_SELECTOR_PROP_STATE_KEY] =
    g_param_spec_string("state-key", NULL, NULL, "", PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, FEATURE_STATE_SELECTOR_N_PROPERTIES,
    feature_state_selector_properties
  );
}

static void maplibre_native_ffi_feature_state_selector_init(
  MaplibreNativeFfiFeatureStateSelector* self
) {
  self->source_id = g_strdup("");
  self->source_layer_id = g_strdup("");
  self->state_key = g_strdup("");
}

MaplibreNativeFfiFeatureStateSelector*
maplibre_native_ffi_feature_state_selector_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_FEATURE_STATE_SELECTOR, NULL);
}

const char* maplibre_native_ffi_feature_state_selector_get_source_id(
  MaplibreNativeFfiFeatureStateSelector* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_FEATURE_STATE_SELECTOR(self), NULL
  );
  return self->source_id;
}

void maplibre_native_ffi_feature_state_selector_set_source_id(
  MaplibreNativeFfiFeatureStateSelector* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_FEATURE_STATE_SELECTOR(self));
  char* previous = self->source_id;
  self->source_id = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->source_id) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      feature_state_selector_properties[FEATURE_STATE_SELECTOR_PROP_SOURCE_ID]
    );
}

const char* maplibre_native_ffi_feature_state_selector_get_source_layer_id(
  MaplibreNativeFfiFeatureStateSelector* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_FEATURE_STATE_SELECTOR(self), NULL
  );
  return self->source_layer_id;
}

void maplibre_native_ffi_feature_state_selector_set_source_layer_id(
  MaplibreNativeFfiFeatureStateSelector* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_FEATURE_STATE_SELECTOR(self));
  char* previous = self->source_layer_id;
  self->source_layer_id = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->source_layer_id) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), feature_state_selector_properties
                        [FEATURE_STATE_SELECTOR_PROP_SOURCE_LAYER_ID]
    );
}

MaplibreNativeFfiFeatureId*
maplibre_native_ffi_feature_state_selector_get_feature_id(
  MaplibreNativeFfiFeatureStateSelector* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_FEATURE_STATE_SELECTOR(self), NULL
  );
  return self->feature_id;
}

gboolean maplibre_native_ffi_feature_state_selector_has_feature_id(
  MaplibreNativeFfiFeatureStateSelector* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_FEATURE_STATE_SELECTOR(self), FALSE
  );
  return (self->fields & MLN_FEATURE_STATE_SELECTOR_FEATURE_ID) != 0;
}

void maplibre_native_ffi_feature_state_selector_set_feature_id(
  MaplibreNativeFfiFeatureStateSelector* self,
  const MaplibreNativeFfiFeatureId* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_FEATURE_STATE_SELECTOR(self));
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiFeatureId* previous = self->feature_id;
  self->feature_id = maplibre_native_ffi_feature_id_copy(value);
  if (previous != NULL) maplibre_native_ffi_feature_id_free(previous);
  self->fields |= MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
  g_object_notify_by_pspec(
    G_OBJECT(self),
    feature_state_selector_properties[FEATURE_STATE_SELECTOR_PROP_FEATURE_ID]
  );
}

const char* maplibre_native_ffi_feature_state_selector_get_state_key(
  MaplibreNativeFfiFeatureStateSelector* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_FEATURE_STATE_SELECTOR(self), NULL
  );
  return self->state_key;
}

void maplibre_native_ffi_feature_state_selector_set_state_key(
  MaplibreNativeFfiFeatureStateSelector* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_FEATURE_STATE_SELECTOR(self));
  char* previous = self->state_key;
  self->state_key = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->state_key) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      feature_state_selector_properties[FEATURE_STATE_SELECTOR_PROP_STATE_KEY]
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

struct _MaplibreNativeFfiFramePacerOptions {
  GObject parent_instance;
  guint64 fields;
  guint32 maximum_frames_per_second;
};

enum {
  FRAME_PACER_OPTIONS_PROP_0,
  FRAME_PACER_OPTIONS_PROP_MAXIMUM_FRAMES_PER_SECOND,
  FRAME_PACER_OPTIONS_N_PROPERTIES
};

static GParamSpec*
  frame_pacer_options_properties[FRAME_PACER_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiFramePacerOptions, maplibre_native_ffi_frame_pacer_options,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_frame_pacer_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiFramePacerOptions* self =
    MAPLIBRE_NATIVE_FFI_FRAME_PACER_OPTIONS(object);
  switch (id) {
    case FRAME_PACER_OPTIONS_PROP_MAXIMUM_FRAMES_PER_SECOND:
      g_value_set_uint(value, self->maximum_frames_per_second);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_frame_pacer_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiFramePacerOptions* self =
    MAPLIBRE_NATIVE_FFI_FRAME_PACER_OPTIONS(object);
  switch (id) {
    case FRAME_PACER_OPTIONS_PROP_MAXIMUM_FRAMES_PER_SECOND:
      maplibre_native_ffi_frame_pacer_options_set_maximum_frames_per_second(
        self, g_value_get_uint(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_frame_pacer_options_class_init(
  MaplibreNativeFfiFramePacerOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_frame_pacer_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_frame_pacer_options_set_property;
  frame_pacer_options_properties
    [FRAME_PACER_OPTIONS_PROP_MAXIMUM_FRAMES_PER_SECOND] = g_param_spec_uint(
      "maximum-frames-per-second", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, FRAME_PACER_OPTIONS_N_PROPERTIES,
    frame_pacer_options_properties
  );
}

static void maplibre_native_ffi_frame_pacer_options_init(
  MaplibreNativeFfiFramePacerOptions* self
) {
  (void)self;
}

MaplibreNativeFfiFramePacerOptions* maplibre_native_ffi_frame_pacer_options_new(
  void
) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_FRAME_PACER_OPTIONS, NULL);
}

guint32 maplibre_native_ffi_frame_pacer_options_get_maximum_frames_per_second(
  MaplibreNativeFfiFramePacerOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_FRAME_PACER_OPTIONS(self), 0);
  return self->maximum_frames_per_second;
}

gboolean maplibre_native_ffi_frame_pacer_options_has_maximum_frames_per_second(
  MaplibreNativeFfiFramePacerOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_FRAME_PACER_OPTIONS(self), FALSE);
  return (self->fields & MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND) != 0;
}

void maplibre_native_ffi_frame_pacer_options_set_maximum_frames_per_second(
  MaplibreNativeFfiFramePacerOptions* self, guint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_FRAME_PACER_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND) != 0;
  gboolean changed = self->maximum_frames_per_second != value;
  self->maximum_frames_per_second = value;
  self->fields |= MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), frame_pacer_options_properties
                        [FRAME_PACER_OPTIONS_PROP_MAXIMUM_FRAMES_PER_SECOND]
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

G_STATIC_ASSERT(sizeof(MaplibreNativeFfiVec3) == sizeof(mln_vec3));
SAME_MEMBER(MaplibreNativeFfiVec3, mln_vec3, x);
SAME_MEMBER(MaplibreNativeFfiVec3, mln_vec3, y);
SAME_MEMBER(MaplibreNativeFfiVec3, mln_vec3, z);

void maplibre_native_ffi_vec3_to_native(
  const MaplibreNativeFfiVec3* self, mln_vec3* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_vec3_read(
  MaplibreNativeFfiVec3* out, const mln_vec3* in
) {
  memcpy(out, in, sizeof *out);
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

G_STATIC_ASSERT(sizeof(MaplibreNativeFfiQuaternion) == sizeof(mln_quaternion));
SAME_MEMBER(MaplibreNativeFfiQuaternion, mln_quaternion, x);
SAME_MEMBER(MaplibreNativeFfiQuaternion, mln_quaternion, y);
SAME_MEMBER(MaplibreNativeFfiQuaternion, mln_quaternion, z);
SAME_MEMBER(MaplibreNativeFfiQuaternion, mln_quaternion, w);

void maplibre_native_ffi_quaternion_to_native(
  const MaplibreNativeFfiQuaternion* self, mln_quaternion* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_quaternion_read(
  MaplibreNativeFfiQuaternion* out, const mln_quaternion* in
) {
  memcpy(out, in, sizeof *out);
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

G_STATIC_ASSERT(sizeof(MaplibreNativeFfiFreeCamera) == sizeof(mln_free_camera));
SAME_MEMBER(MaplibreNativeFfiFreeCamera, mln_free_camera, position);
SAME_MEMBER(MaplibreNativeFfiFreeCamera, mln_free_camera, orientation);

void maplibre_native_ffi_free_camera_read(
  MaplibreNativeFfiFreeCamera* out, const mln_free_camera* in
) {
  memcpy(out, in, sizeof *out);
}

MaplibreNativeFfiFreeCamera* maplibre_native_ffi_free_camera_from_native(
  const mln_free_camera* in
) {
  MaplibreNativeFfiFreeCamera* self = g_new0(MaplibreNativeFfiFreeCamera, 1);
  maplibre_native_ffi_free_camera_read(self, in);
  return self;
}

struct _MaplibreNativeFfiFreeCameraOptions {
  GObject parent_instance;
  guint64 fields;
  MaplibreNativeFfiVec3 position;
  MaplibreNativeFfiQuaternion orientation;
};

enum {
  FREE_CAMERA_OPTIONS_PROP_0,
  FREE_CAMERA_OPTIONS_PROP_POSITION,
  FREE_CAMERA_OPTIONS_PROP_ORIENTATION,
  FREE_CAMERA_OPTIONS_N_PROPERTIES
};

static GParamSpec*
  free_camera_options_properties[FREE_CAMERA_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiFreeCameraOptions, maplibre_native_ffi_free_camera_options,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_free_camera_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiFreeCameraOptions* self =
    MAPLIBRE_NATIVE_FFI_FREE_CAMERA_OPTIONS(object);
  switch (id) {
    case FREE_CAMERA_OPTIONS_PROP_POSITION:
      g_value_set_boxed(value, &self->position);
      break;
    case FREE_CAMERA_OPTIONS_PROP_ORIENTATION:
      g_value_set_boxed(value, &self->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_free_camera_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiFreeCameraOptions* self =
    MAPLIBRE_NATIVE_FFI_FREE_CAMERA_OPTIONS(object);
  switch (id) {
    case FREE_CAMERA_OPTIONS_PROP_POSITION:
      maplibre_native_ffi_free_camera_options_set_position(
        self, g_value_get_boxed(value)
      );
      break;
    case FREE_CAMERA_OPTIONS_PROP_ORIENTATION:
      maplibre_native_ffi_free_camera_options_set_orientation(
        self, g_value_get_boxed(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_free_camera_options_class_init(
  MaplibreNativeFfiFreeCameraOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_free_camera_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_free_camera_options_set_property;
  free_camera_options_properties[FREE_CAMERA_OPTIONS_PROP_POSITION] =
    g_param_spec_boxed(
      "position", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_VEC3, PROPERTY_FLAGS
    );
  free_camera_options_properties[FREE_CAMERA_OPTIONS_PROP_ORIENTATION] =
    g_param_spec_boxed(
      "orientation", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_QUATERNION,
      PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, FREE_CAMERA_OPTIONS_N_PROPERTIES,
    free_camera_options_properties
  );
}

static void maplibre_native_ffi_free_camera_options_init(
  MaplibreNativeFfiFreeCameraOptions* self
) {
  (void)self;
}

MaplibreNativeFfiFreeCameraOptions* maplibre_native_ffi_free_camera_options_new(
  void
) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_FREE_CAMERA_OPTIONS, NULL);
}

MaplibreNativeFfiVec3* maplibre_native_ffi_free_camera_options_get_position(
  MaplibreNativeFfiFreeCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_FREE_CAMERA_OPTIONS(self), NULL);
  return (MaplibreNativeFfiVec3*)&self->position;
}

gboolean maplibre_native_ffi_free_camera_options_has_position(
  MaplibreNativeFfiFreeCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_FREE_CAMERA_OPTIONS(self), FALSE);
  return (self->fields & MLN_FREE_CAMERA_OPTION_POSITION) != 0;
}

void maplibre_native_ffi_free_camera_options_set_position(
  MaplibreNativeFfiFreeCameraOptions* self, const MaplibreNativeFfiVec3* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_FREE_CAMERA_OPTIONS(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_FREE_CAMERA_OPTION_POSITION) != 0;
  gboolean changed = memcmp(&self->position, value, sizeof self->position) != 0;
  self->position = *value;
  self->fields |= MLN_FREE_CAMERA_OPTION_POSITION;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      free_camera_options_properties[FREE_CAMERA_OPTIONS_PROP_POSITION]
    );
}

MaplibreNativeFfiQuaternion*
maplibre_native_ffi_free_camera_options_get_orientation(
  MaplibreNativeFfiFreeCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_FREE_CAMERA_OPTIONS(self), NULL);
  return (MaplibreNativeFfiQuaternion*)&self->orientation;
}

gboolean maplibre_native_ffi_free_camera_options_has_orientation(
  MaplibreNativeFfiFreeCameraOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_FREE_CAMERA_OPTIONS(self), FALSE);
  return (self->fields & MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0;
}

void maplibre_native_ffi_free_camera_options_set_orientation(
  MaplibreNativeFfiFreeCameraOptions* self,
  const MaplibreNativeFfiQuaternion* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_FREE_CAMERA_OPTIONS(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0;
  gboolean changed =
    memcmp(&self->orientation, value, sizeof self->orientation) != 0;
  self->orientation = *value;
  self->fields |= MLN_FREE_CAMERA_OPTION_ORIENTATION;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      free_camera_options_properties[FREE_CAMERA_OPTIONS_PROP_ORIENTATION]
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

struct _MaplibreNativeFfiGeoJsonSourceOptions {
  GObject parent_instance;
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

enum {
  GEO_JSON_SOURCE_OPTIONS_PROP_0,
  GEO_JSON_SOURCE_OPTIONS_PROP_MIN_ZOOM,
  GEO_JSON_SOURCE_OPTIONS_PROP_MAX_ZOOM,
  GEO_JSON_SOURCE_OPTIONS_PROP_TOLERANCE,
  GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_MAX_ZOOM,
  GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_PROPERTIES,
  GEO_JSON_SOURCE_OPTIONS_PROP_BUFFER,
  GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_RADIUS,
  GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_MIN_POINTS,
  GEO_JSON_SOURCE_OPTIONS_PROP_LINE_METRICS,
  GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER,
  GEO_JSON_SOURCE_OPTIONS_N_PROPERTIES
};

static GParamSpec*
  geo_json_source_options_properties[GEO_JSON_SOURCE_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiGeoJsonSourceOptions,
  maplibre_native_ffi_geo_json_source_options, G_TYPE_OBJECT
)

static void maplibre_native_ffi_geo_json_source_options_finalize(
  GObject* object
) {
  MaplibreNativeFfiGeoJsonSourceOptions* self =
    MAPLIBRE_NATIVE_FFI_GEO_JSON_SOURCE_OPTIONS(object);
  g_free(self->cluster_properties);
  G_OBJECT_CLASS(maplibre_native_ffi_geo_json_source_options_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_geo_json_source_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiGeoJsonSourceOptions* self =
    MAPLIBRE_NATIVE_FFI_GEO_JSON_SOURCE_OPTIONS(object);
  switch (id) {
    case GEO_JSON_SOURCE_OPTIONS_PROP_MIN_ZOOM:
      g_value_set_double(value, self->min_zoom);
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_MAX_ZOOM:
      g_value_set_double(value, self->max_zoom);
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_TOLERANCE:
      g_value_set_double(value, self->tolerance);
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_MAX_ZOOM:
      g_value_set_double(value, self->cluster_max_zoom);
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_PROPERTIES:
      g_value_set_string(value, self->cluster_properties);
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_BUFFER:
      g_value_set_uint(value, self->buffer);
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_RADIUS:
      g_value_set_uint(value, self->cluster_radius);
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_MIN_POINTS:
      g_value_set_uint64(value, self->cluster_min_points);
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_LINE_METRICS:
      g_value_set_boolean(value, self->line_metrics);
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER:
      g_value_set_boolean(value, self->cluster);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_geo_json_source_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiGeoJsonSourceOptions* self =
    MAPLIBRE_NATIVE_FFI_GEO_JSON_SOURCE_OPTIONS(object);
  switch (id) {
    case GEO_JSON_SOURCE_OPTIONS_PROP_MIN_ZOOM:
      maplibre_native_ffi_geo_json_source_options_set_min_zoom(
        self, g_value_get_double(value)
      );
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_MAX_ZOOM:
      maplibre_native_ffi_geo_json_source_options_set_max_zoom(
        self, g_value_get_double(value)
      );
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_TOLERANCE:
      maplibre_native_ffi_geo_json_source_options_set_tolerance(
        self, g_value_get_double(value)
      );
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_MAX_ZOOM:
      maplibre_native_ffi_geo_json_source_options_set_cluster_max_zoom(
        self, g_value_get_double(value)
      );
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_PROPERTIES:
      maplibre_native_ffi_geo_json_source_options_set_cluster_properties(
        self, g_value_get_string(value)
      );
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_BUFFER:
      maplibre_native_ffi_geo_json_source_options_set_buffer(
        self, g_value_get_uint(value)
      );
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_RADIUS:
      maplibre_native_ffi_geo_json_source_options_set_cluster_radius(
        self, g_value_get_uint(value)
      );
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_MIN_POINTS:
      maplibre_native_ffi_geo_json_source_options_set_cluster_min_points(
        self, g_value_get_uint64(value)
      );
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_LINE_METRICS:
      maplibre_native_ffi_geo_json_source_options_set_line_metrics(
        self, g_value_get_boolean(value)
      );
      break;
    case GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER:
      maplibre_native_ffi_geo_json_source_options_set_cluster(
        self, g_value_get_boolean(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_geo_json_source_options_class_init(
  MaplibreNativeFfiGeoJsonSourceOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_geo_json_source_options_finalize;
  object_class->get_property =
    maplibre_native_ffi_geo_json_source_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_geo_json_source_options_set_property;
  geo_json_source_options_properties[GEO_JSON_SOURCE_OPTIONS_PROP_MIN_ZOOM] =
    g_param_spec_double(
      "min-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  geo_json_source_options_properties[GEO_JSON_SOURCE_OPTIONS_PROP_MAX_ZOOM] =
    g_param_spec_double(
      "max-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  geo_json_source_options_properties[GEO_JSON_SOURCE_OPTIONS_PROP_TOLERANCE] =
    g_param_spec_double(
      "tolerance", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  geo_json_source_options_properties
    [GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_MAX_ZOOM] = g_param_spec_double(
      "cluster-max-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  geo_json_source_options_properties
    [GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_PROPERTIES] =
      g_param_spec_string("cluster-properties", NULL, NULL, "", PROPERTY_FLAGS);
  geo_json_source_options_properties[GEO_JSON_SOURCE_OPTIONS_PROP_BUFFER] =
    g_param_spec_uint("buffer", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS);
  geo_json_source_options_properties
    [GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_RADIUS] = g_param_spec_uint(
      "cluster-radius", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
    );
  geo_json_source_options_properties
    [GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_MIN_POINTS] = g_param_spec_uint64(
      "cluster-min-points", NULL, NULL, 0, G_MAXUINT64, 0, PROPERTY_FLAGS
    );
  geo_json_source_options_properties
    [GEO_JSON_SOURCE_OPTIONS_PROP_LINE_METRICS] =
      g_param_spec_boolean("line-metrics", NULL, NULL, FALSE, PROPERTY_FLAGS);
  geo_json_source_options_properties[GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER] =
    g_param_spec_boolean("cluster", NULL, NULL, FALSE, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, GEO_JSON_SOURCE_OPTIONS_N_PROPERTIES,
    geo_json_source_options_properties
  );
}

static void maplibre_native_ffi_geo_json_source_options_init(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  self->cluster_properties = g_strdup("");
}

MaplibreNativeFfiGeoJsonSourceOptions*
maplibre_native_ffi_geo_json_source_options_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_GEO_JSON_SOURCE_OPTIONS, NULL);
}

gdouble maplibre_native_ffi_geo_json_source_options_get_min_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), 0);
  return self->min_zoom;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_min_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_min_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM) != 0;
  gboolean changed = self->min_zoom != value;
  self->min_zoom = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      geo_json_source_options_properties[GEO_JSON_SOURCE_OPTIONS_PROP_MIN_ZOOM]
    );
}

gdouble maplibre_native_ffi_geo_json_source_options_get_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), 0);
  return self->max_zoom;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM) != 0;
  gboolean changed = self->max_zoom != value;
  self->max_zoom = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      geo_json_source_options_properties[GEO_JSON_SOURCE_OPTIONS_PROP_MAX_ZOOM]
    );
}

gdouble maplibre_native_ffi_geo_json_source_options_get_tolerance(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), 0);
  return self->tolerance;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_tolerance(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_tolerance(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE) != 0;
  gboolean changed = self->tolerance != value;
  self->tolerance = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      geo_json_source_options_properties[GEO_JSON_SOURCE_OPTIONS_PROP_TOLERANCE]
    );
}

gdouble maplibre_native_ffi_geo_json_source_options_get_cluster_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), 0);
  return self->cluster_max_zoom;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_cluster_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_cluster_max_zoom(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM) != 0;
  gboolean changed = self->cluster_max_zoom != value;
  self->cluster_max_zoom = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), geo_json_source_options_properties
                        [GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_MAX_ZOOM]
    );
}

const char* maplibre_native_ffi_geo_json_source_options_get_cluster_properties(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), NULL
  );
  return self->cluster_properties;
}

void maplibre_native_ffi_geo_json_source_options_set_cluster_properties(
  MaplibreNativeFfiGeoJsonSourceOptions* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self));
  char* previous = self->cluster_properties;
  self->cluster_properties = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->cluster_properties) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), geo_json_source_options_properties
                        [GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_PROPERTIES]
    );
}

guint32 maplibre_native_ffi_geo_json_source_options_get_buffer(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), 0);
  return self->buffer;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_buffer(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_BUFFER) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_buffer(
  MaplibreNativeFfiGeoJsonSourceOptions* self, guint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_GEO_JSON_SOURCE_OPTION_BUFFER) != 0;
  gboolean changed = self->buffer != value;
  self->buffer = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_BUFFER;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      geo_json_source_options_properties[GEO_JSON_SOURCE_OPTIONS_PROP_BUFFER]
    );
}

guint32 maplibre_native_ffi_geo_json_source_options_get_cluster_radius(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), 0);
  return self->cluster_radius;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_cluster_radius(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_cluster_radius(
  MaplibreNativeFfiGeoJsonSourceOptions* self, guint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS) != 0;
  gboolean changed = self->cluster_radius != value;
  self->cluster_radius = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), geo_json_source_options_properties
                        [GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_RADIUS]
    );
}

guint64 maplibre_native_ffi_geo_json_source_options_get_cluster_min_points(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), 0);
  return self->cluster_min_points;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_cluster_min_points(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_cluster_min_points(
  MaplibreNativeFfiGeoJsonSourceOptions* self, guint64 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS) != 0;
  gboolean changed = self->cluster_min_points != value;
  self->cluster_min_points = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), geo_json_source_options_properties
                        [GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER_MIN_POINTS]
    );
}

gboolean maplibre_native_ffi_geo_json_source_options_get_line_metrics(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), FALSE
  );
  return self->line_metrics;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_line_metrics(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_line_metrics(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gboolean value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS) != 0;
  gboolean changed = self->line_metrics != value;
  self->line_metrics = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), geo_json_source_options_properties
                        [GEO_JSON_SOURCE_OPTIONS_PROP_LINE_METRICS]
    );
}

gboolean maplibre_native_ffi_geo_json_source_options_get_cluster(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), FALSE
  );
  return self->cluster;
}

gboolean maplibre_native_ffi_geo_json_source_options_has_cluster(
  MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER) != 0;
}

void maplibre_native_ffi_geo_json_source_options_set_cluster(
  MaplibreNativeFfiGeoJsonSourceOptions* self, gboolean value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_GEO_JSON_SOURCE_OPTION_CLUSTER) != 0;
  gboolean changed = self->cluster != value;
  self->cluster = value;
  self->fields |= MLN_GEO_JSON_SOURCE_OPTION_CLUSTER;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      geo_json_source_options_properties[GEO_JSON_SOURCE_OPTIONS_PROP_CLUSTER]
    );
}

MaplibreNativeFfiGeoJsonSourceOptions*
maplibre_native_ffi_geo_json_source_options_copy(
  const MaplibreNativeFfiGeoJsonSourceOptions* self
) {
  MaplibreNativeFfiGeoJsonSourceOptions* copy =
    maplibre_native_ffi_geo_json_source_options_new();
  copy->fields = self->fields;
  copy->min_zoom = self->min_zoom;
  copy->max_zoom = self->max_zoom;
  copy->tolerance = self->tolerance;
  copy->cluster_max_zoom = self->cluster_max_zoom;
  g_free(copy->cluster_properties);
  copy->cluster_properties = g_strdup(self->cluster_properties);
  copy->buffer = self->buffer;
  copy->cluster_radius = self->cluster_radius;
  copy->cluster_min_points = self->cluster_min_points;
  copy->line_metrics = self->line_metrics;
  copy->cluster = self->cluster;
  return copy;
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
  GObject parent_instance;
  char* name;
  char* value;
};

enum {
  HTTP_HEADER_PROP_0,
  HTTP_HEADER_PROP_NAME,
  HTTP_HEADER_PROP_VALUE,
  HTTP_HEADER_N_PROPERTIES
};

static GParamSpec* http_header_properties[HTTP_HEADER_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiHttpHeader, maplibre_native_ffi_http_header, G_TYPE_OBJECT
)

static void maplibre_native_ffi_http_header_finalize(GObject* object) {
  MaplibreNativeFfiHttpHeader* self = MAPLIBRE_NATIVE_FFI_HTTP_HEADER(object);
  g_free(self->name);
  g_free(self->value);
  G_OBJECT_CLASS(maplibre_native_ffi_http_header_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_http_header_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiHttpHeader* self = MAPLIBRE_NATIVE_FFI_HTTP_HEADER(object);
  switch (id) {
    case HTTP_HEADER_PROP_NAME:
      g_value_set_string(value, self->name);
      break;
    case HTTP_HEADER_PROP_VALUE:
      g_value_set_string(value, self->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_http_header_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiHttpHeader* self = MAPLIBRE_NATIVE_FFI_HTTP_HEADER(object);
  switch (id) {
    case HTTP_HEADER_PROP_NAME:
      maplibre_native_ffi_http_header_set_name(self, g_value_get_string(value));
      break;
    case HTTP_HEADER_PROP_VALUE:
      maplibre_native_ffi_http_header_set_value(
        self, g_value_get_string(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_http_header_class_init(
  MaplibreNativeFfiHttpHeaderClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_http_header_finalize;
  object_class->get_property = maplibre_native_ffi_http_header_get_property;
  object_class->set_property = maplibre_native_ffi_http_header_set_property;
  http_header_properties[HTTP_HEADER_PROP_NAME] =
    g_param_spec_string("name", NULL, NULL, "", PROPERTY_FLAGS);
  http_header_properties[HTTP_HEADER_PROP_VALUE] =
    g_param_spec_string("value", NULL, NULL, "", PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, HTTP_HEADER_N_PROPERTIES, http_header_properties
  );
}

static void maplibre_native_ffi_http_header_init(
  MaplibreNativeFfiHttpHeader* self
) {
  self->name = g_strdup("");
  self->value = g_strdup("");
}

MaplibreNativeFfiHttpHeader* maplibre_native_ffi_http_header_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_HTTP_HEADER, NULL);
}

const char* maplibre_native_ffi_http_header_get_name(
  MaplibreNativeFfiHttpHeader* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER(self), NULL);
  return self->name;
}

void maplibre_native_ffi_http_header_set_name(
  MaplibreNativeFfiHttpHeader* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER(self));
  char* previous = self->name;
  self->name = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->name) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), http_header_properties[HTTP_HEADER_PROP_NAME]
    );
}

const char* maplibre_native_ffi_http_header_get_value(
  MaplibreNativeFfiHttpHeader* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER(self), NULL);
  return self->value;
}

void maplibre_native_ffi_http_header_set_value(
  MaplibreNativeFfiHttpHeader* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER(self));
  char* previous = self->value;
  self->value = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->value) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), http_header_properties[HTTP_HEADER_PROP_VALUE]
    );
}

MaplibreNativeFfiHttpHeader* maplibre_native_ffi_http_header_copy(
  const MaplibreNativeFfiHttpHeader* self
) {
  MaplibreNativeFfiHttpHeader* copy = maplibre_native_ffi_http_header_new();
  g_free(copy->name);
  copy->name = g_strdup(self->name);
  g_free(copy->value);
  copy->value = g_strdup(self->value);
  return copy;
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

struct _MaplibreNativeFfiHttpHeaderRule {
  GObject parent_instance;
  MaplibreNativeFfiResourceKindFlag kinds;
  MaplibreNativeFfiUrlMatchFlag flags;
  char* url;
  GPtrArray* headers;
};

enum {
  HTTP_HEADER_RULE_PROP_0,
  HTTP_HEADER_RULE_PROP_KINDS,
  HTTP_HEADER_RULE_PROP_FLAGS,
  HTTP_HEADER_RULE_PROP_URL,
  HTTP_HEADER_RULE_N_PROPERTIES
};

static GParamSpec* http_header_rule_properties[HTTP_HEADER_RULE_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiHttpHeaderRule, maplibre_native_ffi_http_header_rule,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_http_header_rule_finalize(GObject* object) {
  MaplibreNativeFfiHttpHeaderRule* self =
    MAPLIBRE_NATIVE_FFI_HTTP_HEADER_RULE(object);
  g_free(self->url);
  g_clear_pointer(&self->headers, g_ptr_array_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_http_header_rule_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_http_header_rule_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiHttpHeaderRule* self =
    MAPLIBRE_NATIVE_FFI_HTTP_HEADER_RULE(object);
  switch (id) {
    case HTTP_HEADER_RULE_PROP_KINDS:
      g_value_set_flags(value, self->kinds);
      break;
    case HTTP_HEADER_RULE_PROP_FLAGS:
      g_value_set_flags(value, self->flags);
      break;
    case HTTP_HEADER_RULE_PROP_URL:
      g_value_set_string(value, self->url);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_http_header_rule_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiHttpHeaderRule* self =
    MAPLIBRE_NATIVE_FFI_HTTP_HEADER_RULE(object);
  switch (id) {
    case HTTP_HEADER_RULE_PROP_KINDS:
      maplibre_native_ffi_http_header_rule_set_kinds(
        self, g_value_get_flags(value)
      );
      break;
    case HTTP_HEADER_RULE_PROP_FLAGS:
      maplibre_native_ffi_http_header_rule_set_flags(
        self, g_value_get_flags(value)
      );
      break;
    case HTTP_HEADER_RULE_PROP_URL:
      maplibre_native_ffi_http_header_rule_set_url(
        self, g_value_get_string(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_http_header_rule_class_init(
  MaplibreNativeFfiHttpHeaderRuleClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_http_header_rule_finalize;
  object_class->get_property =
    maplibre_native_ffi_http_header_rule_get_property;
  object_class->set_property =
    maplibre_native_ffi_http_header_rule_set_property;
  http_header_rule_properties[HTTP_HEADER_RULE_PROP_KINDS] = g_param_spec_flags(
    "kinds", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_KIND_FLAG, 0,
    PROPERTY_FLAGS
  );
  http_header_rule_properties[HTTP_HEADER_RULE_PROP_FLAGS] = g_param_spec_flags(
    "flags", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_URL_MATCH_FLAG, 0,
    PROPERTY_FLAGS
  );
  http_header_rule_properties[HTTP_HEADER_RULE_PROP_URL] =
    g_param_spec_string("url", NULL, NULL, "", PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, HTTP_HEADER_RULE_N_PROPERTIES, http_header_rule_properties
  );
}

static void maplibre_native_ffi_http_header_rule_init(
  MaplibreNativeFfiHttpHeaderRule* self
) {
  self->url = g_strdup("");
  self->headers =
    g_ptr_array_new_with_free_func((GDestroyNotify)g_object_unref);
}

MaplibreNativeFfiHttpHeaderRule* maplibre_native_ffi_http_header_rule_new(
  void
) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_HTTP_HEADER_RULE, NULL);
}

MaplibreNativeFfiResourceKindFlag
maplibre_native_ffi_http_header_rule_get_kinds(
  MaplibreNativeFfiHttpHeaderRule* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER_RULE(self),
    (MaplibreNativeFfiResourceKindFlag)0
  );
  return self->kinds;
}

void maplibre_native_ffi_http_header_rule_set_kinds(
  MaplibreNativeFfiHttpHeaderRule* self, MaplibreNativeFfiResourceKindFlag value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER_RULE(self));
  gboolean changed = self->kinds != value;
  self->kinds = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), http_header_rule_properties[HTTP_HEADER_RULE_PROP_KINDS]
    );
}

MaplibreNativeFfiUrlMatchFlag maplibre_native_ffi_http_header_rule_get_flags(
  MaplibreNativeFfiHttpHeaderRule* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER_RULE(self),
    (MaplibreNativeFfiUrlMatchFlag)0
  );
  return self->flags;
}

void maplibre_native_ffi_http_header_rule_set_flags(
  MaplibreNativeFfiHttpHeaderRule* self, MaplibreNativeFfiUrlMatchFlag value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER_RULE(self));
  gboolean changed = self->flags != value;
  self->flags = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), http_header_rule_properties[HTTP_HEADER_RULE_PROP_FLAGS]
    );
}

const char* maplibre_native_ffi_http_header_rule_get_url(
  MaplibreNativeFfiHttpHeaderRule* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER_RULE(self), NULL);
  return self->url;
}

void maplibre_native_ffi_http_header_rule_set_url(
  MaplibreNativeFfiHttpHeaderRule* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER_RULE(self));
  char* previous = self->url;
  self->url = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->url) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), http_header_rule_properties[HTTP_HEADER_RULE_PROP_URL]
    );
}

GPtrArray* maplibre_native_ffi_http_header_rule_get_headers(
  MaplibreNativeFfiHttpHeaderRule* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER_RULE(self), NULL);
  return self->headers;
}

void maplibre_native_ffi_http_header_rule_set_headers(
  MaplibreNativeFfiHttpHeaderRule* self,
  MaplibreNativeFfiHttpHeader* const* value, gsize value_count
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER_RULE(self));
  for (gsize index = 0; index < value_count; index++) {
    g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER(value[index]));
  }
  GPtrArray* previous = self->headers;
  self->headers =
    g_ptr_array_new_full((guint)value_count, (GDestroyNotify)g_object_unref);
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->headers, maplibre_native_ffi_http_header_copy(value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

MaplibreNativeFfiHttpHeaderRule* maplibre_native_ffi_http_header_rule_copy(
  const MaplibreNativeFfiHttpHeaderRule* self
) {
  MaplibreNativeFfiHttpHeaderRule* copy =
    maplibre_native_ffi_http_header_rule_new();
  copy->kinds = self->kinds;
  copy->flags = self->flags;
  g_free(copy->url);
  copy->url = g_strdup(self->url);
  g_clear_pointer(&copy->headers, g_ptr_array_unref);
  copy->headers =
    self->headers != NULL
      ? g_ptr_array_copy(
          self->headers, maplibre_native_ffi_http_header_copy_func, NULL
        )
      : NULL;
  if (copy->headers != NULL)
    g_ptr_array_set_free_func(copy->headers, (GDestroyNotify)g_object_unref);
  return copy;
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
  self->rules = g_ptr_array_new_with_free_func((GDestroyNotify)g_object_unref);
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
    g_ptr_array_set_free_func(copy->rules, (GDestroyNotify)g_object_unref);
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
  for (gsize index = 0; index < value_count; index++) {
    g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_HTTP_HEADER_RULE(value[index]));
  }
  GPtrArray* previous = self->rules;
  self->rules =
    g_ptr_array_new_full((guint)value_count, (GDestroyNotify)g_object_unref);
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->rules, maplibre_native_ffi_http_header_rule_copy(value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiImageContent) == sizeof(mln_image_content)
);
SAME_MEMBER(MaplibreNativeFfiImageContent, mln_image_content, left);
SAME_MEMBER(MaplibreNativeFfiImageContent, mln_image_content, top);
SAME_MEMBER(MaplibreNativeFfiImageContent, mln_image_content, right);
SAME_MEMBER(MaplibreNativeFfiImageContent, mln_image_content, bottom);

void maplibre_native_ffi_image_content_to_native(
  const MaplibreNativeFfiImageContent* self, mln_image_content* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_image_content_read(
  MaplibreNativeFfiImageContent* out, const mln_image_content* in
) {
  memcpy(out, in, sizeof *out);
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiImageStretch) == sizeof(mln_image_stretch)
);
SAME_MEMBER(MaplibreNativeFfiImageStretch, mln_image_stretch, from);
SAME_MEMBER(MaplibreNativeFfiImageStretch, mln_image_stretch, to);

void maplibre_native_ffi_image_stretch_to_native(
  const MaplibreNativeFfiImageStretch* self, mln_image_stretch* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_image_stretch_read(
  MaplibreNativeFfiImageStretch* out, const mln_image_stretch* in
) {
  memcpy(out, in, sizeof *out);
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
  GObject parent_instance;
  gpointer device;
};

enum {
  METAL_CONTEXT_DESCRIPTOR_PROP_0,
  METAL_CONTEXT_DESCRIPTOR_PROP_DEVICE,
  METAL_CONTEXT_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec*
  metal_context_descriptor_properties[METAL_CONTEXT_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiMetalContextDescriptor,
  maplibre_native_ffi_metal_context_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_metal_context_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMetalContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_METAL_CONTEXT_DESCRIPTOR(object);
  switch (id) {
    case METAL_CONTEXT_DESCRIPTOR_PROP_DEVICE:
      g_value_set_pointer(value, self->device);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_metal_context_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMetalContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_METAL_CONTEXT_DESCRIPTOR(object);
  switch (id) {
    case METAL_CONTEXT_DESCRIPTOR_PROP_DEVICE:
      maplibre_native_ffi_metal_context_descriptor_set_device(
        self, g_value_get_pointer(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_metal_context_descriptor_class_init(
  MaplibreNativeFfiMetalContextDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_metal_context_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_metal_context_descriptor_set_property;
  metal_context_descriptor_properties[METAL_CONTEXT_DESCRIPTOR_PROP_DEVICE] =
    g_param_spec_pointer("device", NULL, NULL, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, METAL_CONTEXT_DESCRIPTOR_N_PROPERTIES,
    metal_context_descriptor_properties
  );
}

static void maplibre_native_ffi_metal_context_descriptor_init(
  MaplibreNativeFfiMetalContextDescriptor* self
) {
  (void)self;
}

MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_metal_context_descriptor_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_METAL_CONTEXT_DESCRIPTOR, NULL);
}

gpointer maplibre_native_ffi_metal_context_descriptor_get_device(
  MaplibreNativeFfiMetalContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_CONTEXT_DESCRIPTOR(self), 0
  );
  return self->device;
}

void maplibre_native_ffi_metal_context_descriptor_set_device(
  MaplibreNativeFfiMetalContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_METAL_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->device != value;
  self->device = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      metal_context_descriptor_properties[METAL_CONTEXT_DESCRIPTOR_PROP_DEVICE]
    );
}

MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_metal_context_descriptor_copy(
  const MaplibreNativeFfiMetalContextDescriptor* self
) {
  MaplibreNativeFfiMetalContextDescriptor* copy =
    maplibre_native_ffi_metal_context_descriptor_new();
  copy->device = self->device;
  return copy;
}

void maplibre_native_ffi_metal_context_descriptor_to_native(
  const MaplibreNativeFfiMetalContextDescriptor* self,
  mln_metal_context_descriptor* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memset(out, 0, sizeof *out);
  out->device = (void*)(self->device);
}

struct _MaplibreNativeFfiIoSurfaceRingDescriptor {
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 buffer_count;
  MaplibreNativeFfiMetalContextDescriptor* context;
};

enum {
  IO_SURFACE_RING_DESCRIPTOR_PROP_0,
  IO_SURFACE_RING_DESCRIPTOR_PROP_LOGICAL_SIZE,
  IO_SURFACE_RING_DESCRIPTOR_PROP_BUFFER_COUNT,
  IO_SURFACE_RING_DESCRIPTOR_PROP_CONTEXT,
  IO_SURFACE_RING_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec* io_surface_ring_descriptor_properties
  [IO_SURFACE_RING_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiIoSurfaceRingDescriptor,
  maplibre_native_ffi_io_surface_ring_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_io_surface_ring_descriptor_finalize(
  GObject* object
) {
  MaplibreNativeFfiIoSurfaceRingDescriptor* self =
    MAPLIBRE_NATIVE_FFI_IO_SURFACE_RING_DESCRIPTOR(object);
  g_clear_object(&self->context);
  G_OBJECT_CLASS(maplibre_native_ffi_io_surface_ring_descriptor_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_io_surface_ring_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiIoSurfaceRingDescriptor* self =
    MAPLIBRE_NATIVE_FFI_IO_SURFACE_RING_DESCRIPTOR(object);
  switch (id) {
    case IO_SURFACE_RING_DESCRIPTOR_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case IO_SURFACE_RING_DESCRIPTOR_PROP_BUFFER_COUNT:
      g_value_set_uint(value, self->buffer_count);
      break;
    case IO_SURFACE_RING_DESCRIPTOR_PROP_CONTEXT:
      g_value_set_object(value, self->context);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_io_surface_ring_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiIoSurfaceRingDescriptor* self =
    MAPLIBRE_NATIVE_FFI_IO_SURFACE_RING_DESCRIPTOR(object);
  switch (id) {
    case IO_SURFACE_RING_DESCRIPTOR_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_io_surface_ring_descriptor_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case IO_SURFACE_RING_DESCRIPTOR_PROP_BUFFER_COUNT:
      maplibre_native_ffi_io_surface_ring_descriptor_set_buffer_count(
        self, g_value_get_uint(value)
      );
      break;
    case IO_SURFACE_RING_DESCRIPTOR_PROP_CONTEXT:
      maplibre_native_ffi_io_surface_ring_descriptor_set_context(
        self, g_value_get_object(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_io_surface_ring_descriptor_class_init(
  MaplibreNativeFfiIoSurfaceRingDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_io_surface_ring_descriptor_finalize;
  object_class->get_property =
    maplibre_native_ffi_io_surface_ring_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_io_surface_ring_descriptor_set_property;
  io_surface_ring_descriptor_properties
    [IO_SURFACE_RING_DESCRIPTOR_PROP_LOGICAL_SIZE] = g_param_spec_boxed(
      "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
      PROPERTY_FLAGS
    );
  io_surface_ring_descriptor_properties
    [IO_SURFACE_RING_DESCRIPTOR_PROP_BUFFER_COUNT] = g_param_spec_uint(
      "buffer-count", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
    );
  io_surface_ring_descriptor_properties
    [IO_SURFACE_RING_DESCRIPTOR_PROP_CONTEXT] = g_param_spec_object(
      "context", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_METAL_CONTEXT_DESCRIPTOR,
      PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, IO_SURFACE_RING_DESCRIPTOR_N_PROPERTIES,
    io_surface_ring_descriptor_properties
  );
}

static void maplibre_native_ffi_io_surface_ring_descriptor_init(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self
) {
  self->context = maplibre_native_ffi_metal_context_descriptor_new();
}

MaplibreNativeFfiIoSurfaceRingDescriptor*
maplibre_native_ffi_io_surface_ring_descriptor_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_IO_SURFACE_RING_DESCRIPTOR, NULL
  );
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_io_surface_ring_descriptor_get_logical_size(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_IO_SURFACE_RING_DESCRIPTOR(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_io_surface_ring_descriptor_set_logical_size(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_IO_SURFACE_RING_DESCRIPTOR(self));
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), io_surface_ring_descriptor_properties
                        [IO_SURFACE_RING_DESCRIPTOR_PROP_LOGICAL_SIZE]
    );
}

guint32 maplibre_native_ffi_io_surface_ring_descriptor_get_buffer_count(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_IO_SURFACE_RING_DESCRIPTOR(self), 0
  );
  return self->buffer_count;
}

void maplibre_native_ffi_io_surface_ring_descriptor_set_buffer_count(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self, guint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_IO_SURFACE_RING_DESCRIPTOR(self));
  gboolean changed = self->buffer_count != value;
  self->buffer_count = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), io_surface_ring_descriptor_properties
                        [IO_SURFACE_RING_DESCRIPTOR_PROP_BUFFER_COUNT]
    );
}

MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_io_surface_ring_descriptor_get_context(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_IO_SURFACE_RING_DESCRIPTOR(self), NULL
  );
  return self->context;
}

void maplibre_native_ffi_io_surface_ring_descriptor_set_context(
  MaplibreNativeFfiIoSurfaceRingDescriptor* self,
  MaplibreNativeFfiMetalContextDescriptor* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_IO_SURFACE_RING_DESCRIPTOR(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_METAL_CONTEXT_DESCRIPTOR(value));
  MaplibreNativeFfiMetalContextDescriptor* previous = self->context;
  self->context = maplibre_native_ffi_metal_context_descriptor_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), io_surface_ring_descriptor_properties
                      [IO_SURFACE_RING_DESCRIPTOR_PROP_CONTEXT]
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

struct _MaplibreNativeFfiLogHandler {
  GObject parent_instance;
  MaplibreNativeFfiLogSeverityFlag platform_severities;
  MaplibreNativeFfiClosure* callback;
};

enum {
  LOG_HANDLER_PROP_0,
  LOG_HANDLER_PROP_PLATFORM_SEVERITIES,
  LOG_HANDLER_N_PROPERTIES
};

static GParamSpec* log_handler_properties[LOG_HANDLER_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiLogHandler, maplibre_native_ffi_log_handler, G_TYPE_OBJECT
)

static void maplibre_native_ffi_log_handler_finalize(GObject* object) {
  MaplibreNativeFfiLogHandler* self = MAPLIBRE_NATIVE_FFI_LOG_HANDLER(object);
  maplibre_native_ffi_closure_unref(self->callback);
  G_OBJECT_CLASS(maplibre_native_ffi_log_handler_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_log_handler_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiLogHandler* self = MAPLIBRE_NATIVE_FFI_LOG_HANDLER(object);
  switch (id) {
    case LOG_HANDLER_PROP_PLATFORM_SEVERITIES:
      g_value_set_flags(value, self->platform_severities);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_log_handler_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiLogHandler* self = MAPLIBRE_NATIVE_FFI_LOG_HANDLER(object);
  switch (id) {
    case LOG_HANDLER_PROP_PLATFORM_SEVERITIES:
      maplibre_native_ffi_log_handler_set_platform_severities(
        self, g_value_get_flags(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_log_handler_class_init(
  MaplibreNativeFfiLogHandlerClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_log_handler_finalize;
  object_class->get_property = maplibre_native_ffi_log_handler_get_property;
  object_class->set_property = maplibre_native_ffi_log_handler_set_property;
  log_handler_properties[LOG_HANDLER_PROP_PLATFORM_SEVERITIES] =
    g_param_spec_flags(
      "platform-severities", NULL, NULL,
      MAPLIBRE_NATIVE_FFI_TYPE_LOG_SEVERITY_FLAG, 0, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, LOG_HANDLER_N_PROPERTIES, log_handler_properties
  );
}

static void maplibre_native_ffi_log_handler_init(
  MaplibreNativeFfiLogHandler* self
) {
  (void)self;
}

MaplibreNativeFfiLogHandler* maplibre_native_ffi_log_handler_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_LOG_HANDLER, NULL);
}

MaplibreNativeFfiLogSeverityFlag
maplibre_native_ffi_log_handler_get_platform_severities(
  MaplibreNativeFfiLogHandler* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_LOG_HANDLER(self),
    (MaplibreNativeFfiLogSeverityFlag)0
  );
  return self->platform_severities;
}

void maplibre_native_ffi_log_handler_set_platform_severities(
  MaplibreNativeFfiLogHandler* self, MaplibreNativeFfiLogSeverityFlag value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_LOG_HANDLER(self));
  gboolean changed = self->platform_severities != value;
  self->platform_severities = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      log_handler_properties[LOG_HANDLER_PROP_PLATFORM_SEVERITIES]
    );
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiLogicalExtent) == sizeof(mln_logical_extent)
);
SAME_MEMBER(MaplibreNativeFfiLogicalExtent, mln_logical_extent, width);
SAME_MEMBER(MaplibreNativeFfiLogicalExtent, mln_logical_extent, height);
SAME_MEMBER(MaplibreNativeFfiLogicalExtent, mln_logical_extent, scale_factor);

void maplibre_native_ffi_logical_extent_to_native(
  const MaplibreNativeFfiLogicalExtent* self, mln_logical_extent* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_logical_extent_read(
  MaplibreNativeFfiLogicalExtent* out, const mln_logical_extent* in
) {
  memcpy(out, in, sizeof *out);
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
  GObject parent_instance;
  guint64 fields;
  MaplibreNativeFfiLogicalExtent initial_extent;
  MaplibreNativeFfiMapMode map_mode;
  gboolean fast_pfor_enabled;
  MaplibreNativeFfiRuntimeEventFlag event_mask;
  gboolean tile_cache_enabled;
  char* fallback_font_faces;
};

enum {
  MAP_OPTIONS_PROP_0,
  MAP_OPTIONS_PROP_INITIAL_EXTENT,
  MAP_OPTIONS_PROP_MAP_MODE,
  MAP_OPTIONS_PROP_FAST_PFOR_ENABLED,
  MAP_OPTIONS_PROP_EVENT_MASK,
  MAP_OPTIONS_PROP_TILE_CACHE_ENABLED,
  MAP_OPTIONS_PROP_FALLBACK_FONT_FACES,
  MAP_OPTIONS_N_PROPERTIES
};

static GParamSpec* map_options_properties[MAP_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiMapOptions, maplibre_native_ffi_map_options, G_TYPE_OBJECT
)

static void maplibre_native_ffi_map_options_finalize(GObject* object) {
  MaplibreNativeFfiMapOptions* self = MAPLIBRE_NATIVE_FFI_MAP_OPTIONS(object);
  g_free(self->fallback_font_faces);
  G_OBJECT_CLASS(maplibre_native_ffi_map_options_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_map_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMapOptions* self = MAPLIBRE_NATIVE_FFI_MAP_OPTIONS(object);
  switch (id) {
    case MAP_OPTIONS_PROP_INITIAL_EXTENT:
      g_value_set_boxed(value, &self->initial_extent);
      break;
    case MAP_OPTIONS_PROP_MAP_MODE:
      g_value_set_enum(value, self->map_mode);
      break;
    case MAP_OPTIONS_PROP_FAST_PFOR_ENABLED:
      g_value_set_boolean(value, self->fast_pfor_enabled);
      break;
    case MAP_OPTIONS_PROP_EVENT_MASK:
      g_value_set_flags(value, self->event_mask);
      break;
    case MAP_OPTIONS_PROP_TILE_CACHE_ENABLED:
      g_value_set_boolean(value, self->tile_cache_enabled);
      break;
    case MAP_OPTIONS_PROP_FALLBACK_FONT_FACES:
      g_value_set_string(value, self->fallback_font_faces);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_map_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMapOptions* self = MAPLIBRE_NATIVE_FFI_MAP_OPTIONS(object);
  switch (id) {
    case MAP_OPTIONS_PROP_INITIAL_EXTENT:
      maplibre_native_ffi_map_options_set_initial_extent(
        self, g_value_get_boxed(value)
      );
      break;
    case MAP_OPTIONS_PROP_MAP_MODE:
      maplibre_native_ffi_map_options_set_map_mode(
        self, g_value_get_enum(value)
      );
      break;
    case MAP_OPTIONS_PROP_FAST_PFOR_ENABLED:
      maplibre_native_ffi_map_options_set_fast_pfor_enabled(
        self, g_value_get_boolean(value)
      );
      break;
    case MAP_OPTIONS_PROP_EVENT_MASK:
      maplibre_native_ffi_map_options_set_event_mask(
        self, g_value_get_flags(value)
      );
      break;
    case MAP_OPTIONS_PROP_TILE_CACHE_ENABLED:
      maplibre_native_ffi_map_options_set_tile_cache_enabled(
        self, g_value_get_boolean(value)
      );
      break;
    case MAP_OPTIONS_PROP_FALLBACK_FONT_FACES:
      maplibre_native_ffi_map_options_set_fallback_font_faces(
        self, g_value_get_string(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_map_options_class_init(
  MaplibreNativeFfiMapOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_map_options_finalize;
  object_class->get_property = maplibre_native_ffi_map_options_get_property;
  object_class->set_property = maplibre_native_ffi_map_options_set_property;
  map_options_properties[MAP_OPTIONS_PROP_INITIAL_EXTENT] = g_param_spec_boxed(
    "initial-extent", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_EXTENT,
    PROPERTY_FLAGS
  );
  map_options_properties[MAP_OPTIONS_PROP_MAP_MODE] = g_param_spec_enum(
    "map-mode", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_MAP_MODE, 0, PROPERTY_FLAGS
  );
  map_options_properties[MAP_OPTIONS_PROP_FAST_PFOR_ENABLED] =
    g_param_spec_boolean(
      "fast-pfor-enabled", NULL, NULL, FALSE, PROPERTY_FLAGS
    );
  map_options_properties[MAP_OPTIONS_PROP_EVENT_MASK] = g_param_spec_flags(
    "event-mask", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_FLAG, 0,
    PROPERTY_FLAGS
  );
  map_options_properties[MAP_OPTIONS_PROP_TILE_CACHE_ENABLED] =
    g_param_spec_boolean(
      "tile-cache-enabled", NULL, NULL, FALSE, PROPERTY_FLAGS
    );
  map_options_properties[MAP_OPTIONS_PROP_FALLBACK_FONT_FACES] =
    g_param_spec_string("fallback-font-faces", NULL, NULL, "", PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, MAP_OPTIONS_N_PROPERTIES, map_options_properties
  );
}

static void maplibre_native_ffi_map_options_init(
  MaplibreNativeFfiMapOptions* self
) {
  self->fallback_font_faces = g_strdup("");
}

MaplibreNativeFfiMapOptions* maplibre_native_ffi_map_options_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_MAP_OPTIONS, NULL);
}

MaplibreNativeFfiLogicalExtent*
maplibre_native_ffi_map_options_get_initial_extent(
  MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self), NULL);
  return (MaplibreNativeFfiLogicalExtent*)&self->initial_extent;
}

void maplibre_native_ffi_map_options_set_initial_extent(
  MaplibreNativeFfiMapOptions* self, const MaplibreNativeFfiLogicalExtent* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self));
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->initial_extent, value, sizeof self->initial_extent) != 0;
  self->initial_extent = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), map_options_properties[MAP_OPTIONS_PROP_INITIAL_EXTENT]
    );
}

MaplibreNativeFfiMapMode maplibre_native_ffi_map_options_get_map_mode(
  MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self), (MaplibreNativeFfiMapMode)0
  );
  return self->map_mode;
}

void maplibre_native_ffi_map_options_set_map_mode(
  MaplibreNativeFfiMapOptions* self, MaplibreNativeFfiMapMode value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self));
  gboolean changed = self->map_mode != value;
  self->map_mode = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), map_options_properties[MAP_OPTIONS_PROP_MAP_MODE]
    );
}

gboolean maplibre_native_ffi_map_options_get_fast_pfor_enabled(
  MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self), FALSE);
  return self->fast_pfor_enabled;
}

void maplibre_native_ffi_map_options_set_fast_pfor_enabled(
  MaplibreNativeFfiMapOptions* self, gboolean value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self));
  gboolean changed = self->fast_pfor_enabled != value;
  self->fast_pfor_enabled = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), map_options_properties[MAP_OPTIONS_PROP_FAST_PFOR_ENABLED]
    );
}

MaplibreNativeFfiRuntimeEventFlag
maplibre_native_ffi_map_options_get_event_mask(
  MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self),
    (MaplibreNativeFfiRuntimeEventFlag)0
  );
  return self->event_mask;
}

gboolean maplibre_native_ffi_map_options_has_event_mask(
  MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self), FALSE);
  return (self->fields & MLN_MAP_OPTION_EVENT_MASK) != 0;
}

void maplibre_native_ffi_map_options_set_event_mask(
  MaplibreNativeFfiMapOptions* self, MaplibreNativeFfiRuntimeEventFlag value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_MAP_OPTION_EVENT_MASK) != 0;
  gboolean changed = self->event_mask != value;
  self->event_mask = value;
  self->fields |= MLN_MAP_OPTION_EVENT_MASK;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), map_options_properties[MAP_OPTIONS_PROP_EVENT_MASK]
    );
}

gboolean maplibre_native_ffi_map_options_get_tile_cache_enabled(
  MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self), FALSE);
  return self->tile_cache_enabled;
}

gboolean maplibre_native_ffi_map_options_has_tile_cache_enabled(
  MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self), FALSE);
  return (self->fields & MLN_MAP_OPTION_TILE_CACHE_ENABLED) != 0;
}

void maplibre_native_ffi_map_options_set_tile_cache_enabled(
  MaplibreNativeFfiMapOptions* self, gboolean value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_MAP_OPTION_TILE_CACHE_ENABLED) != 0;
  gboolean changed = self->tile_cache_enabled != value;
  self->tile_cache_enabled = value;
  self->fields |= MLN_MAP_OPTION_TILE_CACHE_ENABLED;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      map_options_properties[MAP_OPTIONS_PROP_TILE_CACHE_ENABLED]
    );
}

const char* maplibre_native_ffi_map_options_get_fallback_font_faces(
  MaplibreNativeFfiMapOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self), NULL);
  return self->fallback_font_faces;
}

void maplibre_native_ffi_map_options_set_fallback_font_faces(
  MaplibreNativeFfiMapOptions* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(self));
  char* previous = self->fallback_font_faces;
  self->fallback_font_faces = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->fallback_font_faces) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      map_options_properties[MAP_OPTIONS_PROP_FALLBACK_FONT_FACES]
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
  out->fallback_font_faces =
    maplibre_native_ffi_text_view(self->fallback_font_faces);
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiMapTileSettings) == sizeof(mln_map_tile_settings)
);
SAME_MEMBER(
  MaplibreNativeFfiMapTileSettings, mln_map_tile_settings, prefetch_zoom_delta
);
SAME_MEMBER(
  MaplibreNativeFfiMapTileSettings, mln_map_tile_settings, lod_min_radius
);
SAME_MEMBER(MaplibreNativeFfiMapTileSettings, mln_map_tile_settings, lod_scale);
SAME_MEMBER(
  MaplibreNativeFfiMapTileSettings, mln_map_tile_settings, lod_pitch_threshold
);
SAME_MEMBER(
  MaplibreNativeFfiMapTileSettings, mln_map_tile_settings, lod_zoom_shift
);
SAME_MEMBER(MaplibreNativeFfiMapTileSettings, mln_map_tile_settings, lod_mode);

void maplibre_native_ffi_map_tile_settings_read(
  MaplibreNativeFfiMapTileSettings* out, const mln_map_tile_settings* in
) {
  memcpy(out, in, sizeof *out);
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiMapViewport) == sizeof(mln_map_viewport)
);
SAME_MEMBER(MaplibreNativeFfiMapViewport, mln_map_viewport, north_orientation);
SAME_MEMBER(MaplibreNativeFfiMapViewport, mln_map_viewport, constrain_mode);
SAME_MEMBER(MaplibreNativeFfiMapViewport, mln_map_viewport, viewport_mode);
SAME_MEMBER(MaplibreNativeFfiMapViewport, mln_map_viewport, frustum_offset);

void maplibre_native_ffi_map_viewport_to_native(
  const MaplibreNativeFfiMapViewport* self, mln_map_viewport* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_map_viewport_read(
  MaplibreNativeFfiMapViewport* out, const mln_map_viewport* in
) {
  memcpy(out, in, sizeof *out);
}

MaplibreNativeFfiMapViewport* maplibre_native_ffi_map_viewport_from_native(
  const mln_map_viewport* in
) {
  MaplibreNativeFfiMapViewport* self = g_new0(MaplibreNativeFfiMapViewport, 1);
  maplibre_native_ffi_map_viewport_read(self, in);
  return self;
}

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiProjectionDefinition,
  maplibre_native_ffi_projection_definition,
  maplibre_native_ffi_projection_definition_copy,
  maplibre_native_ffi_projection_definition_free
)

MaplibreNativeFfiProjectionDefinition*
maplibre_native_ffi_projection_definition_copy(
  const MaplibreNativeFfiProjectionDefinition* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_projection_definition_free(
  MaplibreNativeFfiProjectionDefinition* self
) {
  g_free(self);
}

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiProjectionDefinition) ==
  sizeof(mln_projection_definition)
);
SAME_MEMBER(
  MaplibreNativeFfiProjectionDefinition, mln_projection_definition, from
);
SAME_MEMBER(
  MaplibreNativeFfiProjectionDefinition, mln_projection_definition, to
);
SAME_MEMBER(
  MaplibreNativeFfiProjectionDefinition, mln_projection_definition, transition
);

void maplibre_native_ffi_projection_definition_to_native(
  const MaplibreNativeFfiProjectionDefinition* self,
  mln_projection_definition* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_projection_definition_read(
  MaplibreNativeFfiProjectionDefinition* out,
  const mln_projection_definition* in
) {
  memcpy(out, in, sizeof *out);
}

MaplibreNativeFfiProjectionDefinition*
maplibre_native_ffi_projection_definition_from_native(
  const mln_projection_definition* in
) {
  MaplibreNativeFfiProjectionDefinition* self =
    g_new0(MaplibreNativeFfiProjectionDefinition, 1);
  maplibre_native_ffi_projection_definition_read(self, in);
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
  maplibre_native_ffi_projection_definition_to_native(
    &self->projection, &out->projection, call
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
  maplibre_native_ffi_projection_definition_read(
    &out->projection, &in->projection
  );
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
  out->globe_rendering = (in->globe_rendering) ? TRUE : FALSE;
  out->globe_free_bearing_enabled =
    (in->globe_free_bearing_enabled) ? TRUE : FALSE;
}

MaplibreNativeFfiMapSnapshot* maplibre_native_ffi_map_snapshot_from_native(
  const mln_map_snapshot* in
) {
  MaplibreNativeFfiMapSnapshot* self = g_new0(MaplibreNativeFfiMapSnapshot, 1);
  maplibre_native_ffi_map_snapshot_read(self, in);
  return self;
}

struct _MaplibreNativeFfiMapTileOptions {
  GObject parent_instance;
  guint64 fields;
  guint32 prefetch_zoom_delta;
  gdouble lod_min_radius;
  gdouble lod_scale;
  gdouble lod_pitch_threshold;
  gdouble lod_zoom_shift;
  MaplibreNativeFfiTileLodMode lod_mode;
};

enum {
  MAP_TILE_OPTIONS_PROP_0,
  MAP_TILE_OPTIONS_PROP_PREFETCH_ZOOM_DELTA,
  MAP_TILE_OPTIONS_PROP_LOD_MIN_RADIUS,
  MAP_TILE_OPTIONS_PROP_LOD_SCALE,
  MAP_TILE_OPTIONS_PROP_LOD_PITCH_THRESHOLD,
  MAP_TILE_OPTIONS_PROP_LOD_ZOOM_SHIFT,
  MAP_TILE_OPTIONS_PROP_LOD_MODE,
  MAP_TILE_OPTIONS_N_PROPERTIES
};

static GParamSpec* map_tile_options_properties[MAP_TILE_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiMapTileOptions, maplibre_native_ffi_map_tile_options,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_map_tile_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMapTileOptions* self =
    MAPLIBRE_NATIVE_FFI_MAP_TILE_OPTIONS(object);
  switch (id) {
    case MAP_TILE_OPTIONS_PROP_PREFETCH_ZOOM_DELTA:
      g_value_set_uint(value, self->prefetch_zoom_delta);
      break;
    case MAP_TILE_OPTIONS_PROP_LOD_MIN_RADIUS:
      g_value_set_double(value, self->lod_min_radius);
      break;
    case MAP_TILE_OPTIONS_PROP_LOD_SCALE:
      g_value_set_double(value, self->lod_scale);
      break;
    case MAP_TILE_OPTIONS_PROP_LOD_PITCH_THRESHOLD:
      g_value_set_double(value, self->lod_pitch_threshold);
      break;
    case MAP_TILE_OPTIONS_PROP_LOD_ZOOM_SHIFT:
      g_value_set_double(value, self->lod_zoom_shift);
      break;
    case MAP_TILE_OPTIONS_PROP_LOD_MODE:
      g_value_set_enum(value, self->lod_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_map_tile_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMapTileOptions* self =
    MAPLIBRE_NATIVE_FFI_MAP_TILE_OPTIONS(object);
  switch (id) {
    case MAP_TILE_OPTIONS_PROP_PREFETCH_ZOOM_DELTA:
      maplibre_native_ffi_map_tile_options_set_prefetch_zoom_delta(
        self, g_value_get_uint(value)
      );
      break;
    case MAP_TILE_OPTIONS_PROP_LOD_MIN_RADIUS:
      maplibre_native_ffi_map_tile_options_set_lod_min_radius(
        self, g_value_get_double(value)
      );
      break;
    case MAP_TILE_OPTIONS_PROP_LOD_SCALE:
      maplibre_native_ffi_map_tile_options_set_lod_scale(
        self, g_value_get_double(value)
      );
      break;
    case MAP_TILE_OPTIONS_PROP_LOD_PITCH_THRESHOLD:
      maplibre_native_ffi_map_tile_options_set_lod_pitch_threshold(
        self, g_value_get_double(value)
      );
      break;
    case MAP_TILE_OPTIONS_PROP_LOD_ZOOM_SHIFT:
      maplibre_native_ffi_map_tile_options_set_lod_zoom_shift(
        self, g_value_get_double(value)
      );
      break;
    case MAP_TILE_OPTIONS_PROP_LOD_MODE:
      maplibre_native_ffi_map_tile_options_set_lod_mode(
        self, g_value_get_enum(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_map_tile_options_class_init(
  MaplibreNativeFfiMapTileOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_map_tile_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_map_tile_options_set_property;
  map_tile_options_properties[MAP_TILE_OPTIONS_PROP_PREFETCH_ZOOM_DELTA] =
    g_param_spec_uint(
      "prefetch-zoom-delta", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
    );
  map_tile_options_properties[MAP_TILE_OPTIONS_PROP_LOD_MIN_RADIUS] =
    g_param_spec_double(
      "lod-min-radius", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  map_tile_options_properties[MAP_TILE_OPTIONS_PROP_LOD_SCALE] =
    g_param_spec_double(
      "lod-scale", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  map_tile_options_properties[MAP_TILE_OPTIONS_PROP_LOD_PITCH_THRESHOLD] =
    g_param_spec_double(
      "lod-pitch-threshold", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  map_tile_options_properties[MAP_TILE_OPTIONS_PROP_LOD_ZOOM_SHIFT] =
    g_param_spec_double(
      "lod-zoom-shift", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  map_tile_options_properties[MAP_TILE_OPTIONS_PROP_LOD_MODE] =
    g_param_spec_enum(
      "lod-mode", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_TILE_LOD_MODE, 0,
      PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, MAP_TILE_OPTIONS_N_PROPERTIES, map_tile_options_properties
  );
}

static void maplibre_native_ffi_map_tile_options_init(
  MaplibreNativeFfiMapTileOptions* self
) {
  (void)self;
}

MaplibreNativeFfiMapTileOptions* maplibre_native_ffi_map_tile_options_new(
  void
) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_MAP_TILE_OPTIONS, NULL);
}

guint32 maplibre_native_ffi_map_tile_options_get_prefetch_zoom_delta(
  MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self), 0);
  return self->prefetch_zoom_delta;
}

gboolean maplibre_native_ffi_map_tile_options_has_prefetch_zoom_delta(
  MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self), FALSE);
  return (self->fields & MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0;
}

void maplibre_native_ffi_map_tile_options_set_prefetch_zoom_delta(
  MaplibreNativeFfiMapTileOptions* self, guint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0;
  gboolean changed = self->prefetch_zoom_delta != value;
  self->prefetch_zoom_delta = value;
  self->fields |= MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      map_tile_options_properties[MAP_TILE_OPTIONS_PROP_PREFETCH_ZOOM_DELTA]
    );
}

gdouble maplibre_native_ffi_map_tile_options_get_lod_min_radius(
  MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self), 0);
  return self->lod_min_radius;
}

gboolean maplibre_native_ffi_map_tile_options_has_lod_min_radius(
  MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self), FALSE);
  return (self->fields & MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0;
}

void maplibre_native_ffi_map_tile_options_set_lod_min_radius(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0;
  gboolean changed = self->lod_min_radius != value;
  self->lod_min_radius = value;
  self->fields |= MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      map_tile_options_properties[MAP_TILE_OPTIONS_PROP_LOD_MIN_RADIUS]
    );
}

gdouble maplibre_native_ffi_map_tile_options_get_lod_scale(
  MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self), 0);
  return self->lod_scale;
}

gboolean maplibre_native_ffi_map_tile_options_has_lod_scale(
  MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self), FALSE);
  return (self->fields & MLN_MAP_TILE_OPTION_LOD_SCALE) != 0;
}

void maplibre_native_ffi_map_tile_options_set_lod_scale(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_MAP_TILE_OPTION_LOD_SCALE) != 0;
  gboolean changed = self->lod_scale != value;
  self->lod_scale = value;
  self->fields |= MLN_MAP_TILE_OPTION_LOD_SCALE;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      map_tile_options_properties[MAP_TILE_OPTIONS_PROP_LOD_SCALE]
    );
}

gdouble maplibre_native_ffi_map_tile_options_get_lod_pitch_threshold(
  MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self), 0);
  return self->lod_pitch_threshold;
}

gboolean maplibre_native_ffi_map_tile_options_has_lod_pitch_threshold(
  MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self), FALSE);
  return (self->fields & MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0;
}

void maplibre_native_ffi_map_tile_options_set_lod_pitch_threshold(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0;
  gboolean changed = self->lod_pitch_threshold != value;
  self->lod_pitch_threshold = value;
  self->fields |= MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      map_tile_options_properties[MAP_TILE_OPTIONS_PROP_LOD_PITCH_THRESHOLD]
    );
}

gdouble maplibre_native_ffi_map_tile_options_get_lod_zoom_shift(
  MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self), 0);
  return self->lod_zoom_shift;
}

gboolean maplibre_native_ffi_map_tile_options_has_lod_zoom_shift(
  MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self), FALSE);
  return (self->fields & MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0;
}

void maplibre_native_ffi_map_tile_options_set_lod_zoom_shift(
  MaplibreNativeFfiMapTileOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0;
  gboolean changed = self->lod_zoom_shift != value;
  self->lod_zoom_shift = value;
  self->fields |= MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      map_tile_options_properties[MAP_TILE_OPTIONS_PROP_LOD_ZOOM_SHIFT]
    );
}

MaplibreNativeFfiTileLodMode maplibre_native_ffi_map_tile_options_get_lod_mode(
  MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self),
    (MaplibreNativeFfiTileLodMode)0
  );
  return self->lod_mode;
}

gboolean maplibre_native_ffi_map_tile_options_has_lod_mode(
  MaplibreNativeFfiMapTileOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self), FALSE);
  return (self->fields & MLN_MAP_TILE_OPTION_LOD_MODE) != 0;
}

void maplibre_native_ffi_map_tile_options_set_lod_mode(
  MaplibreNativeFfiMapTileOptions* self, MaplibreNativeFfiTileLodMode value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_MAP_TILE_OPTION_LOD_MODE) != 0;
  gboolean changed = self->lod_mode != value;
  self->lod_mode = value;
  self->fields |= MLN_MAP_TILE_OPTION_LOD_MODE;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      map_tile_options_properties[MAP_TILE_OPTIONS_PROP_LOD_MODE]
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

struct _MaplibreNativeFfiMapViewportOptions {
  GObject parent_instance;
  guint64 fields;
  MaplibreNativeFfiNorthOrientation north_orientation;
  MaplibreNativeFfiConstrainMode constrain_mode;
  MaplibreNativeFfiViewportMode viewport_mode;
  MaplibreNativeFfiEdgeInsets frustum_offset;
};

enum {
  MAP_VIEWPORT_OPTIONS_PROP_0,
  MAP_VIEWPORT_OPTIONS_PROP_NORTH_ORIENTATION,
  MAP_VIEWPORT_OPTIONS_PROP_CONSTRAIN_MODE,
  MAP_VIEWPORT_OPTIONS_PROP_VIEWPORT_MODE,
  MAP_VIEWPORT_OPTIONS_PROP_FRUSTUM_OFFSET,
  MAP_VIEWPORT_OPTIONS_N_PROPERTIES
};

static GParamSpec*
  map_viewport_options_properties[MAP_VIEWPORT_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiMapViewportOptions, maplibre_native_ffi_map_viewport_options,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_map_viewport_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMapViewportOptions* self =
    MAPLIBRE_NATIVE_FFI_MAP_VIEWPORT_OPTIONS(object);
  switch (id) {
    case MAP_VIEWPORT_OPTIONS_PROP_NORTH_ORIENTATION:
      g_value_set_enum(value, self->north_orientation);
      break;
    case MAP_VIEWPORT_OPTIONS_PROP_CONSTRAIN_MODE:
      g_value_set_enum(value, self->constrain_mode);
      break;
    case MAP_VIEWPORT_OPTIONS_PROP_VIEWPORT_MODE:
      g_value_set_enum(value, self->viewport_mode);
      break;
    case MAP_VIEWPORT_OPTIONS_PROP_FRUSTUM_OFFSET:
      g_value_set_boxed(value, &self->frustum_offset);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_map_viewport_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMapViewportOptions* self =
    MAPLIBRE_NATIVE_FFI_MAP_VIEWPORT_OPTIONS(object);
  switch (id) {
    case MAP_VIEWPORT_OPTIONS_PROP_NORTH_ORIENTATION:
      maplibre_native_ffi_map_viewport_options_set_north_orientation(
        self, g_value_get_enum(value)
      );
      break;
    case MAP_VIEWPORT_OPTIONS_PROP_CONSTRAIN_MODE:
      maplibre_native_ffi_map_viewport_options_set_constrain_mode(
        self, g_value_get_enum(value)
      );
      break;
    case MAP_VIEWPORT_OPTIONS_PROP_VIEWPORT_MODE:
      maplibre_native_ffi_map_viewport_options_set_viewport_mode(
        self, g_value_get_enum(value)
      );
      break;
    case MAP_VIEWPORT_OPTIONS_PROP_FRUSTUM_OFFSET:
      maplibre_native_ffi_map_viewport_options_set_frustum_offset(
        self, g_value_get_boxed(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_map_viewport_options_class_init(
  MaplibreNativeFfiMapViewportOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_map_viewport_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_map_viewport_options_set_property;
  map_viewport_options_properties[MAP_VIEWPORT_OPTIONS_PROP_NORTH_ORIENTATION] =
    g_param_spec_enum(
      "north-orientation", NULL, NULL,
      MAPLIBRE_NATIVE_FFI_TYPE_NORTH_ORIENTATION, 0, PROPERTY_FLAGS
    );
  map_viewport_options_properties[MAP_VIEWPORT_OPTIONS_PROP_CONSTRAIN_MODE] =
    g_param_spec_enum(
      "constrain-mode", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_CONSTRAIN_MODE, 0,
      PROPERTY_FLAGS
    );
  map_viewport_options_properties[MAP_VIEWPORT_OPTIONS_PROP_VIEWPORT_MODE] =
    g_param_spec_enum(
      "viewport-mode", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_VIEWPORT_MODE, 0,
      PROPERTY_FLAGS
    );
  map_viewport_options_properties[MAP_VIEWPORT_OPTIONS_PROP_FRUSTUM_OFFSET] =
    g_param_spec_boxed(
      "frustum-offset", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_EDGE_INSETS,
      PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, MAP_VIEWPORT_OPTIONS_N_PROPERTIES,
    map_viewport_options_properties
  );
}

static void maplibre_native_ffi_map_viewport_options_init(
  MaplibreNativeFfiMapViewportOptions* self
) {
  (void)self;
}

MaplibreNativeFfiMapViewportOptions*
maplibre_native_ffi_map_viewport_options_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_MAP_VIEWPORT_OPTIONS, NULL);
}

MaplibreNativeFfiNorthOrientation
maplibre_native_ffi_map_viewport_options_get_north_orientation(
  MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_MAP_VIEWPORT_OPTIONS(self),
    (MaplibreNativeFfiNorthOrientation)0
  );
  return self->north_orientation;
}

gboolean maplibre_native_ffi_map_viewport_options_has_north_orientation(
  MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_MAP_VIEWPORT_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0;
}

void maplibre_native_ffi_map_viewport_options_set_north_orientation(
  MaplibreNativeFfiMapViewportOptions* self,
  MaplibreNativeFfiNorthOrientation value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_VIEWPORT_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0;
  gboolean changed = self->north_orientation != value;
  self->north_orientation = value;
  self->fields |= MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), map_viewport_options_properties
                        [MAP_VIEWPORT_OPTIONS_PROP_NORTH_ORIENTATION]
    );
}

MaplibreNativeFfiConstrainMode
maplibre_native_ffi_map_viewport_options_get_constrain_mode(
  MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_MAP_VIEWPORT_OPTIONS(self),
    (MaplibreNativeFfiConstrainMode)0
  );
  return self->constrain_mode;
}

gboolean maplibre_native_ffi_map_viewport_options_has_constrain_mode(
  MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_MAP_VIEWPORT_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0;
}

void maplibre_native_ffi_map_viewport_options_set_constrain_mode(
  MaplibreNativeFfiMapViewportOptions* self,
  MaplibreNativeFfiConstrainMode value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_VIEWPORT_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0;
  gboolean changed = self->constrain_mode != value;
  self->constrain_mode = value;
  self->fields |= MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      map_viewport_options_properties[MAP_VIEWPORT_OPTIONS_PROP_CONSTRAIN_MODE]
    );
}

MaplibreNativeFfiViewportMode
maplibre_native_ffi_map_viewport_options_get_viewport_mode(
  MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_MAP_VIEWPORT_OPTIONS(self),
    (MaplibreNativeFfiViewportMode)0
  );
  return self->viewport_mode;
}

gboolean maplibre_native_ffi_map_viewport_options_has_viewport_mode(
  MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_MAP_VIEWPORT_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0;
}

void maplibre_native_ffi_map_viewport_options_set_viewport_mode(
  MaplibreNativeFfiMapViewportOptions* self, MaplibreNativeFfiViewportMode value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_VIEWPORT_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0;
  gboolean changed = self->viewport_mode != value;
  self->viewport_mode = value;
  self->fields |= MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      map_viewport_options_properties[MAP_VIEWPORT_OPTIONS_PROP_VIEWPORT_MODE]
    );
}

MaplibreNativeFfiEdgeInsets*
maplibre_native_ffi_map_viewport_options_get_frustum_offset(
  MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_VIEWPORT_OPTIONS(self), NULL);
  return (MaplibreNativeFfiEdgeInsets*)&self->frustum_offset;
}

gboolean maplibre_native_ffi_map_viewport_options_has_frustum_offset(
  MaplibreNativeFfiMapViewportOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_MAP_VIEWPORT_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0;
}

void maplibre_native_ffi_map_viewport_options_set_frustum_offset(
  MaplibreNativeFfiMapViewportOptions* self,
  const MaplibreNativeFfiEdgeInsets* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_VIEWPORT_OPTIONS(self));
  g_return_if_fail(value != NULL);
  gboolean was_set =
    (self->fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0;
  gboolean changed =
    memcmp(&self->frustum_offset, value, sizeof self->frustum_offset) != 0;
  self->frustum_offset = *value;
  self->fields |= MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      map_viewport_options_properties[MAP_VIEWPORT_OPTIONS_PROP_FRUSTUM_OFFSET]
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

struct _MaplibreNativeFfiMetalBorrowedTexture {
  GObject parent_instance;
  gpointer texture;
};

enum {
  METAL_BORROWED_TEXTURE_PROP_0,
  METAL_BORROWED_TEXTURE_PROP_TEXTURE,
  METAL_BORROWED_TEXTURE_N_PROPERTIES
};

static GParamSpec*
  metal_borrowed_texture_properties[METAL_BORROWED_TEXTURE_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiMetalBorrowedTexture,
  maplibre_native_ffi_metal_borrowed_texture, G_TYPE_OBJECT
)

static void maplibre_native_ffi_metal_borrowed_texture_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMetalBorrowedTexture* self =
    MAPLIBRE_NATIVE_FFI_METAL_BORROWED_TEXTURE(object);
  switch (id) {
    case METAL_BORROWED_TEXTURE_PROP_TEXTURE:
      g_value_set_pointer(value, self->texture);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_metal_borrowed_texture_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMetalBorrowedTexture* self =
    MAPLIBRE_NATIVE_FFI_METAL_BORROWED_TEXTURE(object);
  switch (id) {
    case METAL_BORROWED_TEXTURE_PROP_TEXTURE:
      maplibre_native_ffi_metal_borrowed_texture_set_texture(
        self, g_value_get_pointer(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_metal_borrowed_texture_class_init(
  MaplibreNativeFfiMetalBorrowedTextureClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_metal_borrowed_texture_get_property;
  object_class->set_property =
    maplibre_native_ffi_metal_borrowed_texture_set_property;
  metal_borrowed_texture_properties[METAL_BORROWED_TEXTURE_PROP_TEXTURE] =
    g_param_spec_pointer("texture", NULL, NULL, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, METAL_BORROWED_TEXTURE_N_PROPERTIES,
    metal_borrowed_texture_properties
  );
}

static void maplibre_native_ffi_metal_borrowed_texture_init(
  MaplibreNativeFfiMetalBorrowedTexture* self
) {
  (void)self;
}

MaplibreNativeFfiMetalBorrowedTexture*
maplibre_native_ffi_metal_borrowed_texture_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_METAL_BORROWED_TEXTURE, NULL);
}

gpointer maplibre_native_ffi_metal_borrowed_texture_get_texture(
  MaplibreNativeFfiMetalBorrowedTexture* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE(self), 0);
  return self->texture;
}

void maplibre_native_ffi_metal_borrowed_texture_set_texture(
  MaplibreNativeFfiMetalBorrowedTexture* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE(self));
  gboolean changed = self->texture != value;
  self->texture = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      metal_borrowed_texture_properties[METAL_BORROWED_TEXTURE_PROP_TEXTURE]
    );
}

MaplibreNativeFfiMetalBorrowedTexture*
maplibre_native_ffi_metal_borrowed_texture_copy(
  const MaplibreNativeFfiMetalBorrowedTexture* self
) {
  MaplibreNativeFfiMetalBorrowedTexture* copy =
    maplibre_native_ffi_metal_borrowed_texture_new();
  copy->texture = self->texture;
  return copy;
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

struct _MaplibreNativeFfiMetalBorrowedTextureDescriptor {
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 physical_width;
  guint32 physical_height;
  GPtrArray* textures;
  gpointer command_queue;
};

enum {
  METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_0,
  METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE,
  METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH,
  METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT,
  METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_COMMAND_QUEUE,
  METAL_BORROWED_TEXTURE_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec* metal_borrowed_texture_descriptor_properties
  [METAL_BORROWED_TEXTURE_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor,
  maplibre_native_ffi_metal_borrowed_texture_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_metal_borrowed_texture_descriptor_finalize(
  GObject* object
) {
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self =
    MAPLIBRE_NATIVE_FFI_METAL_BORROWED_TEXTURE_DESCRIPTOR(object);
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  G_OBJECT_CLASS(
    maplibre_native_ffi_metal_borrowed_texture_descriptor_parent_class
  )
    ->finalize(object);
}

static void maplibre_native_ffi_metal_borrowed_texture_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self =
    MAPLIBRE_NATIVE_FFI_METAL_BORROWED_TEXTURE_DESCRIPTOR(object);
  switch (id) {
    case METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH:
      g_value_set_uint(value, self->physical_width);
      break;
    case METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT:
      g_value_set_uint(value, self->physical_height);
      break;
    case METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_COMMAND_QUEUE:
      g_value_set_pointer(value, self->command_queue);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self =
    MAPLIBRE_NATIVE_FFI_METAL_BORROWED_TEXTURE_DESCRIPTOR(object);
  switch (id) {
    case METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_metal_borrowed_texture_descriptor_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH:
      maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_width(
        self, g_value_get_uint(value)
      );
      break;
    case METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT:
      maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_height(
        self, g_value_get_uint(value)
      );
      break;
    case METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_COMMAND_QUEUE:
      maplibre_native_ffi_metal_borrowed_texture_descriptor_set_command_queue(
        self, g_value_get_pointer(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_metal_borrowed_texture_descriptor_class_init(
  MaplibreNativeFfiMetalBorrowedTextureDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_metal_borrowed_texture_descriptor_finalize;
  object_class->get_property =
    maplibre_native_ffi_metal_borrowed_texture_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_metal_borrowed_texture_descriptor_set_property;
  metal_borrowed_texture_descriptor_properties
    [METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE] = g_param_spec_boxed(
      "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
      PROPERTY_FLAGS
    );
  metal_borrowed_texture_descriptor_properties
    [METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH] = g_param_spec_uint(
      "physical-width", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
    );
  metal_borrowed_texture_descriptor_properties
    [METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT] =
      g_param_spec_uint(
        "physical-height", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
      );
  metal_borrowed_texture_descriptor_properties
    [METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_COMMAND_QUEUE] =
      g_param_spec_pointer("command-queue", NULL, NULL, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, METAL_BORROWED_TEXTURE_DESCRIPTOR_N_PROPERTIES,
    metal_borrowed_texture_descriptor_properties
  );
}

static void maplibre_native_ffi_metal_borrowed_texture_descriptor_init(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
) {
  self->textures =
    g_ptr_array_new_with_free_func((GDestroyNotify)g_object_unref);
}

MaplibreNativeFfiMetalBorrowedTextureDescriptor*
maplibre_native_ffi_metal_borrowed_texture_descriptor_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_METAL_BORROWED_TEXTURE_DESCRIPTOR, NULL
  );
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_logical_size(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_DESCRIPTOR(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_logical_size(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), metal_borrowed_texture_descriptor_properties
                        [METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE]
    );
}

guint32
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_physical_width(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_DESCRIPTOR(self), 0
  );
  return self->physical_width;
}

void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_width(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  gboolean changed = self->physical_width != value;
  self->physical_width = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), metal_borrowed_texture_descriptor_properties
                        [METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH]
    );
}

guint32
maplibre_native_ffi_metal_borrowed_texture_descriptor_get_physical_height(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_DESCRIPTOR(self), 0
  );
  return self->physical_height;
}

void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_physical_height(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  gboolean changed = self->physical_height != value;
  self->physical_height = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), metal_borrowed_texture_descriptor_properties
                        [METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT]
    );
}

GPtrArray* maplibre_native_ffi_metal_borrowed_texture_descriptor_get_textures(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_DESCRIPTOR(self), NULL
  );
  return self->textures;
}

void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_textures(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self,
  MaplibreNativeFfiMetalBorrowedTexture* const* value, gsize value_count
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  for (gsize index = 0; index < value_count; index++) {
    g_return_if_fail(
      MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE(value[index])
    );
  }
  GPtrArray* previous = self->textures;
  self->textures =
    g_ptr_array_new_full((guint)value_count, (GDestroyNotify)g_object_unref);
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
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_DESCRIPTOR(self), 0
  );
  return self->command_queue;
}

void maplibre_native_ffi_metal_borrowed_texture_descriptor_set_command_queue(
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* self, gpointer value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  gboolean changed = self->command_queue != value;
  self->command_queue = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), metal_borrowed_texture_descriptor_properties
                        [METAL_BORROWED_TEXTURE_DESCRIPTOR_PROP_COMMAND_QUEUE]
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

struct _MaplibreNativeFfiMetalBorrowedTextureReplacement {
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 physical_width;
  guint32 physical_height;
  GPtrArray* textures;
};

enum {
  METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_0,
  METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE,
  METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH,
  METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT,
  METAL_BORROWED_TEXTURE_REPLACEMENT_N_PROPERTIES
};

static GParamSpec* metal_borrowed_texture_replacement_properties
  [METAL_BORROWED_TEXTURE_REPLACEMENT_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiMetalBorrowedTextureReplacement,
  maplibre_native_ffi_metal_borrowed_texture_replacement, G_TYPE_OBJECT
)

static void maplibre_native_ffi_metal_borrowed_texture_replacement_finalize(
  GObject* object
) {
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self =
    MAPLIBRE_NATIVE_FFI_METAL_BORROWED_TEXTURE_REPLACEMENT(object);
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  G_OBJECT_CLASS(
    maplibre_native_ffi_metal_borrowed_texture_replacement_parent_class
  )
    ->finalize(object);
}

static void maplibre_native_ffi_metal_borrowed_texture_replacement_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self =
    MAPLIBRE_NATIVE_FFI_METAL_BORROWED_TEXTURE_REPLACEMENT(object);
  switch (id) {
    case METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH:
      g_value_set_uint(value, self->physical_width);
      break;
    case METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT:
      g_value_set_uint(value, self->physical_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_metal_borrowed_texture_replacement_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self =
    MAPLIBRE_NATIVE_FFI_METAL_BORROWED_TEXTURE_REPLACEMENT(object);
  switch (id) {
    case METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_metal_borrowed_texture_replacement_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH:
      maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_width(
        self, g_value_get_uint(value)
      );
      break;
    case METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT:
      maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_height(
        self, g_value_get_uint(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_metal_borrowed_texture_replacement_class_init(
  MaplibreNativeFfiMetalBorrowedTextureReplacementClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_metal_borrowed_texture_replacement_finalize;
  object_class->get_property =
    maplibre_native_ffi_metal_borrowed_texture_replacement_get_property;
  object_class->set_property =
    maplibre_native_ffi_metal_borrowed_texture_replacement_set_property;
  metal_borrowed_texture_replacement_properties
    [METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE] = g_param_spec_boxed(
      "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
      PROPERTY_FLAGS
    );
  metal_borrowed_texture_replacement_properties
    [METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH] =
      g_param_spec_uint(
        "physical-width", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
      );
  metal_borrowed_texture_replacement_properties
    [METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT] =
      g_param_spec_uint(
        "physical-height", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
      );
  g_object_class_install_properties(
    object_class, METAL_BORROWED_TEXTURE_REPLACEMENT_N_PROPERTIES,
    metal_borrowed_texture_replacement_properties
  );
}

static void maplibre_native_ffi_metal_borrowed_texture_replacement_init(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self
) {
  self->textures =
    g_ptr_array_new_with_free_func((GDestroyNotify)g_object_unref);
}

MaplibreNativeFfiMetalBorrowedTextureReplacement*
maplibre_native_ffi_metal_borrowed_texture_replacement_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_METAL_BORROWED_TEXTURE_REPLACEMENT, NULL
  );
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_borrowed_texture_replacement_get_logical_size(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_REPLACEMENT(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_metal_borrowed_texture_replacement_set_logical_size(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_REPLACEMENT(self)
  );
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), metal_borrowed_texture_replacement_properties
                        [METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE]
    );
}

guint32
maplibre_native_ffi_metal_borrowed_texture_replacement_get_physical_width(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_REPLACEMENT(self), 0
  );
  return self->physical_width;
}

void maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_width(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_REPLACEMENT(self)
  );
  gboolean changed = self->physical_width != value;
  self->physical_width = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), metal_borrowed_texture_replacement_properties
                        [METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH]
    );
}

guint32
maplibre_native_ffi_metal_borrowed_texture_replacement_get_physical_height(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_REPLACEMENT(self), 0
  );
  return self->physical_height;
}

void maplibre_native_ffi_metal_borrowed_texture_replacement_set_physical_height(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_REPLACEMENT(self)
  );
  gboolean changed = self->physical_height != value;
  self->physical_height = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      metal_borrowed_texture_replacement_properties
        [METAL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT]
    );
}

GPtrArray* maplibre_native_ffi_metal_borrowed_texture_replacement_get_textures(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_REPLACEMENT(self), NULL
  );
  return self->textures;
}

void maplibre_native_ffi_metal_borrowed_texture_replacement_set_textures(
  MaplibreNativeFfiMetalBorrowedTextureReplacement* self,
  MaplibreNativeFfiMetalBorrowedTexture* const* value, gsize value_count
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_REPLACEMENT(self)
  );
  for (gsize index = 0; index < value_count; index++) {
    g_return_if_fail(
      MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE(value[index])
    );
  }
  GPtrArray* previous = self->textures;
  self->textures =
    g_ptr_array_new_full((guint)value_count, (GDestroyNotify)g_object_unref);
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->textures,
      maplibre_native_ffi_metal_borrowed_texture_copy(value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
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

struct _MaplibreNativeFfiMetalSurfaceDescriptor {
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  MaplibreNativeFfiMetalContextDescriptor* context;
  gpointer layer;
};

enum {
  METAL_SURFACE_DESCRIPTOR_PROP_0,
  METAL_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE,
  METAL_SURFACE_DESCRIPTOR_PROP_CONTEXT,
  METAL_SURFACE_DESCRIPTOR_PROP_LAYER,
  METAL_SURFACE_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec*
  metal_surface_descriptor_properties[METAL_SURFACE_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiMetalSurfaceDescriptor,
  maplibre_native_ffi_metal_surface_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_metal_surface_descriptor_finalize(
  GObject* object
) {
  MaplibreNativeFfiMetalSurfaceDescriptor* self =
    MAPLIBRE_NATIVE_FFI_METAL_SURFACE_DESCRIPTOR(object);
  g_clear_object(&self->context);
  G_OBJECT_CLASS(maplibre_native_ffi_metal_surface_descriptor_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_metal_surface_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMetalSurfaceDescriptor* self =
    MAPLIBRE_NATIVE_FFI_METAL_SURFACE_DESCRIPTOR(object);
  switch (id) {
    case METAL_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case METAL_SURFACE_DESCRIPTOR_PROP_CONTEXT:
      g_value_set_object(value, self->context);
      break;
    case METAL_SURFACE_DESCRIPTOR_PROP_LAYER:
      g_value_set_pointer(value, self->layer);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_metal_surface_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMetalSurfaceDescriptor* self =
    MAPLIBRE_NATIVE_FFI_METAL_SURFACE_DESCRIPTOR(object);
  switch (id) {
    case METAL_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_metal_surface_descriptor_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case METAL_SURFACE_DESCRIPTOR_PROP_CONTEXT:
      maplibre_native_ffi_metal_surface_descriptor_set_context(
        self, g_value_get_object(value)
      );
      break;
    case METAL_SURFACE_DESCRIPTOR_PROP_LAYER:
      maplibre_native_ffi_metal_surface_descriptor_set_layer(
        self, g_value_get_pointer(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_metal_surface_descriptor_class_init(
  MaplibreNativeFfiMetalSurfaceDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_metal_surface_descriptor_finalize;
  object_class->get_property =
    maplibre_native_ffi_metal_surface_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_metal_surface_descriptor_set_property;
  metal_surface_descriptor_properties
    [METAL_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE] = g_param_spec_boxed(
      "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
      PROPERTY_FLAGS
    );
  metal_surface_descriptor_properties[METAL_SURFACE_DESCRIPTOR_PROP_CONTEXT] =
    g_param_spec_object(
      "context", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_METAL_CONTEXT_DESCRIPTOR,
      PROPERTY_FLAGS
    );
  metal_surface_descriptor_properties[METAL_SURFACE_DESCRIPTOR_PROP_LAYER] =
    g_param_spec_pointer("layer", NULL, NULL, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, METAL_SURFACE_DESCRIPTOR_N_PROPERTIES,
    metal_surface_descriptor_properties
  );
}

static void maplibre_native_ffi_metal_surface_descriptor_init(
  MaplibreNativeFfiMetalSurfaceDescriptor* self
) {
  self->context = maplibre_native_ffi_metal_context_descriptor_new();
}

MaplibreNativeFfiMetalSurfaceDescriptor*
maplibre_native_ffi_metal_surface_descriptor_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_METAL_SURFACE_DESCRIPTOR, NULL);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_surface_descriptor_get_logical_size(
  MaplibreNativeFfiMetalSurfaceDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_SURFACE_DESCRIPTOR(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_metal_surface_descriptor_set_logical_size(
  MaplibreNativeFfiMetalSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_METAL_SURFACE_DESCRIPTOR(self));
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), metal_surface_descriptor_properties
                        [METAL_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE]
    );
}

MaplibreNativeFfiMetalContextDescriptor*
maplibre_native_ffi_metal_surface_descriptor_get_context(
  MaplibreNativeFfiMetalSurfaceDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_SURFACE_DESCRIPTOR(self), NULL
  );
  return self->context;
}

void maplibre_native_ffi_metal_surface_descriptor_set_context(
  MaplibreNativeFfiMetalSurfaceDescriptor* self,
  MaplibreNativeFfiMetalContextDescriptor* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_METAL_SURFACE_DESCRIPTOR(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_METAL_CONTEXT_DESCRIPTOR(value));
  MaplibreNativeFfiMetalContextDescriptor* previous = self->context;
  self->context = maplibre_native_ffi_metal_context_descriptor_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self),
    metal_surface_descriptor_properties[METAL_SURFACE_DESCRIPTOR_PROP_CONTEXT]
  );
}

gpointer maplibre_native_ffi_metal_surface_descriptor_get_layer(
  MaplibreNativeFfiMetalSurfaceDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_SURFACE_DESCRIPTOR(self), 0
  );
  return self->layer;
}

void maplibre_native_ffi_metal_surface_descriptor_set_layer(
  MaplibreNativeFfiMetalSurfaceDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_METAL_SURFACE_DESCRIPTOR(self));
  gboolean changed = self->layer != value;
  self->layer = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      metal_surface_descriptor_properties[METAL_SURFACE_DESCRIPTOR_PROP_LAYER]
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

struct _MaplibreNativeFfiMetalSurfaceReplacement {
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  gpointer layer;
};

enum {
  METAL_SURFACE_REPLACEMENT_PROP_0,
  METAL_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE,
  METAL_SURFACE_REPLACEMENT_PROP_LAYER,
  METAL_SURFACE_REPLACEMENT_N_PROPERTIES
};

static GParamSpec*
  metal_surface_replacement_properties[METAL_SURFACE_REPLACEMENT_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiMetalSurfaceReplacement,
  maplibre_native_ffi_metal_surface_replacement, G_TYPE_OBJECT
)

static void maplibre_native_ffi_metal_surface_replacement_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMetalSurfaceReplacement* self =
    MAPLIBRE_NATIVE_FFI_METAL_SURFACE_REPLACEMENT(object);
  switch (id) {
    case METAL_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case METAL_SURFACE_REPLACEMENT_PROP_LAYER:
      g_value_set_pointer(value, self->layer);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_metal_surface_replacement_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiMetalSurfaceReplacement* self =
    MAPLIBRE_NATIVE_FFI_METAL_SURFACE_REPLACEMENT(object);
  switch (id) {
    case METAL_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_metal_surface_replacement_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case METAL_SURFACE_REPLACEMENT_PROP_LAYER:
      maplibre_native_ffi_metal_surface_replacement_set_layer(
        self, g_value_get_pointer(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_metal_surface_replacement_class_init(
  MaplibreNativeFfiMetalSurfaceReplacementClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_metal_surface_replacement_get_property;
  object_class->set_property =
    maplibre_native_ffi_metal_surface_replacement_set_property;
  metal_surface_replacement_properties
    [METAL_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE] = g_param_spec_boxed(
      "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
      PROPERTY_FLAGS
    );
  metal_surface_replacement_properties[METAL_SURFACE_REPLACEMENT_PROP_LAYER] =
    g_param_spec_pointer("layer", NULL, NULL, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, METAL_SURFACE_REPLACEMENT_N_PROPERTIES,
    metal_surface_replacement_properties
  );
}

static void maplibre_native_ffi_metal_surface_replacement_init(
  MaplibreNativeFfiMetalSurfaceReplacement* self
) {
  (void)self;
}

MaplibreNativeFfiMetalSurfaceReplacement*
maplibre_native_ffi_metal_surface_replacement_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_METAL_SURFACE_REPLACEMENT, NULL);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_metal_surface_replacement_get_logical_size(
  MaplibreNativeFfiMetalSurfaceReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_SURFACE_REPLACEMENT(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_metal_surface_replacement_set_logical_size(
  MaplibreNativeFfiMetalSurfaceReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_METAL_SURFACE_REPLACEMENT(self));
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), metal_surface_replacement_properties
                        [METAL_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE]
    );
}

gpointer maplibre_native_ffi_metal_surface_replacement_get_layer(
  MaplibreNativeFfiMetalSurfaceReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_SURFACE_REPLACEMENT(self), 0
  );
  return self->layer;
}

void maplibre_native_ffi_metal_surface_replacement_set_layer(
  MaplibreNativeFfiMetalSurfaceReplacement* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_METAL_SURFACE_REPLACEMENT(self));
  gboolean changed = self->layer != value;
  self->layer = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      metal_surface_replacement_properties[METAL_SURFACE_REPLACEMENT_PROP_LAYER]
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
  GObject parent_instance;
  char* style_url;
  char* geometry;
  gdouble min_zoom;
  gdouble max_zoom;
  gfloat pixel_ratio;
  gboolean include_ideographs;
};

enum {
  OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_0,
  OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_STYLE_URL,
  OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_GEOMETRY,
  OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_MIN_ZOOM,
  OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_MAX_ZOOM,
  OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_PIXEL_RATIO,
  OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_INCLUDE_IDEOGRAPHS,
  OFFLINE_GEOMETRY_REGION_DEFINITION_N_PROPERTIES
};

static GParamSpec* offline_geometry_region_definition_properties
  [OFFLINE_GEOMETRY_REGION_DEFINITION_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiOfflineGeometryRegionDefinition,
  maplibre_native_ffi_offline_geometry_region_definition, G_TYPE_OBJECT
)

static void maplibre_native_ffi_offline_geometry_region_definition_finalize(
  GObject* object
) {
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self =
    MAPLIBRE_NATIVE_FFI_OFFLINE_GEOMETRY_REGION_DEFINITION(object);
  g_free(self->style_url);
  g_free(self->geometry);
  G_OBJECT_CLASS(
    maplibre_native_ffi_offline_geometry_region_definition_parent_class
  )
    ->finalize(object);
}

static void maplibre_native_ffi_offline_geometry_region_definition_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self =
    MAPLIBRE_NATIVE_FFI_OFFLINE_GEOMETRY_REGION_DEFINITION(object);
  switch (id) {
    case OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_STYLE_URL:
      g_value_set_string(value, self->style_url);
      break;
    case OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_GEOMETRY:
      g_value_set_string(value, self->geometry);
      break;
    case OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_MIN_ZOOM:
      g_value_set_double(value, self->min_zoom);
      break;
    case OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_MAX_ZOOM:
      g_value_set_double(value, self->max_zoom);
      break;
    case OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_PIXEL_RATIO:
      g_value_set_float(value, self->pixel_ratio);
      break;
    case OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_INCLUDE_IDEOGRAPHS:
      g_value_set_boolean(value, self->include_ideographs);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_offline_geometry_region_definition_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self =
    MAPLIBRE_NATIVE_FFI_OFFLINE_GEOMETRY_REGION_DEFINITION(object);
  switch (id) {
    case OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_STYLE_URL:
      maplibre_native_ffi_offline_geometry_region_definition_set_style_url(
        self, g_value_get_string(value)
      );
      break;
    case OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_GEOMETRY:
      maplibre_native_ffi_offline_geometry_region_definition_set_geometry(
        self, g_value_get_string(value)
      );
      break;
    case OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_MIN_ZOOM:
      maplibre_native_ffi_offline_geometry_region_definition_set_min_zoom(
        self, g_value_get_double(value)
      );
      break;
    case OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_MAX_ZOOM:
      maplibre_native_ffi_offline_geometry_region_definition_set_max_zoom(
        self, g_value_get_double(value)
      );
      break;
    case OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_PIXEL_RATIO:
      maplibre_native_ffi_offline_geometry_region_definition_set_pixel_ratio(
        self, g_value_get_float(value)
      );
      break;
    case OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_INCLUDE_IDEOGRAPHS:
      maplibre_native_ffi_offline_geometry_region_definition_set_include_ideographs(
        self, g_value_get_boolean(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_offline_geometry_region_definition_class_init(
  MaplibreNativeFfiOfflineGeometryRegionDefinitionClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_offline_geometry_region_definition_finalize;
  object_class->get_property =
    maplibre_native_ffi_offline_geometry_region_definition_get_property;
  object_class->set_property =
    maplibre_native_ffi_offline_geometry_region_definition_set_property;
  offline_geometry_region_definition_properties
    [OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_STYLE_URL] =
      g_param_spec_string("style-url", NULL, NULL, "", PROPERTY_FLAGS);
  offline_geometry_region_definition_properties
    [OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_GEOMETRY] =
      g_param_spec_string("geometry", NULL, NULL, "", PROPERTY_FLAGS);
  offline_geometry_region_definition_properties
    [OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_MIN_ZOOM] = g_param_spec_double(
      "min-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  offline_geometry_region_definition_properties
    [OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_MAX_ZOOM] = g_param_spec_double(
      "max-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  offline_geometry_region_definition_properties
    [OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_PIXEL_RATIO] = g_param_spec_float(
      "pixel-ratio", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  offline_geometry_region_definition_properties
    [OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_INCLUDE_IDEOGRAPHS] =
      g_param_spec_boolean(
        "include-ideographs", NULL, NULL, FALSE, PROPERTY_FLAGS
      );
  g_object_class_install_properties(
    object_class, OFFLINE_GEOMETRY_REGION_DEFINITION_N_PROPERTIES,
    offline_geometry_region_definition_properties
  );
}

static void maplibre_native_ffi_offline_geometry_region_definition_init(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  self->style_url = g_strdup("");
  self->geometry = g_strdup("");
}

MaplibreNativeFfiOfflineGeometryRegionDefinition*
maplibre_native_ffi_offline_geometry_region_definition_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_OFFLINE_GEOMETRY_REGION_DEFINITION, NULL
  );
}

const char*
maplibre_native_ffi_offline_geometry_region_definition_get_style_url(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_GEOMETRY_REGION_DEFINITION(self), NULL
  );
  return self->style_url;
}

void maplibre_native_ffi_offline_geometry_region_definition_set_style_url(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, const char* value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_GEOMETRY_REGION_DEFINITION(self)
  );
  char* previous = self->style_url;
  self->style_url = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->style_url) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), offline_geometry_region_definition_properties
                        [OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_STYLE_URL]
    );
}

const char* maplibre_native_ffi_offline_geometry_region_definition_get_geometry(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_GEOMETRY_REGION_DEFINITION(self), NULL
  );
  return self->geometry;
}

void maplibre_native_ffi_offline_geometry_region_definition_set_geometry(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, const char* value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_GEOMETRY_REGION_DEFINITION(self)
  );
  char* previous = self->geometry;
  self->geometry = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->geometry) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), offline_geometry_region_definition_properties
                        [OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_GEOMETRY]
    );
}

gdouble maplibre_native_ffi_offline_geometry_region_definition_get_min_zoom(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_GEOMETRY_REGION_DEFINITION(self), 0
  );
  return self->min_zoom;
}

void maplibre_native_ffi_offline_geometry_region_definition_set_min_zoom(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gdouble value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_GEOMETRY_REGION_DEFINITION(self)
  );
  gboolean changed = self->min_zoom != value;
  self->min_zoom = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), offline_geometry_region_definition_properties
                        [OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_MIN_ZOOM]
    );
}

gdouble maplibre_native_ffi_offline_geometry_region_definition_get_max_zoom(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_GEOMETRY_REGION_DEFINITION(self), 0
  );
  return self->max_zoom;
}

void maplibre_native_ffi_offline_geometry_region_definition_set_max_zoom(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gdouble value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_GEOMETRY_REGION_DEFINITION(self)
  );
  gboolean changed = self->max_zoom != value;
  self->max_zoom = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), offline_geometry_region_definition_properties
                        [OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_MAX_ZOOM]
    );
}

gfloat maplibre_native_ffi_offline_geometry_region_definition_get_pixel_ratio(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_GEOMETRY_REGION_DEFINITION(self), 0
  );
  return self->pixel_ratio;
}

void maplibre_native_ffi_offline_geometry_region_definition_set_pixel_ratio(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gfloat value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_GEOMETRY_REGION_DEFINITION(self)
  );
  gboolean changed = self->pixel_ratio != value;
  self->pixel_ratio = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), offline_geometry_region_definition_properties
                        [OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_PIXEL_RATIO]
    );
}

gboolean
maplibre_native_ffi_offline_geometry_region_definition_get_include_ideographs(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_GEOMETRY_REGION_DEFINITION(self), FALSE
  );
  return self->include_ideographs;
}

void maplibre_native_ffi_offline_geometry_region_definition_set_include_ideographs(
  MaplibreNativeFfiOfflineGeometryRegionDefinition* self, gboolean value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_GEOMETRY_REGION_DEFINITION(self)
  );
  gboolean changed = self->include_ideographs != value;
  self->include_ideographs = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      offline_geometry_region_definition_properties
        [OFFLINE_GEOMETRY_REGION_DEFINITION_PROP_INCLUDE_IDEOGRAPHS]
    );
}

MaplibreNativeFfiOfflineGeometryRegionDefinition*
maplibre_native_ffi_offline_geometry_region_definition_copy(
  const MaplibreNativeFfiOfflineGeometryRegionDefinition* self
) {
  MaplibreNativeFfiOfflineGeometryRegionDefinition* copy =
    maplibre_native_ffi_offline_geometry_region_definition_new();
  g_free(copy->style_url);
  copy->style_url = g_strdup(self->style_url);
  g_free(copy->geometry);
  copy->geometry = g_strdup(self->geometry);
  copy->min_zoom = self->min_zoom;
  copy->max_zoom = self->max_zoom;
  copy->pixel_ratio = self->pixel_ratio;
  copy->include_ideographs = self->include_ideographs;
  return copy;
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
  GObject parent_instance;
  char* style_url;
  MaplibreNativeFfiLatLngBounds bounds;
  gdouble min_zoom;
  gdouble max_zoom;
  gfloat pixel_ratio;
  gboolean include_ideographs;
};

enum {
  OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_0,
  OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_STYLE_URL,
  OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_BOUNDS,
  OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_MIN_ZOOM,
  OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_MAX_ZOOM,
  OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_PIXEL_RATIO,
  OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_INCLUDE_IDEOGRAPHS,
  OFFLINE_TILE_PYRAMID_REGION_DEFINITION_N_PROPERTIES
};

static GParamSpec* offline_tile_pyramid_region_definition_properties
  [OFFLINE_TILE_PYRAMID_REGION_DEFINITION_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition,
  maplibre_native_ffi_offline_tile_pyramid_region_definition, G_TYPE_OBJECT
)

static void maplibre_native_ffi_offline_tile_pyramid_region_definition_finalize(
  GObject* object
) {
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self =
    MAPLIBRE_NATIVE_FFI_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(object);
  g_free(self->style_url);
  G_OBJECT_CLASS(
    maplibre_native_ffi_offline_tile_pyramid_region_definition_parent_class
  )
    ->finalize(object);
}

static void
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self =
    MAPLIBRE_NATIVE_FFI_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(object);
  switch (id) {
    case OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_STYLE_URL:
      g_value_set_string(value, self->style_url);
      break;
    case OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_BOUNDS:
      g_value_set_boxed(value, &self->bounds);
      break;
    case OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_MIN_ZOOM:
      g_value_set_double(value, self->min_zoom);
      break;
    case OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_MAX_ZOOM:
      g_value_set_double(value, self->max_zoom);
      break;
    case OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_PIXEL_RATIO:
      g_value_set_float(value, self->pixel_ratio);
      break;
    case OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_INCLUDE_IDEOGRAPHS:
      g_value_set_boolean(value, self->include_ideographs);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void
maplibre_native_ffi_offline_tile_pyramid_region_definition_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self =
    MAPLIBRE_NATIVE_FFI_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(object);
  switch (id) {
    case OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_STYLE_URL:
      maplibre_native_ffi_offline_tile_pyramid_region_definition_set_style_url(
        self, g_value_get_string(value)
      );
      break;
    case OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_BOUNDS:
      maplibre_native_ffi_offline_tile_pyramid_region_definition_set_bounds(
        self, g_value_get_boxed(value)
      );
      break;
    case OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_MIN_ZOOM:
      maplibre_native_ffi_offline_tile_pyramid_region_definition_set_min_zoom(
        self, g_value_get_double(value)
      );
      break;
    case OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_MAX_ZOOM:
      maplibre_native_ffi_offline_tile_pyramid_region_definition_set_max_zoom(
        self, g_value_get_double(value)
      );
      break;
    case OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_PIXEL_RATIO:
      maplibre_native_ffi_offline_tile_pyramid_region_definition_set_pixel_ratio(
        self, g_value_get_float(value)
      );
      break;
    case OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_INCLUDE_IDEOGRAPHS:
      maplibre_native_ffi_offline_tile_pyramid_region_definition_set_include_ideographs(
        self, g_value_get_boolean(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void
maplibre_native_ffi_offline_tile_pyramid_region_definition_class_init(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinitionClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_offline_tile_pyramid_region_definition_finalize;
  object_class->get_property =
    maplibre_native_ffi_offline_tile_pyramid_region_definition_get_property;
  object_class->set_property =
    maplibre_native_ffi_offline_tile_pyramid_region_definition_set_property;
  offline_tile_pyramid_region_definition_properties
    [OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_STYLE_URL] =
      g_param_spec_string("style-url", NULL, NULL, "", PROPERTY_FLAGS);
  offline_tile_pyramid_region_definition_properties
    [OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_BOUNDS] = g_param_spec_boxed(
      "bounds", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LAT_LNG_BOUNDS,
      PROPERTY_FLAGS
    );
  offline_tile_pyramid_region_definition_properties
    [OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_MIN_ZOOM] =
      g_param_spec_double(
        "min-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
      );
  offline_tile_pyramid_region_definition_properties
    [OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_MAX_ZOOM] =
      g_param_spec_double(
        "max-zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
      );
  offline_tile_pyramid_region_definition_properties
    [OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_PIXEL_RATIO] =
      g_param_spec_float(
        "pixel-ratio", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
      );
  offline_tile_pyramid_region_definition_properties
    [OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_INCLUDE_IDEOGRAPHS] =
      g_param_spec_boolean(
        "include-ideographs", NULL, NULL, FALSE, PROPERTY_FLAGS
      );
  g_object_class_install_properties(
    object_class, OFFLINE_TILE_PYRAMID_REGION_DEFINITION_N_PROPERTIES,
    offline_tile_pyramid_region_definition_properties
  );
}

static void maplibre_native_ffi_offline_tile_pyramid_region_definition_init(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  self->style_url = g_strdup("");
}

MaplibreNativeFfiOfflineTilePyramidRegionDefinition*
maplibre_native_ffi_offline_tile_pyramid_region_definition_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_OFFLINE_TILE_PYRAMID_REGION_DEFINITION, NULL
  );
}

const char*
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_style_url(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(self), NULL
  );
  return self->style_url;
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_style_url(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, const char* value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(self)
  );
  char* previous = self->style_url;
  self->style_url = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->style_url) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), offline_tile_pyramid_region_definition_properties
                        [OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_STYLE_URL]
    );
}

MaplibreNativeFfiLatLngBounds*
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_bounds(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(self), NULL
  );
  return (MaplibreNativeFfiLatLngBounds*)&self->bounds;
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_bounds(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self,
  const MaplibreNativeFfiLatLngBounds* value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(self)
  );
  g_return_if_fail(value != NULL);
  gboolean changed = memcmp(&self->bounds, value, sizeof self->bounds) != 0;
  self->bounds = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), offline_tile_pyramid_region_definition_properties
                        [OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_BOUNDS]
    );
}

gdouble maplibre_native_ffi_offline_tile_pyramid_region_definition_get_min_zoom(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(self), 0
  );
  return self->min_zoom;
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_min_zoom(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gdouble value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(self)
  );
  gboolean changed = self->min_zoom != value;
  self->min_zoom = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), offline_tile_pyramid_region_definition_properties
                        [OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_MIN_ZOOM]
    );
}

gdouble maplibre_native_ffi_offline_tile_pyramid_region_definition_get_max_zoom(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(self), 0
  );
  return self->max_zoom;
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_max_zoom(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gdouble value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(self)
  );
  gboolean changed = self->max_zoom != value;
  self->max_zoom = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), offline_tile_pyramid_region_definition_properties
                        [OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_MAX_ZOOM]
    );
}

gfloat
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_pixel_ratio(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(self), 0
  );
  return self->pixel_ratio;
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_pixel_ratio(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gfloat value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(self)
  );
  gboolean changed = self->pixel_ratio != value;
  self->pixel_ratio = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      offline_tile_pyramid_region_definition_properties
        [OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_PIXEL_RATIO]
    );
}

gboolean
maplibre_native_ffi_offline_tile_pyramid_region_definition_get_include_ideographs(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(self), FALSE
  );
  return self->include_ideographs;
}

void maplibre_native_ffi_offline_tile_pyramid_region_definition_set_include_ideographs(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self, gboolean value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(self)
  );
  gboolean changed = self->include_ideographs != value;
  self->include_ideographs = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      offline_tile_pyramid_region_definition_properties
        [OFFLINE_TILE_PYRAMID_REGION_DEFINITION_PROP_INCLUDE_IDEOGRAPHS]
    );
}

MaplibreNativeFfiOfflineTilePyramidRegionDefinition*
maplibre_native_ffi_offline_tile_pyramid_region_definition_copy(
  const MaplibreNativeFfiOfflineTilePyramidRegionDefinition* self
) {
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* copy =
    maplibre_native_ffi_offline_tile_pyramid_region_definition_new();
  g_free(copy->style_url);
  copy->style_url = g_strdup(self->style_url);
  copy->bounds = self->bounds;
  copy->min_zoom = self->min_zoom;
  copy->max_zoom = self->max_zoom;
  copy->pixel_ratio = self->pixel_ratio;
  copy->include_ideographs = self->include_ideographs;
  return copy;
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
  g_clear_object(&self->tile_pyramid);
  g_clear_object(&self->geometry);
  g_free(self);
}

MaplibreNativeFfiOfflineRegionDefinition*
maplibre_native_ffi_offline_region_definition_new_tile_pyramid(
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* value
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_TILE_PYRAMID_REGION_DEFINITION(value), NULL
  );
  MaplibreNativeFfiOfflineRegionDefinition* self =
    g_new0(MaplibreNativeFfiOfflineRegionDefinition, 1);
  self->tag = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
  self->tile_pyramid =
    maplibre_native_ffi_offline_tile_pyramid_region_definition_new();
  MaplibreNativeFfiOfflineTilePyramidRegionDefinition* previous =
    self->tile_pyramid;
  self->tile_pyramid =
    maplibre_native_ffi_offline_tile_pyramid_region_definition_copy(value);
  if (previous != NULL) g_object_unref(previous);
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
  MaplibreNativeFfiOfflineGeometryRegionDefinition* value
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFLINE_GEOMETRY_REGION_DEFINITION(value), NULL
  );
  MaplibreNativeFfiOfflineRegionDefinition* self =
    g_new0(MaplibreNativeFfiOfflineRegionDefinition, 1);
  self->tag = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
  self->geometry = maplibre_native_ffi_offline_geometry_region_definition_new();
  MaplibreNativeFfiOfflineGeometryRegionDefinition* previous = self->geometry;
  self->geometry =
    maplibre_native_ffi_offline_geometry_region_definition_copy(value);
  if (previous != NULL) g_object_unref(previous);
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

struct _MaplibreNativeFfiOffscreenDeviceOptions {
  GObject parent_instance;
  guint64 fields;
  guint32 adapter_index;
  gboolean single_context;
  MaplibreNativeFfiOpenGlClientApi client_api;
};

enum {
  OFFSCREEN_DEVICE_OPTIONS_PROP_0,
  OFFSCREEN_DEVICE_OPTIONS_PROP_ADAPTER_INDEX,
  OFFSCREEN_DEVICE_OPTIONS_PROP_SINGLE_CONTEXT,
  OFFSCREEN_DEVICE_OPTIONS_PROP_CLIENT_API,
  OFFSCREEN_DEVICE_OPTIONS_N_PROPERTIES
};

static GParamSpec*
  offscreen_device_options_properties[OFFSCREEN_DEVICE_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiOffscreenDeviceOptions,
  maplibre_native_ffi_offscreen_device_options, G_TYPE_OBJECT
)

static void maplibre_native_ffi_offscreen_device_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOffscreenDeviceOptions* self =
    MAPLIBRE_NATIVE_FFI_OFFSCREEN_DEVICE_OPTIONS(object);
  switch (id) {
    case OFFSCREEN_DEVICE_OPTIONS_PROP_ADAPTER_INDEX:
      g_value_set_uint(value, self->adapter_index);
      break;
    case OFFSCREEN_DEVICE_OPTIONS_PROP_SINGLE_CONTEXT:
      g_value_set_boolean(value, self->single_context);
      break;
    case OFFSCREEN_DEVICE_OPTIONS_PROP_CLIENT_API:
      g_value_set_enum(value, self->client_api);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_offscreen_device_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOffscreenDeviceOptions* self =
    MAPLIBRE_NATIVE_FFI_OFFSCREEN_DEVICE_OPTIONS(object);
  switch (id) {
    case OFFSCREEN_DEVICE_OPTIONS_PROP_ADAPTER_INDEX:
      maplibre_native_ffi_offscreen_device_options_set_adapter_index(
        self, g_value_get_uint(value)
      );
      break;
    case OFFSCREEN_DEVICE_OPTIONS_PROP_SINGLE_CONTEXT:
      maplibre_native_ffi_offscreen_device_options_set_single_context(
        self, g_value_get_boolean(value)
      );
      break;
    case OFFSCREEN_DEVICE_OPTIONS_PROP_CLIENT_API:
      maplibre_native_ffi_offscreen_device_options_set_client_api(
        self, g_value_get_enum(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_offscreen_device_options_class_init(
  MaplibreNativeFfiOffscreenDeviceOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_offscreen_device_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_offscreen_device_options_set_property;
  offscreen_device_options_properties
    [OFFSCREEN_DEVICE_OPTIONS_PROP_ADAPTER_INDEX] = g_param_spec_uint(
      "adapter-index", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
    );
  offscreen_device_options_properties
    [OFFSCREEN_DEVICE_OPTIONS_PROP_SINGLE_CONTEXT] =
      g_param_spec_boolean("single-context", NULL, NULL, FALSE, PROPERTY_FLAGS);
  offscreen_device_options_properties
    [OFFSCREEN_DEVICE_OPTIONS_PROP_CLIENT_API] = g_param_spec_enum(
      "client-api", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_CLIENT_API, 0,
      PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, OFFSCREEN_DEVICE_OPTIONS_N_PROPERTIES,
    offscreen_device_options_properties
  );
}

static void maplibre_native_ffi_offscreen_device_options_init(
  MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  (void)self;
}

MaplibreNativeFfiOffscreenDeviceOptions*
maplibre_native_ffi_offscreen_device_options_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_OFFSCREEN_DEVICE_OPTIONS, NULL);
}

guint32 maplibre_native_ffi_offscreen_device_options_get_adapter_index(
  MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFSCREEN_DEVICE_OPTIONS(self), 0
  );
  return self->adapter_index;
}

gboolean maplibre_native_ffi_offscreen_device_options_has_adapter_index(
  MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFSCREEN_DEVICE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX) != 0;
}

void maplibre_native_ffi_offscreen_device_options_set_adapter_index(
  MaplibreNativeFfiOffscreenDeviceOptions* self, guint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_OFFSCREEN_DEVICE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX) != 0;
  gboolean changed = self->adapter_index != value;
  self->adapter_index = value;
  self->fields |= MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), offscreen_device_options_properties
                        [OFFSCREEN_DEVICE_OPTIONS_PROP_ADAPTER_INDEX]
    );
}

gboolean maplibre_native_ffi_offscreen_device_options_get_single_context(
  MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFSCREEN_DEVICE_OPTIONS(self), FALSE
  );
  return self->single_context;
}

gboolean maplibre_native_ffi_offscreen_device_options_has_single_context(
  MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFSCREEN_DEVICE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT) != 0;
}

void maplibre_native_ffi_offscreen_device_options_set_single_context(
  MaplibreNativeFfiOffscreenDeviceOptions* self, gboolean value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_OFFSCREEN_DEVICE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT) != 0;
  gboolean changed = self->single_context != value;
  self->single_context = value;
  self->fields |= MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), offscreen_device_options_properties
                        [OFFSCREEN_DEVICE_OPTIONS_PROP_SINGLE_CONTEXT]
    );
}

MaplibreNativeFfiOpenGlClientApi
maplibre_native_ffi_offscreen_device_options_get_client_api(
  MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFSCREEN_DEVICE_OPTIONS(self),
    (MaplibreNativeFfiOpenGlClientApi)0
  );
  return self->client_api;
}

gboolean maplibre_native_ffi_offscreen_device_options_has_client_api(
  MaplibreNativeFfiOffscreenDeviceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OFFSCREEN_DEVICE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API) != 0;
}

void maplibre_native_ffi_offscreen_device_options_set_client_api(
  MaplibreNativeFfiOffscreenDeviceOptions* self,
  MaplibreNativeFfiOpenGlClientApi value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_OFFSCREEN_DEVICE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API) != 0;
  gboolean changed = self->client_api != value;
  self->client_api = value;
  self->fields |= MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), offscreen_device_options_properties
                        [OFFSCREEN_DEVICE_OPTIONS_PROP_CLIENT_API]
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiOpenGlBorrowedTexture) ==
  sizeof(mln_open_gl_borrowed_texture)
);
SAME_MEMBER(
  MaplibreNativeFfiOpenGlBorrowedTexture, mln_open_gl_borrowed_texture, texture
);

void maplibre_native_ffi_open_gl_borrowed_texture_to_native(
  const MaplibreNativeFfiOpenGlBorrowedTexture* self,
  mln_open_gl_borrowed_texture* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

struct _MaplibreNativeFfiWglContextDescriptor {
  GObject parent_instance;
  gpointer device_context;
  gpointer share_context;
  gpointer get_proc_address;
};

enum {
  WGL_CONTEXT_DESCRIPTOR_PROP_0,
  WGL_CONTEXT_DESCRIPTOR_PROP_DEVICE_CONTEXT,
  WGL_CONTEXT_DESCRIPTOR_PROP_SHARE_CONTEXT,
  WGL_CONTEXT_DESCRIPTOR_PROP_GET_PROC_ADDRESS,
  WGL_CONTEXT_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec*
  wgl_context_descriptor_properties[WGL_CONTEXT_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiWglContextDescriptor,
  maplibre_native_ffi_wgl_context_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_wgl_context_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiWglContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_WGL_CONTEXT_DESCRIPTOR(object);
  switch (id) {
    case WGL_CONTEXT_DESCRIPTOR_PROP_DEVICE_CONTEXT:
      g_value_set_pointer(value, self->device_context);
      break;
    case WGL_CONTEXT_DESCRIPTOR_PROP_SHARE_CONTEXT:
      g_value_set_pointer(value, self->share_context);
      break;
    case WGL_CONTEXT_DESCRIPTOR_PROP_GET_PROC_ADDRESS:
      g_value_set_pointer(value, self->get_proc_address);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_wgl_context_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiWglContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_WGL_CONTEXT_DESCRIPTOR(object);
  switch (id) {
    case WGL_CONTEXT_DESCRIPTOR_PROP_DEVICE_CONTEXT:
      maplibre_native_ffi_wgl_context_descriptor_set_device_context(
        self, g_value_get_pointer(value)
      );
      break;
    case WGL_CONTEXT_DESCRIPTOR_PROP_SHARE_CONTEXT:
      maplibre_native_ffi_wgl_context_descriptor_set_share_context(
        self, g_value_get_pointer(value)
      );
      break;
    case WGL_CONTEXT_DESCRIPTOR_PROP_GET_PROC_ADDRESS:
      maplibre_native_ffi_wgl_context_descriptor_set_get_proc_address(
        self, g_value_get_pointer(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_wgl_context_descriptor_class_init(
  MaplibreNativeFfiWglContextDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_wgl_context_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_wgl_context_descriptor_set_property;
  wgl_context_descriptor_properties
    [WGL_CONTEXT_DESCRIPTOR_PROP_DEVICE_CONTEXT] =
      g_param_spec_pointer("device-context", NULL, NULL, PROPERTY_FLAGS);
  wgl_context_descriptor_properties[WGL_CONTEXT_DESCRIPTOR_PROP_SHARE_CONTEXT] =
    g_param_spec_pointer("share-context", NULL, NULL, PROPERTY_FLAGS);
  wgl_context_descriptor_properties
    [WGL_CONTEXT_DESCRIPTOR_PROP_GET_PROC_ADDRESS] =
      g_param_spec_pointer("get-proc-address", NULL, NULL, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, WGL_CONTEXT_DESCRIPTOR_N_PROPERTIES,
    wgl_context_descriptor_properties
  );
}

static void maplibre_native_ffi_wgl_context_descriptor_init(
  MaplibreNativeFfiWglContextDescriptor* self
) {
  (void)self;
}

MaplibreNativeFfiWglContextDescriptor*
maplibre_native_ffi_wgl_context_descriptor_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_WGL_CONTEXT_DESCRIPTOR, NULL);
}

gpointer maplibre_native_ffi_wgl_context_descriptor_get_device_context(
  MaplibreNativeFfiWglContextDescriptor* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_WGL_CONTEXT_DESCRIPTOR(self), 0);
  return self->device_context;
}

void maplibre_native_ffi_wgl_context_descriptor_set_device_context(
  MaplibreNativeFfiWglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_WGL_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->device_context != value;
  self->device_context = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), wgl_context_descriptor_properties
                        [WGL_CONTEXT_DESCRIPTOR_PROP_DEVICE_CONTEXT]
    );
}

gpointer maplibre_native_ffi_wgl_context_descriptor_get_share_context(
  MaplibreNativeFfiWglContextDescriptor* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_WGL_CONTEXT_DESCRIPTOR(self), 0);
  return self->share_context;
}

void maplibre_native_ffi_wgl_context_descriptor_set_share_context(
  MaplibreNativeFfiWglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_WGL_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->share_context != value;
  self->share_context = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), wgl_context_descriptor_properties
                        [WGL_CONTEXT_DESCRIPTOR_PROP_SHARE_CONTEXT]
    );
}

gpointer maplibre_native_ffi_wgl_context_descriptor_get_get_proc_address(
  MaplibreNativeFfiWglContextDescriptor* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_WGL_CONTEXT_DESCRIPTOR(self), 0);
  return self->get_proc_address;
}

void maplibre_native_ffi_wgl_context_descriptor_set_get_proc_address(
  MaplibreNativeFfiWglContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_WGL_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->get_proc_address != value;
  self->get_proc_address = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), wgl_context_descriptor_properties
                        [WGL_CONTEXT_DESCRIPTOR_PROP_GET_PROC_ADDRESS]
    );
}

MaplibreNativeFfiWglContextDescriptor*
maplibre_native_ffi_wgl_context_descriptor_copy(
  const MaplibreNativeFfiWglContextDescriptor* self
) {
  MaplibreNativeFfiWglContextDescriptor* copy =
    maplibre_native_ffi_wgl_context_descriptor_new();
  copy->device_context = self->device_context;
  copy->share_context = self->share_context;
  copy->get_proc_address = self->get_proc_address;
  return copy;
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

struct _MaplibreNativeFfiWebGlContextDescriptor {
  GObject parent_instance;
  MaplibreNativeFfiWebGlContextKind kind;
  gint32 context;
  char* canvas_selector;
};

enum {
  WEB_GL_CONTEXT_DESCRIPTOR_PROP_0,
  WEB_GL_CONTEXT_DESCRIPTOR_PROP_KIND,
  WEB_GL_CONTEXT_DESCRIPTOR_PROP_CONTEXT,
  WEB_GL_CONTEXT_DESCRIPTOR_PROP_CANVAS_SELECTOR,
  WEB_GL_CONTEXT_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec*
  web_gl_context_descriptor_properties[WEB_GL_CONTEXT_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiWebGlContextDescriptor,
  maplibre_native_ffi_web_gl_context_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_web_gl_context_descriptor_finalize(
  GObject* object
) {
  MaplibreNativeFfiWebGlContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_WEB_GL_CONTEXT_DESCRIPTOR(object);
  g_free(self->canvas_selector);
  G_OBJECT_CLASS(maplibre_native_ffi_web_gl_context_descriptor_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_web_gl_context_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiWebGlContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_WEB_GL_CONTEXT_DESCRIPTOR(object);
  switch (id) {
    case WEB_GL_CONTEXT_DESCRIPTOR_PROP_KIND:
      g_value_set_enum(value, self->kind);
      break;
    case WEB_GL_CONTEXT_DESCRIPTOR_PROP_CONTEXT:
      g_value_set_int(value, self->context);
      break;
    case WEB_GL_CONTEXT_DESCRIPTOR_PROP_CANVAS_SELECTOR:
      g_value_set_string(value, self->canvas_selector);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_web_gl_context_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiWebGlContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_WEB_GL_CONTEXT_DESCRIPTOR(object);
  switch (id) {
    case WEB_GL_CONTEXT_DESCRIPTOR_PROP_KIND:
      maplibre_native_ffi_web_gl_context_descriptor_set_kind(
        self, g_value_get_enum(value)
      );
      break;
    case WEB_GL_CONTEXT_DESCRIPTOR_PROP_CONTEXT:
      maplibre_native_ffi_web_gl_context_descriptor_set_context(
        self, g_value_get_int(value)
      );
      break;
    case WEB_GL_CONTEXT_DESCRIPTOR_PROP_CANVAS_SELECTOR:
      maplibre_native_ffi_web_gl_context_descriptor_set_canvas_selector(
        self, g_value_get_string(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_web_gl_context_descriptor_class_init(
  MaplibreNativeFfiWebGlContextDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_web_gl_context_descriptor_finalize;
  object_class->get_property =
    maplibre_native_ffi_web_gl_context_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_web_gl_context_descriptor_set_property;
  web_gl_context_descriptor_properties[WEB_GL_CONTEXT_DESCRIPTOR_PROP_KIND] =
    g_param_spec_enum(
      "kind", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_WEB_GL_CONTEXT_KIND, 0,
      PROPERTY_FLAGS
    );
  web_gl_context_descriptor_properties[WEB_GL_CONTEXT_DESCRIPTOR_PROP_CONTEXT] =
    g_param_spec_int(
      "context", NULL, NULL, G_MININT32, G_MAXINT32, 0, PROPERTY_FLAGS
    );
  web_gl_context_descriptor_properties
    [WEB_GL_CONTEXT_DESCRIPTOR_PROP_CANVAS_SELECTOR] =
      g_param_spec_string("canvas-selector", NULL, NULL, "", PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, WEB_GL_CONTEXT_DESCRIPTOR_N_PROPERTIES,
    web_gl_context_descriptor_properties
  );
}

static void maplibre_native_ffi_web_gl_context_descriptor_init(
  MaplibreNativeFfiWebGlContextDescriptor* self
) {
  self->canvas_selector = g_strdup("");
}

MaplibreNativeFfiWebGlContextDescriptor*
maplibre_native_ffi_web_gl_context_descriptor_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_WEB_GL_CONTEXT_DESCRIPTOR, NULL);
}

MaplibreNativeFfiWebGlContextKind
maplibre_native_ffi_web_gl_context_descriptor_get_kind(
  MaplibreNativeFfiWebGlContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_WEB_GL_CONTEXT_DESCRIPTOR(self),
    (MaplibreNativeFfiWebGlContextKind)0
  );
  return self->kind;
}

void maplibre_native_ffi_web_gl_context_descriptor_set_kind(
  MaplibreNativeFfiWebGlContextDescriptor* self,
  MaplibreNativeFfiWebGlContextKind value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_WEB_GL_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->kind != value;
  self->kind = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      web_gl_context_descriptor_properties[WEB_GL_CONTEXT_DESCRIPTOR_PROP_KIND]
    );
}

gint32 maplibre_native_ffi_web_gl_context_descriptor_get_context(
  MaplibreNativeFfiWebGlContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_WEB_GL_CONTEXT_DESCRIPTOR(self), 0
  );
  return self->context;
}

void maplibre_native_ffi_web_gl_context_descriptor_set_context(
  MaplibreNativeFfiWebGlContextDescriptor* self, gint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_WEB_GL_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->context != value;
  self->context = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), web_gl_context_descriptor_properties
                        [WEB_GL_CONTEXT_DESCRIPTOR_PROP_CONTEXT]
    );
}

const char* maplibre_native_ffi_web_gl_context_descriptor_get_canvas_selector(
  MaplibreNativeFfiWebGlContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_WEB_GL_CONTEXT_DESCRIPTOR(self), NULL
  );
  return self->canvas_selector;
}

void maplibre_native_ffi_web_gl_context_descriptor_set_canvas_selector(
  MaplibreNativeFfiWebGlContextDescriptor* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_WEB_GL_CONTEXT_DESCRIPTOR(self));
  char* previous = self->canvas_selector;
  self->canvas_selector = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->canvas_selector) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), web_gl_context_descriptor_properties
                        [WEB_GL_CONTEXT_DESCRIPTOR_PROP_CANVAS_SELECTOR]
    );
}

MaplibreNativeFfiWebGlContextDescriptor*
maplibre_native_ffi_web_gl_context_descriptor_copy(
  const MaplibreNativeFfiWebGlContextDescriptor* self
) {
  MaplibreNativeFfiWebGlContextDescriptor* copy =
    maplibre_native_ffi_web_gl_context_descriptor_new();
  copy->kind = self->kind;
  copy->context = self->context;
  g_free(copy->canvas_selector);
  copy->canvas_selector = g_strdup(self->canvas_selector);
  return copy;
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
  g_clear_object(&self->wgl);
  g_clear_object(&self->egl);
  g_clear_object(&self->web_gl);
  g_free(self);
}

MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_data_new_wgl(
  MaplibreNativeFfiWglContextDescriptor* value
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_WGL_CONTEXT_DESCRIPTOR(value), NULL
  );
  MaplibreNativeFfiOpenGlContextDescriptorData* self =
    g_new0(MaplibreNativeFfiOpenGlContextDescriptorData, 1);
  self->tag = MLN_OPEN_GL_CONTEXT_PLATFORM_WGL;
  self->wgl = maplibre_native_ffi_wgl_context_descriptor_new();
  MaplibreNativeFfiWglContextDescriptor* previous = self->wgl;
  self->wgl = maplibre_native_ffi_wgl_context_descriptor_copy(value);
  if (previous != NULL) g_object_unref(previous);
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
  MaplibreNativeFfiEglContextDescriptor* value
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_EGL_CONTEXT_DESCRIPTOR(value), NULL
  );
  MaplibreNativeFfiOpenGlContextDescriptorData* self =
    g_new0(MaplibreNativeFfiOpenGlContextDescriptorData, 1);
  self->tag = MLN_OPEN_GL_CONTEXT_PLATFORM_EGL;
  self->egl = maplibre_native_ffi_egl_context_descriptor_new();
  MaplibreNativeFfiEglContextDescriptor* previous = self->egl;
  self->egl = maplibre_native_ffi_egl_context_descriptor_copy(value);
  if (previous != NULL) g_object_unref(previous);
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
  MaplibreNativeFfiWebGlContextDescriptor* value
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_WEB_GL_CONTEXT_DESCRIPTOR(value), NULL
  );
  MaplibreNativeFfiOpenGlContextDescriptorData* self =
    g_new0(MaplibreNativeFfiOpenGlContextDescriptorData, 1);
  self->tag = MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL;
  self->web_gl = maplibre_native_ffi_web_gl_context_descriptor_new();
  MaplibreNativeFfiWebGlContextDescriptor* previous = self->web_gl;
  self->web_gl = maplibre_native_ffi_web_gl_context_descriptor_copy(value);
  if (previous != NULL) g_object_unref(previous);
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

struct _MaplibreNativeFfiOpenGlContextDescriptor {
  GObject parent_instance;
  MaplibreNativeFfiOpenGlContextOwnership ownership;
  MaplibreNativeFfiOpenGlContextDescriptorData* data;
};

enum {
  OPEN_GL_CONTEXT_DESCRIPTOR_PROP_0,
  OPEN_GL_CONTEXT_DESCRIPTOR_PROP_OWNERSHIP,
  OPEN_GL_CONTEXT_DESCRIPTOR_PROP_DATA,
  OPEN_GL_CONTEXT_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec* open_gl_context_descriptor_properties
  [OPEN_GL_CONTEXT_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiOpenGlContextDescriptor,
  maplibre_native_ffi_open_gl_context_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_open_gl_context_descriptor_finalize(
  GObject* object
) {
  MaplibreNativeFfiOpenGlContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_DESCRIPTOR(object);
  g_clear_pointer(
    &self->data, maplibre_native_ffi_open_gl_context_descriptor_data_free
  );
  G_OBJECT_CLASS(maplibre_native_ffi_open_gl_context_descriptor_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_open_gl_context_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOpenGlContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_DESCRIPTOR(object);
  switch (id) {
    case OPEN_GL_CONTEXT_DESCRIPTOR_PROP_OWNERSHIP:
      g_value_set_enum(value, self->ownership);
      break;
    case OPEN_GL_CONTEXT_DESCRIPTOR_PROP_DATA:
      g_value_set_boxed(value, self->data);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_open_gl_context_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOpenGlContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_DESCRIPTOR(object);
  switch (id) {
    case OPEN_GL_CONTEXT_DESCRIPTOR_PROP_OWNERSHIP:
      maplibre_native_ffi_open_gl_context_descriptor_set_ownership(
        self, g_value_get_enum(value)
      );
      break;
    case OPEN_GL_CONTEXT_DESCRIPTOR_PROP_DATA:
      maplibre_native_ffi_open_gl_context_descriptor_set_data(
        self, g_value_get_boxed(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_open_gl_context_descriptor_class_init(
  MaplibreNativeFfiOpenGlContextDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_open_gl_context_descriptor_finalize;
  object_class->get_property =
    maplibre_native_ffi_open_gl_context_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_open_gl_context_descriptor_set_property;
  open_gl_context_descriptor_properties
    [OPEN_GL_CONTEXT_DESCRIPTOR_PROP_OWNERSHIP] = g_param_spec_enum(
      "ownership", NULL, NULL,
      MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_CONTEXT_OWNERSHIP, 0, PROPERTY_FLAGS
    );
  open_gl_context_descriptor_properties[OPEN_GL_CONTEXT_DESCRIPTOR_PROP_DATA] =
    g_param_spec_boxed(
      "data", NULL, NULL,
      MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_CONTEXT_DESCRIPTOR_DATA, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, OPEN_GL_CONTEXT_DESCRIPTOR_N_PROPERTIES,
    open_gl_context_descriptor_properties
  );
}

static void maplibre_native_ffi_open_gl_context_descriptor_init(
  MaplibreNativeFfiOpenGlContextDescriptor* self
) {
  (void)self;
}

MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_CONTEXT_DESCRIPTOR, NULL
  );
}

MaplibreNativeFfiOpenGlContextOwnership
maplibre_native_ffi_open_gl_context_descriptor_get_ownership(
  MaplibreNativeFfiOpenGlContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_CONTEXT_DESCRIPTOR(self),
    (MaplibreNativeFfiOpenGlContextOwnership)0
  );
  return self->ownership;
}

void maplibre_native_ffi_open_gl_context_descriptor_set_ownership(
  MaplibreNativeFfiOpenGlContextDescriptor* self,
  MaplibreNativeFfiOpenGlContextOwnership value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->ownership != value;
  self->ownership = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), open_gl_context_descriptor_properties
                        [OPEN_GL_CONTEXT_DESCRIPTOR_PROP_OWNERSHIP]
    );
}

MaplibreNativeFfiOpenGlContextDescriptorData*
maplibre_native_ffi_open_gl_context_descriptor_get_data(
  MaplibreNativeFfiOpenGlContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_CONTEXT_DESCRIPTOR(self), NULL
  );
  return self->data;
}

void maplibre_native_ffi_open_gl_context_descriptor_set_data(
  MaplibreNativeFfiOpenGlContextDescriptor* self,
  const MaplibreNativeFfiOpenGlContextDescriptorData* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_CONTEXT_DESCRIPTOR(self));
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiOpenGlContextDescriptorData* previous = self->data;
  self->data = maplibre_native_ffi_open_gl_context_descriptor_data_copy(value);
  if (previous != NULL)
    maplibre_native_ffi_open_gl_context_descriptor_data_free(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self),
    open_gl_context_descriptor_properties[OPEN_GL_CONTEXT_DESCRIPTOR_PROP_DATA]
  );
}

MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_context_descriptor_copy(
  const MaplibreNativeFfiOpenGlContextDescriptor* self
) {
  MaplibreNativeFfiOpenGlContextDescriptor* copy =
    maplibre_native_ffi_open_gl_context_descriptor_new();
  copy->ownership = self->ownership;
  copy->data =
    self->data != NULL
      ? maplibre_native_ffi_open_gl_context_descriptor_data_copy(self->data)
      : NULL;
  return copy;
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

struct _MaplibreNativeFfiOpenGlBorrowedTextureDescriptor {
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 physical_width;
  guint32 physical_height;
  MaplibreNativeFfiOpenGlContextDescriptor* context;
  GPtrArray* textures;
  guint32 target;
};

enum {
  OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_0,
  OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE,
  OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH,
  OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT,
  OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_CONTEXT,
  OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_TARGET,
  OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec* open_gl_borrowed_texture_descriptor_properties
  [OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor,
  maplibre_native_ffi_open_gl_borrowed_texture_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_finalize(
  GObject* object
) {
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(object);
  g_clear_object(&self->context);
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  G_OBJECT_CLASS(
    maplibre_native_ffi_open_gl_borrowed_texture_descriptor_parent_class
  )
    ->finalize(object);
}

static void
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(object);
  switch (id) {
    case OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH:
      g_value_set_uint(value, self->physical_width);
      break;
    case OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT:
      g_value_set_uint(value, self->physical_height);
      break;
    case OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_CONTEXT:
      g_value_set_object(value, self->context);
      break;
    case OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_TARGET:
      g_value_set_uint(value, self->target);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(object);
  switch (id) {
    case OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH:
      maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_width(
        self, g_value_get_uint(value)
      );
      break;
    case OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT:
      maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_height(
        self, g_value_get_uint(value)
      );
      break;
    case OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_CONTEXT:
      maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_context(
        self, g_value_get_object(value)
      );
      break;
    case OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_TARGET:
      maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_target(
        self, g_value_get_uint(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_class_init(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_open_gl_borrowed_texture_descriptor_finalize;
  object_class->get_property =
    maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_property;
  open_gl_borrowed_texture_descriptor_properties
    [OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE] =
      g_param_spec_boxed(
        "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
        PROPERTY_FLAGS
      );
  open_gl_borrowed_texture_descriptor_properties
    [OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH] =
      g_param_spec_uint(
        "physical-width", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
      );
  open_gl_borrowed_texture_descriptor_properties
    [OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT] =
      g_param_spec_uint(
        "physical-height", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
      );
  open_gl_borrowed_texture_descriptor_properties
    [OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_CONTEXT] = g_param_spec_object(
      "context", NULL, NULL,
      MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_CONTEXT_DESCRIPTOR, PROPERTY_FLAGS
    );
  open_gl_borrowed_texture_descriptor_properties
    [OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_TARGET] = g_param_spec_uint(
      "target", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_N_PROPERTIES,
    open_gl_borrowed_texture_descriptor_properties
  );
}

static void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_init(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  self->context = maplibre_native_ffi_open_gl_context_descriptor_new();
  self->textures = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_open_gl_borrowed_texture_free
  );
}

MaplibreNativeFfiOpenGlBorrowedTextureDescriptor*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR, NULL
  );
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_logical_size(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_logical_size(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), open_gl_borrowed_texture_descriptor_properties
                        [OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE]
    );
}

guint32
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_physical_width(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(self), 0
  );
  return self->physical_width;
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_width(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  gboolean changed = self->physical_width != value;
  self->physical_width = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      open_gl_borrowed_texture_descriptor_properties
        [OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH]
    );
}

guint32
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_physical_height(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(self), 0
  );
  return self->physical_height;
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_height(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  gboolean changed = self->physical_height != value;
  self->physical_height = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      open_gl_borrowed_texture_descriptor_properties
        [OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT]
    );
}

MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_context(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(self), NULL
  );
  return self->context;
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_context(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self,
  MaplibreNativeFfiOpenGlContextDescriptor* value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_CONTEXT_DESCRIPTOR(value));
  MaplibreNativeFfiOpenGlContextDescriptor* previous = self->context;
  self->context = maplibre_native_ffi_open_gl_context_descriptor_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), open_gl_borrowed_texture_descriptor_properties
                      [OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_CONTEXT]
  );
}

GPtrArray* maplibre_native_ffi_open_gl_borrowed_texture_descriptor_get_textures(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(self), NULL
  );
  return self->textures;
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_textures(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiOpenGlBorrowedTexture* value, gsize value_count
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
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
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(self), 0
  );
  return self->target;
}

void maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_target(
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  gboolean changed = self->target != value;
  self->target = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), open_gl_borrowed_texture_descriptor_properties
                        [OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR_PROP_TARGET]
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

struct _MaplibreNativeFfiOpenGlBorrowedTextureReplacement {
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 physical_width;
  guint32 physical_height;
  GPtrArray* textures;
};

enum {
  OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_0,
  OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE,
  OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH,
  OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT,
  OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_N_PROPERTIES
};

static GParamSpec* open_gl_borrowed_texture_replacement_properties
  [OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement,
  maplibre_native_ffi_open_gl_borrowed_texture_replacement, G_TYPE_OBJECT
)

static void maplibre_native_ffi_open_gl_borrowed_texture_replacement_finalize(
  GObject* object
) {
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT(object);
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  G_OBJECT_CLASS(
    maplibre_native_ffi_open_gl_borrowed_texture_replacement_parent_class
  )
    ->finalize(object);
}

static void
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT(object);
  switch (id) {
    case OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH:
      g_value_set_uint(value, self->physical_width);
      break;
    case OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT:
      g_value_set_uint(value, self->physical_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void
maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT(object);
  switch (id) {
    case OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH:
      maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_width(
        self, g_value_get_uint(value)
      );
      break;
    case OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT:
      maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_height(
        self, g_value_get_uint(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_open_gl_borrowed_texture_replacement_class_init(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacementClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_open_gl_borrowed_texture_replacement_finalize;
  object_class->get_property =
    maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_property;
  object_class->set_property =
    maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_property;
  open_gl_borrowed_texture_replacement_properties
    [OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE] =
      g_param_spec_boxed(
        "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
        PROPERTY_FLAGS
      );
  open_gl_borrowed_texture_replacement_properties
    [OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH] =
      g_param_spec_uint(
        "physical-width", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
      );
  open_gl_borrowed_texture_replacement_properties
    [OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT] =
      g_param_spec_uint(
        "physical-height", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
      );
  g_object_class_install_properties(
    object_class, OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_N_PROPERTIES,
    open_gl_borrowed_texture_replacement_properties
  );
}

static void maplibre_native_ffi_open_gl_borrowed_texture_replacement_init(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
) {
  self->textures = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_open_gl_borrowed_texture_free
  );
}

MaplibreNativeFfiOpenGlBorrowedTextureReplacement*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT, NULL
  );
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_logical_size(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_logical_size(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT(self)
  );
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), open_gl_borrowed_texture_replacement_properties
                        [OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE]
    );
}

guint32
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_physical_width(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT(self), 0
  );
  return self->physical_width;
}

void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_width(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT(self)
  );
  gboolean changed = self->physical_width != value;
  self->physical_width = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      open_gl_borrowed_texture_replacement_properties
        [OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH]
    );
}

guint32
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_physical_height(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT(self), 0
  );
  return self->physical_height;
}

void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_physical_height(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT(self)
  );
  gboolean changed = self->physical_height != value;
  self->physical_height = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      open_gl_borrowed_texture_replacement_properties
        [OPEN_GL_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT]
    );
}

GPtrArray*
maplibre_native_ffi_open_gl_borrowed_texture_replacement_get_textures(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT(self), NULL
  );
  return self->textures;
}

void maplibre_native_ffi_open_gl_borrowed_texture_replacement_set_textures(
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* self,
  const MaplibreNativeFfiOpenGlBorrowedTexture* value, gsize value_count
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT(self)
  );
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

struct _MaplibreNativeFfiOpenGlSurfaceDescriptor {
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  MaplibreNativeFfiOpenGlContextDescriptor* context;
  gpointer surface;
};

enum {
  OPEN_GL_SURFACE_DESCRIPTOR_PROP_0,
  OPEN_GL_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE,
  OPEN_GL_SURFACE_DESCRIPTOR_PROP_CONTEXT,
  OPEN_GL_SURFACE_DESCRIPTOR_PROP_SURFACE,
  OPEN_GL_SURFACE_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec* open_gl_surface_descriptor_properties
  [OPEN_GL_SURFACE_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiOpenGlSurfaceDescriptor,
  maplibre_native_ffi_open_gl_surface_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_open_gl_surface_descriptor_finalize(
  GObject* object
) {
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_SURFACE_DESCRIPTOR(object);
  g_clear_object(&self->context);
  G_OBJECT_CLASS(maplibre_native_ffi_open_gl_surface_descriptor_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_open_gl_surface_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_SURFACE_DESCRIPTOR(object);
  switch (id) {
    case OPEN_GL_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case OPEN_GL_SURFACE_DESCRIPTOR_PROP_CONTEXT:
      g_value_set_object(value, self->context);
      break;
    case OPEN_GL_SURFACE_DESCRIPTOR_PROP_SURFACE:
      g_value_set_pointer(value, self->surface);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_open_gl_surface_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_SURFACE_DESCRIPTOR(object);
  switch (id) {
    case OPEN_GL_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_open_gl_surface_descriptor_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case OPEN_GL_SURFACE_DESCRIPTOR_PROP_CONTEXT:
      maplibre_native_ffi_open_gl_surface_descriptor_set_context(
        self, g_value_get_object(value)
      );
      break;
    case OPEN_GL_SURFACE_DESCRIPTOR_PROP_SURFACE:
      maplibre_native_ffi_open_gl_surface_descriptor_set_surface(
        self, g_value_get_pointer(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_open_gl_surface_descriptor_class_init(
  MaplibreNativeFfiOpenGlSurfaceDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_open_gl_surface_descriptor_finalize;
  object_class->get_property =
    maplibre_native_ffi_open_gl_surface_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_open_gl_surface_descriptor_set_property;
  open_gl_surface_descriptor_properties
    [OPEN_GL_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE] = g_param_spec_boxed(
      "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
      PROPERTY_FLAGS
    );
  open_gl_surface_descriptor_properties
    [OPEN_GL_SURFACE_DESCRIPTOR_PROP_CONTEXT] = g_param_spec_object(
      "context", NULL, NULL,
      MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_CONTEXT_DESCRIPTOR, PROPERTY_FLAGS
    );
  open_gl_surface_descriptor_properties
    [OPEN_GL_SURFACE_DESCRIPTOR_PROP_SURFACE] =
      g_param_spec_pointer("surface", NULL, NULL, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, OPEN_GL_SURFACE_DESCRIPTOR_N_PROPERTIES,
    open_gl_surface_descriptor_properties
  );
}

static void maplibre_native_ffi_open_gl_surface_descriptor_init(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self
) {
  self->context = maplibre_native_ffi_open_gl_context_descriptor_new();
}

MaplibreNativeFfiOpenGlSurfaceDescriptor*
maplibre_native_ffi_open_gl_surface_descriptor_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_SURFACE_DESCRIPTOR, NULL
  );
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_surface_descriptor_get_logical_size(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_SURFACE_DESCRIPTOR(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_open_gl_surface_descriptor_set_logical_size(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_SURFACE_DESCRIPTOR(self));
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), open_gl_surface_descriptor_properties
                        [OPEN_GL_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE]
    );
}

MaplibreNativeFfiOpenGlContextDescriptor*
maplibre_native_ffi_open_gl_surface_descriptor_get_context(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_SURFACE_DESCRIPTOR(self), NULL
  );
  return self->context;
}

void maplibre_native_ffi_open_gl_surface_descriptor_set_context(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self,
  MaplibreNativeFfiOpenGlContextDescriptor* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_SURFACE_DESCRIPTOR(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_CONTEXT_DESCRIPTOR(value));
  MaplibreNativeFfiOpenGlContextDescriptor* previous = self->context;
  self->context = maplibre_native_ffi_open_gl_context_descriptor_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), open_gl_surface_descriptor_properties
                      [OPEN_GL_SURFACE_DESCRIPTOR_PROP_CONTEXT]
  );
}

gpointer maplibre_native_ffi_open_gl_surface_descriptor_get_surface(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_SURFACE_DESCRIPTOR(self), 0
  );
  return self->surface;
}

void maplibre_native_ffi_open_gl_surface_descriptor_set_surface(
  MaplibreNativeFfiOpenGlSurfaceDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_SURFACE_DESCRIPTOR(self));
  gboolean changed = self->surface != value;
  self->surface = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), open_gl_surface_descriptor_properties
                        [OPEN_GL_SURFACE_DESCRIPTOR_PROP_SURFACE]
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

struct _MaplibreNativeFfiOpenGlSurfaceReplacement {
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  gpointer surface;
};

enum {
  OPEN_GL_SURFACE_REPLACEMENT_PROP_0,
  OPEN_GL_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE,
  OPEN_GL_SURFACE_REPLACEMENT_PROP_SURFACE,
  OPEN_GL_SURFACE_REPLACEMENT_N_PROPERTIES
};

static GParamSpec* open_gl_surface_replacement_properties
  [OPEN_GL_SURFACE_REPLACEMENT_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiOpenGlSurfaceReplacement,
  maplibre_native_ffi_open_gl_surface_replacement, G_TYPE_OBJECT
)

static void maplibre_native_ffi_open_gl_surface_replacement_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOpenGlSurfaceReplacement* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_SURFACE_REPLACEMENT(object);
  switch (id) {
    case OPEN_GL_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case OPEN_GL_SURFACE_REPLACEMENT_PROP_SURFACE:
      g_value_set_pointer(value, self->surface);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_open_gl_surface_replacement_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiOpenGlSurfaceReplacement* self =
    MAPLIBRE_NATIVE_FFI_OPEN_GL_SURFACE_REPLACEMENT(object);
  switch (id) {
    case OPEN_GL_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_open_gl_surface_replacement_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case OPEN_GL_SURFACE_REPLACEMENT_PROP_SURFACE:
      maplibre_native_ffi_open_gl_surface_replacement_set_surface(
        self, g_value_get_pointer(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_open_gl_surface_replacement_class_init(
  MaplibreNativeFfiOpenGlSurfaceReplacementClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_open_gl_surface_replacement_get_property;
  object_class->set_property =
    maplibre_native_ffi_open_gl_surface_replacement_set_property;
  open_gl_surface_replacement_properties
    [OPEN_GL_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE] = g_param_spec_boxed(
      "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
      PROPERTY_FLAGS
    );
  open_gl_surface_replacement_properties
    [OPEN_GL_SURFACE_REPLACEMENT_PROP_SURFACE] =
      g_param_spec_pointer("surface", NULL, NULL, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, OPEN_GL_SURFACE_REPLACEMENT_N_PROPERTIES,
    open_gl_surface_replacement_properties
  );
}

static void maplibre_native_ffi_open_gl_surface_replacement_init(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self
) {
  (void)self;
}

MaplibreNativeFfiOpenGlSurfaceReplacement*
maplibre_native_ffi_open_gl_surface_replacement_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_SURFACE_REPLACEMENT, NULL
  );
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_open_gl_surface_replacement_get_logical_size(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_SURFACE_REPLACEMENT(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_open_gl_surface_replacement_set_logical_size(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_SURFACE_REPLACEMENT(self));
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), open_gl_surface_replacement_properties
                        [OPEN_GL_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE]
    );
}

gpointer maplibre_native_ffi_open_gl_surface_replacement_get_surface(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_SURFACE_REPLACEMENT(self), 0
  );
  return self->surface;
}

void maplibre_native_ffi_open_gl_surface_replacement_set_surface(
  MaplibreNativeFfiOpenGlSurfaceReplacement* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_SURFACE_REPLACEMENT(self));
  gboolean changed = self->surface != value;
  self->surface = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), open_gl_surface_replacement_properties
                        [OPEN_GL_SURFACE_REPLACEMENT_PROP_SURFACE]
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
  GObject parent_instance;
  guint32 width;
  guint32 height;
  guint32 stride;
  GBytes* pixels;
};

enum {
  PREMULTIPLIED_RGBA8_IMAGE_PROP_0,
  PREMULTIPLIED_RGBA8_IMAGE_PROP_WIDTH,
  PREMULTIPLIED_RGBA8_IMAGE_PROP_HEIGHT,
  PREMULTIPLIED_RGBA8_IMAGE_PROP_STRIDE,
  PREMULTIPLIED_RGBA8_IMAGE_PROP_PIXELS,
  PREMULTIPLIED_RGBA8_IMAGE_N_PROPERTIES
};

static GParamSpec*
  premultiplied_rgba8_image_properties[PREMULTIPLIED_RGBA8_IMAGE_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiPremultipliedRgba8Image,
  maplibre_native_ffi_premultiplied_rgba8_image, G_TYPE_OBJECT
)

static void maplibre_native_ffi_premultiplied_rgba8_image_finalize(
  GObject* object
) {
  MaplibreNativeFfiPremultipliedRgba8Image* self =
    MAPLIBRE_NATIVE_FFI_PREMULTIPLIED_RGBA8_IMAGE(object);
  g_clear_pointer(&self->pixels, g_bytes_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_premultiplied_rgba8_image_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_premultiplied_rgba8_image_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiPremultipliedRgba8Image* self =
    MAPLIBRE_NATIVE_FFI_PREMULTIPLIED_RGBA8_IMAGE(object);
  switch (id) {
    case PREMULTIPLIED_RGBA8_IMAGE_PROP_WIDTH:
      g_value_set_uint(value, self->width);
      break;
    case PREMULTIPLIED_RGBA8_IMAGE_PROP_HEIGHT:
      g_value_set_uint(value, self->height);
      break;
    case PREMULTIPLIED_RGBA8_IMAGE_PROP_STRIDE:
      g_value_set_uint(value, self->stride);
      break;
    case PREMULTIPLIED_RGBA8_IMAGE_PROP_PIXELS:
      g_value_set_boxed(value, self->pixels);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_premultiplied_rgba8_image_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiPremultipliedRgba8Image* self =
    MAPLIBRE_NATIVE_FFI_PREMULTIPLIED_RGBA8_IMAGE(object);
  switch (id) {
    case PREMULTIPLIED_RGBA8_IMAGE_PROP_WIDTH:
      maplibre_native_ffi_premultiplied_rgba8_image_set_width(
        self, g_value_get_uint(value)
      );
      break;
    case PREMULTIPLIED_RGBA8_IMAGE_PROP_HEIGHT:
      maplibre_native_ffi_premultiplied_rgba8_image_set_height(
        self, g_value_get_uint(value)
      );
      break;
    case PREMULTIPLIED_RGBA8_IMAGE_PROP_STRIDE:
      maplibre_native_ffi_premultiplied_rgba8_image_set_stride(
        self, g_value_get_uint(value)
      );
      break;
    case PREMULTIPLIED_RGBA8_IMAGE_PROP_PIXELS:
      maplibre_native_ffi_premultiplied_rgba8_image_set_pixels(
        self, g_value_get_boxed(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_premultiplied_rgba8_image_class_init(
  MaplibreNativeFfiPremultipliedRgba8ImageClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_premultiplied_rgba8_image_finalize;
  object_class->get_property =
    maplibre_native_ffi_premultiplied_rgba8_image_get_property;
  object_class->set_property =
    maplibre_native_ffi_premultiplied_rgba8_image_set_property;
  premultiplied_rgba8_image_properties[PREMULTIPLIED_RGBA8_IMAGE_PROP_WIDTH] =
    g_param_spec_uint("width", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS);
  premultiplied_rgba8_image_properties[PREMULTIPLIED_RGBA8_IMAGE_PROP_HEIGHT] =
    g_param_spec_uint("height", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS);
  premultiplied_rgba8_image_properties[PREMULTIPLIED_RGBA8_IMAGE_PROP_STRIDE] =
    g_param_spec_uint("stride", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS);
  premultiplied_rgba8_image_properties[PREMULTIPLIED_RGBA8_IMAGE_PROP_PIXELS] =
    g_param_spec_boxed("pixels", NULL, NULL, G_TYPE_BYTES, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, PREMULTIPLIED_RGBA8_IMAGE_N_PROPERTIES,
    premultiplied_rgba8_image_properties
  );
}

static void maplibre_native_ffi_premultiplied_rgba8_image_init(
  MaplibreNativeFfiPremultipliedRgba8Image* self
) {
  (void)self;
}

MaplibreNativeFfiPremultipliedRgba8Image*
maplibre_native_ffi_premultiplied_rgba8_image_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_PREMULTIPLIED_RGBA8_IMAGE, NULL);
}

guint32 maplibre_native_ffi_premultiplied_rgba8_image_get_width(
  MaplibreNativeFfiPremultipliedRgba8Image* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_PREMULTIPLIED_RGBA8_IMAGE(self), 0
  );
  return self->width;
}

void maplibre_native_ffi_premultiplied_rgba8_image_set_width(
  MaplibreNativeFfiPremultipliedRgba8Image* self, guint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PREMULTIPLIED_RGBA8_IMAGE(self));
  gboolean changed = self->width != value;
  self->width = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      premultiplied_rgba8_image_properties[PREMULTIPLIED_RGBA8_IMAGE_PROP_WIDTH]
    );
}

guint32 maplibre_native_ffi_premultiplied_rgba8_image_get_height(
  MaplibreNativeFfiPremultipliedRgba8Image* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_PREMULTIPLIED_RGBA8_IMAGE(self), 0
  );
  return self->height;
}

void maplibre_native_ffi_premultiplied_rgba8_image_set_height(
  MaplibreNativeFfiPremultipliedRgba8Image* self, guint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PREMULTIPLIED_RGBA8_IMAGE(self));
  gboolean changed = self->height != value;
  self->height = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), premultiplied_rgba8_image_properties
                        [PREMULTIPLIED_RGBA8_IMAGE_PROP_HEIGHT]
    );
}

guint32 maplibre_native_ffi_premultiplied_rgba8_image_get_stride(
  MaplibreNativeFfiPremultipliedRgba8Image* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_PREMULTIPLIED_RGBA8_IMAGE(self), 0
  );
  return self->stride;
}

void maplibre_native_ffi_premultiplied_rgba8_image_set_stride(
  MaplibreNativeFfiPremultipliedRgba8Image* self, guint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PREMULTIPLIED_RGBA8_IMAGE(self));
  gboolean changed = self->stride != value;
  self->stride = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), premultiplied_rgba8_image_properties
                        [PREMULTIPLIED_RGBA8_IMAGE_PROP_STRIDE]
    );
}

GBytes* maplibre_native_ffi_premultiplied_rgba8_image_get_pixels(
  MaplibreNativeFfiPremultipliedRgba8Image* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_PREMULTIPLIED_RGBA8_IMAGE(self), NULL
  );
  return self->pixels;
}

void maplibre_native_ffi_premultiplied_rgba8_image_set_pixels(
  MaplibreNativeFfiPremultipliedRgba8Image* self, GBytes* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PREMULTIPLIED_RGBA8_IMAGE(self));
  GBytes* previous = self->pixels;
  self->pixels = value != NULL ? g_bytes_ref(value) : NULL;
  gboolean changed = !maplibre_native_ffi_bytes_equal(previous, self->pixels);
  if (previous != NULL) g_bytes_unref(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), premultiplied_rgba8_image_properties
                        [PREMULTIPLIED_RGBA8_IMAGE_PROP_PIXELS]
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiProjectedMeters) == sizeof(mln_projected_meters)
);
SAME_MEMBER(MaplibreNativeFfiProjectedMeters, mln_projected_meters, northing);
SAME_MEMBER(MaplibreNativeFfiProjectedMeters, mln_projected_meters, easting);

void maplibre_native_ffi_projected_meters_to_native(
  const MaplibreNativeFfiProjectedMeters* self, mln_projected_meters* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

void maplibre_native_ffi_projected_meters_read(
  MaplibreNativeFfiProjectedMeters* out, const mln_projected_meters* in
) {
  memcpy(out, in, sizeof *out);
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
  GObject parent_instance;
  guint64 fields;
  gboolean axonometric;
  gdouble x_skew;
  gdouble y_skew;
};

enum {
  PROJECTION_MODE_OPTIONS_PROP_0,
  PROJECTION_MODE_OPTIONS_PROP_AXONOMETRIC,
  PROJECTION_MODE_OPTIONS_PROP_X_SKEW,
  PROJECTION_MODE_OPTIONS_PROP_Y_SKEW,
  PROJECTION_MODE_OPTIONS_N_PROPERTIES
};

static GParamSpec*
  projection_mode_options_properties[PROJECTION_MODE_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiProjectionModeOptions,
  maplibre_native_ffi_projection_mode_options, G_TYPE_OBJECT
)

static void maplibre_native_ffi_projection_mode_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiProjectionModeOptions* self =
    MAPLIBRE_NATIVE_FFI_PROJECTION_MODE_OPTIONS(object);
  switch (id) {
    case PROJECTION_MODE_OPTIONS_PROP_AXONOMETRIC:
      g_value_set_boolean(value, self->axonometric);
      break;
    case PROJECTION_MODE_OPTIONS_PROP_X_SKEW:
      g_value_set_double(value, self->x_skew);
      break;
    case PROJECTION_MODE_OPTIONS_PROP_Y_SKEW:
      g_value_set_double(value, self->y_skew);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_projection_mode_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiProjectionModeOptions* self =
    MAPLIBRE_NATIVE_FFI_PROJECTION_MODE_OPTIONS(object);
  switch (id) {
    case PROJECTION_MODE_OPTIONS_PROP_AXONOMETRIC:
      maplibre_native_ffi_projection_mode_options_set_axonometric(
        self, g_value_get_boolean(value)
      );
      break;
    case PROJECTION_MODE_OPTIONS_PROP_X_SKEW:
      maplibre_native_ffi_projection_mode_options_set_x_skew(
        self, g_value_get_double(value)
      );
      break;
    case PROJECTION_MODE_OPTIONS_PROP_Y_SKEW:
      maplibre_native_ffi_projection_mode_options_set_y_skew(
        self, g_value_get_double(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_projection_mode_options_class_init(
  MaplibreNativeFfiProjectionModeOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_projection_mode_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_projection_mode_options_set_property;
  projection_mode_options_properties[PROJECTION_MODE_OPTIONS_PROP_AXONOMETRIC] =
    g_param_spec_boolean("axonometric", NULL, NULL, FALSE, PROPERTY_FLAGS);
  projection_mode_options_properties[PROJECTION_MODE_OPTIONS_PROP_X_SKEW] =
    g_param_spec_double(
      "x-skew", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  projection_mode_options_properties[PROJECTION_MODE_OPTIONS_PROP_Y_SKEW] =
    g_param_spec_double(
      "y-skew", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, PROJECTION_MODE_OPTIONS_N_PROPERTIES,
    projection_mode_options_properties
  );
}

static void maplibre_native_ffi_projection_mode_options_init(
  MaplibreNativeFfiProjectionModeOptions* self
) {
  (void)self;
}

MaplibreNativeFfiProjectionModeOptions*
maplibre_native_ffi_projection_mode_options_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_PROJECTION_MODE_OPTIONS, NULL);
}

gboolean maplibre_native_ffi_projection_mode_options_get_axonometric(
  MaplibreNativeFfiProjectionModeOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_PROJECTION_MODE_OPTIONS(self), FALSE
  );
  return self->axonometric;
}

gboolean maplibre_native_ffi_projection_mode_options_has_axonometric(
  MaplibreNativeFfiProjectionModeOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_PROJECTION_MODE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_PROJECTION_MODE_OPTION_AXONOMETRIC) != 0;
}

void maplibre_native_ffi_projection_mode_options_set_axonometric(
  MaplibreNativeFfiProjectionModeOptions* self, gboolean value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PROJECTION_MODE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_PROJECTION_MODE_OPTION_AXONOMETRIC) != 0;
  gboolean changed = self->axonometric != value;
  self->axonometric = value;
  self->fields |= MLN_PROJECTION_MODE_OPTION_AXONOMETRIC;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), projection_mode_options_properties
                        [PROJECTION_MODE_OPTIONS_PROP_AXONOMETRIC]
    );
}

gdouble maplibre_native_ffi_projection_mode_options_get_x_skew(
  MaplibreNativeFfiProjectionModeOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_PROJECTION_MODE_OPTIONS(self), 0);
  return self->x_skew;
}

gboolean maplibre_native_ffi_projection_mode_options_has_x_skew(
  MaplibreNativeFfiProjectionModeOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_PROJECTION_MODE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_PROJECTION_MODE_OPTION_X_SKEW) != 0;
}

void maplibre_native_ffi_projection_mode_options_set_x_skew(
  MaplibreNativeFfiProjectionModeOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PROJECTION_MODE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_PROJECTION_MODE_OPTION_X_SKEW) != 0;
  gboolean changed = self->x_skew != value;
  self->x_skew = value;
  self->fields |= MLN_PROJECTION_MODE_OPTION_X_SKEW;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      projection_mode_options_properties[PROJECTION_MODE_OPTIONS_PROP_X_SKEW]
    );
}

gdouble maplibre_native_ffi_projection_mode_options_get_y_skew(
  MaplibreNativeFfiProjectionModeOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_PROJECTION_MODE_OPTIONS(self), 0);
  return self->y_skew;
}

gboolean maplibre_native_ffi_projection_mode_options_has_y_skew(
  MaplibreNativeFfiProjectionModeOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_PROJECTION_MODE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_PROJECTION_MODE_OPTION_Y_SKEW) != 0;
}

void maplibre_native_ffi_projection_mode_options_set_y_skew(
  MaplibreNativeFfiProjectionModeOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PROJECTION_MODE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_PROJECTION_MODE_OPTION_Y_SKEW) != 0;
  gboolean changed = self->y_skew != value;
  self->y_skew = value;
  self->fields |= MLN_PROJECTION_MODE_OPTION_Y_SKEW;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      projection_mode_options_properties[PROJECTION_MODE_OPTIONS_PROP_Y_SKEW]
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

struct _MaplibreNativeFfiRenderSessionAttachOptions {
  GObject parent_instance;
  MaplibreNativeFfiRenderDriverKind driver;
  MaplibreNativeFfiGpuSyncKindFlag producer_sync_kinds;
  MaplibreNativeFfiWake* frame_wake;
  MaplibreNativeFfiWake* redraw_wake;
  MaplibreNativeFfiWake* driver_work_wake;
  MaplibreNativeFfiQueueLock* queue_lock;
};

enum {
  RENDER_SESSION_ATTACH_OPTIONS_PROP_0,
  RENDER_SESSION_ATTACH_OPTIONS_PROP_DRIVER,
  RENDER_SESSION_ATTACH_OPTIONS_PROP_PRODUCER_SYNC_KINDS,
  RENDER_SESSION_ATTACH_OPTIONS_PROP_FRAME_WAKE,
  RENDER_SESSION_ATTACH_OPTIONS_PROP_REDRAW_WAKE,
  RENDER_SESSION_ATTACH_OPTIONS_PROP_DRIVER_WORK_WAKE,
  RENDER_SESSION_ATTACH_OPTIONS_PROP_QUEUE_LOCK,
  RENDER_SESSION_ATTACH_OPTIONS_N_PROPERTIES
};

static GParamSpec* render_session_attach_options_properties
  [RENDER_SESSION_ATTACH_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiRenderSessionAttachOptions,
  maplibre_native_ffi_render_session_attach_options, G_TYPE_OBJECT
)

static void maplibre_native_ffi_render_session_attach_options_finalize(
  GObject* object
) {
  MaplibreNativeFfiRenderSessionAttachOptions* self =
    MAPLIBRE_NATIVE_FFI_RENDER_SESSION_ATTACH_OPTIONS(object);
  g_clear_pointer(&self->frame_wake, maplibre_native_ffi_wake_free);
  g_clear_pointer(&self->redraw_wake, maplibre_native_ffi_wake_free);
  g_clear_pointer(&self->driver_work_wake, maplibre_native_ffi_wake_free);
  g_clear_pointer(&self->queue_lock, maplibre_native_ffi_queue_lock_free);
  G_OBJECT_CLASS(maplibre_native_ffi_render_session_attach_options_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_render_session_attach_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiRenderSessionAttachOptions* self =
    MAPLIBRE_NATIVE_FFI_RENDER_SESSION_ATTACH_OPTIONS(object);
  switch (id) {
    case RENDER_SESSION_ATTACH_OPTIONS_PROP_DRIVER:
      g_value_set_enum(value, self->driver);
      break;
    case RENDER_SESSION_ATTACH_OPTIONS_PROP_PRODUCER_SYNC_KINDS:
      g_value_set_flags(value, self->producer_sync_kinds);
      break;
    case RENDER_SESSION_ATTACH_OPTIONS_PROP_FRAME_WAKE:
      g_value_set_boxed(value, self->frame_wake);
      break;
    case RENDER_SESSION_ATTACH_OPTIONS_PROP_REDRAW_WAKE:
      g_value_set_boxed(value, self->redraw_wake);
      break;
    case RENDER_SESSION_ATTACH_OPTIONS_PROP_DRIVER_WORK_WAKE:
      g_value_set_boxed(value, self->driver_work_wake);
      break;
    case RENDER_SESSION_ATTACH_OPTIONS_PROP_QUEUE_LOCK:
      g_value_set_boxed(value, self->queue_lock);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_render_session_attach_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiRenderSessionAttachOptions* self =
    MAPLIBRE_NATIVE_FFI_RENDER_SESSION_ATTACH_OPTIONS(object);
  switch (id) {
    case RENDER_SESSION_ATTACH_OPTIONS_PROP_DRIVER:
      maplibre_native_ffi_render_session_attach_options_set_driver(
        self, g_value_get_enum(value)
      );
      break;
    case RENDER_SESSION_ATTACH_OPTIONS_PROP_PRODUCER_SYNC_KINDS:
      maplibre_native_ffi_render_session_attach_options_set_producer_sync_kinds(
        self, g_value_get_flags(value)
      );
      break;
    case RENDER_SESSION_ATTACH_OPTIONS_PROP_FRAME_WAKE:
      maplibre_native_ffi_render_session_attach_options_set_frame_wake(
        self, g_value_get_boxed(value)
      );
      break;
    case RENDER_SESSION_ATTACH_OPTIONS_PROP_REDRAW_WAKE:
      maplibre_native_ffi_render_session_attach_options_set_redraw_wake(
        self, g_value_get_boxed(value)
      );
      break;
    case RENDER_SESSION_ATTACH_OPTIONS_PROP_DRIVER_WORK_WAKE:
      maplibre_native_ffi_render_session_attach_options_set_driver_work_wake(
        self, g_value_get_boxed(value)
      );
      break;
    case RENDER_SESSION_ATTACH_OPTIONS_PROP_QUEUE_LOCK:
      maplibre_native_ffi_render_session_attach_options_set_queue_lock(
        self, g_value_get_boxed(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_render_session_attach_options_class_init(
  MaplibreNativeFfiRenderSessionAttachOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_render_session_attach_options_finalize;
  object_class->get_property =
    maplibre_native_ffi_render_session_attach_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_render_session_attach_options_set_property;
  render_session_attach_options_properties
    [RENDER_SESSION_ATTACH_OPTIONS_PROP_DRIVER] = g_param_spec_enum(
      "driver", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_RENDER_DRIVER_KIND,
      MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CORE_WORKER, PROPERTY_FLAGS
    );
  render_session_attach_options_properties
    [RENDER_SESSION_ATTACH_OPTIONS_PROP_PRODUCER_SYNC_KINDS] =
      g_param_spec_flags(
        "producer-sync-kinds", NULL, NULL,
        MAPLIBRE_NATIVE_FFI_TYPE_GPU_SYNC_KIND_FLAG, 0, PROPERTY_FLAGS
      );
  render_session_attach_options_properties
    [RENDER_SESSION_ATTACH_OPTIONS_PROP_FRAME_WAKE] = g_param_spec_boxed(
      "frame-wake", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_WAKE, PROPERTY_FLAGS
    );
  render_session_attach_options_properties
    [RENDER_SESSION_ATTACH_OPTIONS_PROP_REDRAW_WAKE] = g_param_spec_boxed(
      "redraw-wake", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_WAKE, PROPERTY_FLAGS
    );
  render_session_attach_options_properties
    [RENDER_SESSION_ATTACH_OPTIONS_PROP_DRIVER_WORK_WAKE] = g_param_spec_boxed(
      "driver-work-wake", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_WAKE,
      PROPERTY_FLAGS
    );
  render_session_attach_options_properties
    [RENDER_SESSION_ATTACH_OPTIONS_PROP_QUEUE_LOCK] = g_param_spec_boxed(
      "queue-lock", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_QUEUE_LOCK,
      PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, RENDER_SESSION_ATTACH_OPTIONS_N_PROPERTIES,
    render_session_attach_options_properties
  );
}

static void maplibre_native_ffi_render_session_attach_options_init(
  MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  self->frame_wake = maplibre_native_ffi_wake_new();
  self->redraw_wake = maplibre_native_ffi_wake_new();
  self->driver_work_wake = maplibre_native_ffi_wake_new();
  self->queue_lock = maplibre_native_ffi_queue_lock_new();
}

MaplibreNativeFfiRenderSessionAttachOptions*
maplibre_native_ffi_render_session_attach_options_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_RENDER_SESSION_ATTACH_OPTIONS, NULL
  );
}

MaplibreNativeFfiRenderDriverKind
maplibre_native_ffi_render_session_attach_options_get_driver(
  MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(self),
    (MaplibreNativeFfiRenderDriverKind)0
  );
  return self->driver;
}

void maplibre_native_ffi_render_session_attach_options_set_driver(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  MaplibreNativeFfiRenderDriverKind value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(self));
  gboolean changed = self->driver != value;
  self->driver = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), render_session_attach_options_properties
                        [RENDER_SESSION_ATTACH_OPTIONS_PROP_DRIVER]
    );
}

MaplibreNativeFfiGpuSyncKindFlag
maplibre_native_ffi_render_session_attach_options_get_producer_sync_kinds(
  MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(self),
    (MaplibreNativeFfiGpuSyncKindFlag)0
  );
  return self->producer_sync_kinds;
}

void maplibre_native_ffi_render_session_attach_options_set_producer_sync_kinds(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  MaplibreNativeFfiGpuSyncKindFlag value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(self));
  gboolean changed = self->producer_sync_kinds != value;
  self->producer_sync_kinds = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), render_session_attach_options_properties
                        [RENDER_SESSION_ATTACH_OPTIONS_PROP_PRODUCER_SYNC_KINDS]
    );
}

MaplibreNativeFfiWake*
maplibre_native_ffi_render_session_attach_options_get_frame_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(self), NULL
  );
  return self->frame_wake;
}

void maplibre_native_ffi_render_session_attach_options_set_frame_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiWake* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(self));
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiWake* previous = self->frame_wake;
  self->frame_wake = maplibre_native_ffi_wake_copy(value);
  if (previous != NULL) maplibre_native_ffi_wake_free(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), render_session_attach_options_properties
                      [RENDER_SESSION_ATTACH_OPTIONS_PROP_FRAME_WAKE]
  );
}

MaplibreNativeFfiWake*
maplibre_native_ffi_render_session_attach_options_get_redraw_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(self), NULL
  );
  return self->redraw_wake;
}

void maplibre_native_ffi_render_session_attach_options_set_redraw_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiWake* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(self));
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiWake* previous = self->redraw_wake;
  self->redraw_wake = maplibre_native_ffi_wake_copy(value);
  if (previous != NULL) maplibre_native_ffi_wake_free(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), render_session_attach_options_properties
                      [RENDER_SESSION_ATTACH_OPTIONS_PROP_REDRAW_WAKE]
  );
}

MaplibreNativeFfiWake*
maplibre_native_ffi_render_session_attach_options_get_driver_work_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(self), NULL
  );
  return self->driver_work_wake;
}

void maplibre_native_ffi_render_session_attach_options_set_driver_work_wake(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiWake* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(self));
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiWake* previous = self->driver_work_wake;
  self->driver_work_wake = maplibre_native_ffi_wake_copy(value);
  if (previous != NULL) maplibre_native_ffi_wake_free(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), render_session_attach_options_properties
                      [RENDER_SESSION_ATTACH_OPTIONS_PROP_DRIVER_WORK_WAKE]
  );
}

MaplibreNativeFfiQueueLock*
maplibre_native_ffi_render_session_attach_options_get_queue_lock(
  MaplibreNativeFfiRenderSessionAttachOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(self), NULL
  );
  return self->queue_lock;
}

void maplibre_native_ffi_render_session_attach_options_set_queue_lock(
  MaplibreNativeFfiRenderSessionAttachOptions* self,
  const MaplibreNativeFfiQueueLock* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(self));
  g_return_if_fail(value != NULL);
  MaplibreNativeFfiQueueLock* previous = self->queue_lock;
  self->queue_lock = maplibre_native_ffi_queue_lock_copy(value);
  if (previous != NULL) maplibre_native_ffi_queue_lock_free(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), render_session_attach_options_properties
                      [RENDER_SESSION_ATTACH_OPTIONS_PROP_QUEUE_LOCK]
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
  GObject parent_instance;
  guint64 fields;
  char** layer_ids;
  char* filter;
};

enum {
  RENDERED_FEATURE_QUERY_OPTIONS_PROP_0,
  RENDERED_FEATURE_QUERY_OPTIONS_PROP_LAYER_IDS,
  RENDERED_FEATURE_QUERY_OPTIONS_PROP_FILTER,
  RENDERED_FEATURE_QUERY_OPTIONS_N_PROPERTIES
};

static GParamSpec* rendered_feature_query_options_properties
  [RENDERED_FEATURE_QUERY_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiRenderedFeatureQueryOptions,
  maplibre_native_ffi_rendered_feature_query_options, G_TYPE_OBJECT
)

static void maplibre_native_ffi_rendered_feature_query_options_finalize(
  GObject* object
) {
  MaplibreNativeFfiRenderedFeatureQueryOptions* self =
    MAPLIBRE_NATIVE_FFI_RENDERED_FEATURE_QUERY_OPTIONS(object);
  g_strfreev(self->layer_ids);
  g_free(self->filter);
  G_OBJECT_CLASS(
    maplibre_native_ffi_rendered_feature_query_options_parent_class
  )
    ->finalize(object);
}

static void maplibre_native_ffi_rendered_feature_query_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiRenderedFeatureQueryOptions* self =
    MAPLIBRE_NATIVE_FFI_RENDERED_FEATURE_QUERY_OPTIONS(object);
  switch (id) {
    case RENDERED_FEATURE_QUERY_OPTIONS_PROP_LAYER_IDS:
      g_value_set_boxed(value, self->layer_ids);
      break;
    case RENDERED_FEATURE_QUERY_OPTIONS_PROP_FILTER:
      g_value_set_string(value, self->filter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_rendered_feature_query_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiRenderedFeatureQueryOptions* self =
    MAPLIBRE_NATIVE_FFI_RENDERED_FEATURE_QUERY_OPTIONS(object);
  switch (id) {
    case RENDERED_FEATURE_QUERY_OPTIONS_PROP_LAYER_IDS:
      maplibre_native_ffi_rendered_feature_query_options_set_layer_ids(
        self, g_value_get_boxed(value)
      );
      break;
    case RENDERED_FEATURE_QUERY_OPTIONS_PROP_FILTER:
      maplibre_native_ffi_rendered_feature_query_options_set_filter(
        self, g_value_get_string(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_rendered_feature_query_options_class_init(
  MaplibreNativeFfiRenderedFeatureQueryOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_rendered_feature_query_options_finalize;
  object_class->get_property =
    maplibre_native_ffi_rendered_feature_query_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_rendered_feature_query_options_set_property;
  rendered_feature_query_options_properties
    [RENDERED_FEATURE_QUERY_OPTIONS_PROP_LAYER_IDS] =
      g_param_spec_boxed("layer-ids", NULL, NULL, G_TYPE_STRV, PROPERTY_FLAGS);
  rendered_feature_query_options_properties
    [RENDERED_FEATURE_QUERY_OPTIONS_PROP_FILTER] =
      g_param_spec_string("filter", NULL, NULL, "", PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, RENDERED_FEATURE_QUERY_OPTIONS_N_PROPERTIES,
    rendered_feature_query_options_properties
  );
}

static void maplibre_native_ffi_rendered_feature_query_options_init(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self
) {
  self->layer_ids = g_new0(char*, 1);
  self->filter = g_strdup("");
}

MaplibreNativeFfiRenderedFeatureQueryOptions*
maplibre_native_ffi_rendered_feature_query_options_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_RENDERED_FEATURE_QUERY_OPTIONS, NULL
  );
}

const char* const*
maplibre_native_ffi_rendered_feature_query_options_get_layer_ids(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDERED_FEATURE_QUERY_OPTIONS(self), NULL
  );
  return (const char* const*)self->layer_ids;
}

gboolean maplibre_native_ffi_rendered_feature_query_options_has_layer_ids(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDERED_FEATURE_QUERY_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS) != 0;
}

void maplibre_native_ffi_rendered_feature_query_options_set_layer_ids(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self, const char* const* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDERED_FEATURE_QUERY_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS) != 0;
  char** previous = self->layer_ids;
  self->layer_ids = value != NULL ? g_strdupv((char**)value) : g_new0(char*, 1);
  gboolean changed = !maplibre_native_ffi_strv_equal(previous, self->layer_ids);
  g_strfreev(previous);
  self->fields |= MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), rendered_feature_query_options_properties
                        [RENDERED_FEATURE_QUERY_OPTIONS_PROP_LAYER_IDS]
    );
}

const char* maplibre_native_ffi_rendered_feature_query_options_get_filter(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDERED_FEATURE_QUERY_OPTIONS(self), NULL
  );
  return self->filter;
}

void maplibre_native_ffi_rendered_feature_query_options_set_filter(
  MaplibreNativeFfiRenderedFeatureQueryOptions* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDERED_FEATURE_QUERY_OPTIONS(self));
  char* previous = self->filter;
  self->filter = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->filter) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), rendered_feature_query_options_properties
                        [RENDERED_FEATURE_QUERY_OPTIONS_PROP_FILTER]
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

G_STATIC_ASSERT(sizeof(MaplibreNativeFfiScreenBox) == sizeof(mln_screen_box));
SAME_MEMBER(MaplibreNativeFfiScreenBox, mln_screen_box, min);
SAME_MEMBER(MaplibreNativeFfiScreenBox, mln_screen_box, max);

void maplibre_native_ffi_screen_box_to_native(
  const MaplibreNativeFfiScreenBox* self, mln_screen_box* out,
  MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

struct _MaplibreNativeFfiScreenLineString {
  GObject parent_instance;
  GPtrArray* points;
};

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiScreenLineString, maplibre_native_ffi_screen_line_string,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_screen_line_string_finalize(GObject* object) {
  MaplibreNativeFfiScreenLineString* self =
    MAPLIBRE_NATIVE_FFI_SCREEN_LINE_STRING(object);
  g_clear_pointer(&self->points, g_ptr_array_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_screen_line_string_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_screen_line_string_class_init(
  MaplibreNativeFfiScreenLineStringClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_screen_line_string_finalize;
}

static void maplibre_native_ffi_screen_line_string_init(
  MaplibreNativeFfiScreenLineString* self
) {
  self->points = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_screen_point_free
  );
}

MaplibreNativeFfiScreenLineString* maplibre_native_ffi_screen_line_string_new(
  void
) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_SCREEN_LINE_STRING, NULL);
}

GPtrArray* maplibre_native_ffi_screen_line_string_get_points(
  MaplibreNativeFfiScreenLineString* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_SCREEN_LINE_STRING(self), NULL);
  return self->points;
}

void maplibre_native_ffi_screen_line_string_set_points(
  MaplibreNativeFfiScreenLineString* self,
  const MaplibreNativeFfiScreenPoint* value, gsize value_count
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_SCREEN_LINE_STRING(self));
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

MaplibreNativeFfiScreenLineString* maplibre_native_ffi_screen_line_string_copy(
  const MaplibreNativeFfiScreenLineString* self
) {
  MaplibreNativeFfiScreenLineString* copy =
    maplibre_native_ffi_screen_line_string_new();
  g_clear_pointer(&copy->points, g_ptr_array_unref);
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
  g_clear_object(&self->line_string);
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
  MaplibreNativeFfiScreenLineString* value
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_SCREEN_LINE_STRING(value), NULL);
  MaplibreNativeFfiRenderedQueryGeometry* self =
    g_new0(MaplibreNativeFfiRenderedQueryGeometry, 1);
  self->tag = MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING;
  self->line_string = maplibre_native_ffi_screen_line_string_new();
  MaplibreNativeFfiScreenLineString* previous = self->line_string;
  self->line_string = maplibre_native_ffi_screen_line_string_copy(value);
  if (previous != NULL) g_object_unref(previous);
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiRenderingStats) == sizeof(mln_rendering_stats)
);
SAME_MEMBER(
  MaplibreNativeFfiRenderingStats, mln_rendering_stats, encoding_time_ms
);
SAME_MEMBER(
  MaplibreNativeFfiRenderingStats, mln_rendering_stats, rendering_time_ms
);
SAME_MEMBER(MaplibreNativeFfiRenderingStats, mln_rendering_stats, frame_count);
SAME_MEMBER(
  MaplibreNativeFfiRenderingStats, mln_rendering_stats, draw_call_count
);
SAME_MEMBER(
  MaplibreNativeFfiRenderingStats, mln_rendering_stats, total_draw_call_count
);

void maplibre_native_ffi_rendering_stats_read(
  MaplibreNativeFfiRenderingStats* out, const mln_rendering_stats* in
) {
  memcpy(out, in, sizeof *out);
}

MaplibreNativeFfiRenderingStats*
maplibre_native_ffi_rendering_stats_from_native(const mln_rendering_stats* in) {
  MaplibreNativeFfiRenderingStats* self =
    g_new0(MaplibreNativeFfiRenderingStats, 1);
  maplibre_native_ffi_rendering_stats_read(self, in);
  return self;
}

struct _MaplibreNativeFfiResourceRoute {
  GObject parent_instance;
  MaplibreNativeFfiResourceKindFlag kinds;
  MaplibreNativeFfiUrlMatchFlag flags;
  char* url;
};

enum {
  RESOURCE_ROUTE_PROP_0,
  RESOURCE_ROUTE_PROP_KINDS,
  RESOURCE_ROUTE_PROP_FLAGS,
  RESOURCE_ROUTE_PROP_URL,
  RESOURCE_ROUTE_N_PROPERTIES
};

static GParamSpec* resource_route_properties[RESOURCE_ROUTE_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiResourceRoute, maplibre_native_ffi_resource_route,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_resource_route_finalize(GObject* object) {
  MaplibreNativeFfiResourceRoute* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_ROUTE(object);
  g_free(self->url);
  G_OBJECT_CLASS(maplibre_native_ffi_resource_route_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_resource_route_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiResourceRoute* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_ROUTE(object);
  switch (id) {
    case RESOURCE_ROUTE_PROP_KINDS:
      g_value_set_flags(value, self->kinds);
      break;
    case RESOURCE_ROUTE_PROP_FLAGS:
      g_value_set_flags(value, self->flags);
      break;
    case RESOURCE_ROUTE_PROP_URL:
      g_value_set_string(value, self->url);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_resource_route_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiResourceRoute* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_ROUTE(object);
  switch (id) {
    case RESOURCE_ROUTE_PROP_KINDS:
      maplibre_native_ffi_resource_route_set_kinds(
        self, g_value_get_flags(value)
      );
      break;
    case RESOURCE_ROUTE_PROP_FLAGS:
      maplibre_native_ffi_resource_route_set_flags(
        self, g_value_get_flags(value)
      );
      break;
    case RESOURCE_ROUTE_PROP_URL:
      maplibre_native_ffi_resource_route_set_url(
        self, g_value_get_string(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_resource_route_class_init(
  MaplibreNativeFfiResourceRouteClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_resource_route_finalize;
  object_class->get_property = maplibre_native_ffi_resource_route_get_property;
  object_class->set_property = maplibre_native_ffi_resource_route_set_property;
  resource_route_properties[RESOURCE_ROUTE_PROP_KINDS] = g_param_spec_flags(
    "kinds", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_KIND_FLAG, 0,
    PROPERTY_FLAGS
  );
  resource_route_properties[RESOURCE_ROUTE_PROP_FLAGS] = g_param_spec_flags(
    "flags", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_URL_MATCH_FLAG, 0,
    PROPERTY_FLAGS
  );
  resource_route_properties[RESOURCE_ROUTE_PROP_URL] =
    g_param_spec_string("url", NULL, NULL, "", PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, RESOURCE_ROUTE_N_PROPERTIES, resource_route_properties
  );
}

static void maplibre_native_ffi_resource_route_init(
  MaplibreNativeFfiResourceRoute* self
) {
  self->url = g_strdup("");
}

MaplibreNativeFfiResourceRoute* maplibre_native_ffi_resource_route_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_ROUTE, NULL);
}

MaplibreNativeFfiResourceKindFlag maplibre_native_ffi_resource_route_get_kinds(
  MaplibreNativeFfiResourceRoute* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_ROUTE(self),
    (MaplibreNativeFfiResourceKindFlag)0
  );
  return self->kinds;
}

void maplibre_native_ffi_resource_route_set_kinds(
  MaplibreNativeFfiResourceRoute* self, MaplibreNativeFfiResourceKindFlag value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_ROUTE(self));
  gboolean changed = self->kinds != value;
  self->kinds = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), resource_route_properties[RESOURCE_ROUTE_PROP_KINDS]
    );
}

MaplibreNativeFfiUrlMatchFlag maplibre_native_ffi_resource_route_get_flags(
  MaplibreNativeFfiResourceRoute* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_ROUTE(self),
    (MaplibreNativeFfiUrlMatchFlag)0
  );
  return self->flags;
}

void maplibre_native_ffi_resource_route_set_flags(
  MaplibreNativeFfiResourceRoute* self, MaplibreNativeFfiUrlMatchFlag value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_ROUTE(self));
  gboolean changed = self->flags != value;
  self->flags = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), resource_route_properties[RESOURCE_ROUTE_PROP_FLAGS]
    );
}

const char* maplibre_native_ffi_resource_route_get_url(
  MaplibreNativeFfiResourceRoute* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_ROUTE(self), NULL);
  return self->url;
}

void maplibre_native_ffi_resource_route_set_url(
  MaplibreNativeFfiResourceRoute* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_ROUTE(self));
  char* previous = self->url;
  self->url = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->url) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), resource_route_properties[RESOURCE_ROUTE_PROP_URL]
    );
}

MaplibreNativeFfiResourceRoute* maplibre_native_ffi_resource_route_copy(
  const MaplibreNativeFfiResourceRoute* self
) {
  MaplibreNativeFfiResourceRoute* copy =
    maplibre_native_ffi_resource_route_new();
  copy->kinds = self->kinds;
  copy->flags = self->flags;
  g_free(copy->url);
  copy->url = g_strdup(self->url);
  return copy;
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
  self->routes = g_ptr_array_new_with_free_func((GDestroyNotify)g_object_unref);
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
    g_ptr_array_set_free_func(copy->routes, (GDestroyNotify)g_object_unref);
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
  for (gsize index = 0; index < value_count; index++) {
    g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_ROUTE(value[index]));
  }
  GPtrArray* previous = self->routes;
  self->routes =
    g_ptr_array_new_full((guint)value_count, (GDestroyNotify)g_object_unref);
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiResourceRange) == sizeof(mln_resource_range)
);
SAME_MEMBER(MaplibreNativeFfiResourceRange, mln_resource_range, start);
SAME_MEMBER(MaplibreNativeFfiResourceRange, mln_resource_range, end);

void maplibre_native_ffi_resource_range_read(
  MaplibreNativeFfiResourceRange* out, const mln_resource_range* in
) {
  memcpy(out, in, sizeof *out);
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
  GObject parent_instance;
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

enum {
  RESOURCE_RESPONSE_PROP_0,
  RESOURCE_RESPONSE_PROP_STATUS,
  RESOURCE_RESPONSE_PROP_ERROR_REASON,
  RESOURCE_RESPONSE_PROP_BYTES,
  RESOURCE_RESPONSE_PROP_ERROR_MESSAGE,
  RESOURCE_RESPONSE_PROP_MUST_REVALIDATE,
  RESOURCE_RESPONSE_PROP_MODIFIED_UNIX_MS,
  RESOURCE_RESPONSE_PROP_EXPIRES_UNIX_MS,
  RESOURCE_RESPONSE_PROP_ETAG,
  RESOURCE_RESPONSE_PROP_RETRY_AFTER_UNIX_MS,
  RESOURCE_RESPONSE_N_PROPERTIES
};

static GParamSpec* resource_response_properties[RESOURCE_RESPONSE_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiResourceResponse, maplibre_native_ffi_resource_response,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_resource_response_finalize(GObject* object) {
  MaplibreNativeFfiResourceResponse* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE(object);
  g_clear_pointer(&self->bytes, g_bytes_unref);
  g_free(self->error_message);
  g_free(self->etag);
  G_OBJECT_CLASS(maplibre_native_ffi_resource_response_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_resource_response_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiResourceResponse* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE(object);
  switch (id) {
    case RESOURCE_RESPONSE_PROP_STATUS:
      g_value_set_enum(value, self->status);
      break;
    case RESOURCE_RESPONSE_PROP_ERROR_REASON:
      g_value_set_enum(value, self->error_reason);
      break;
    case RESOURCE_RESPONSE_PROP_BYTES:
      g_value_set_boxed(value, self->bytes);
      break;
    case RESOURCE_RESPONSE_PROP_ERROR_MESSAGE:
      g_value_set_string(value, self->error_message);
      break;
    case RESOURCE_RESPONSE_PROP_MUST_REVALIDATE:
      g_value_set_boolean(value, self->must_revalidate);
      break;
    case RESOURCE_RESPONSE_PROP_MODIFIED_UNIX_MS:
      g_value_set_int64(value, self->modified_unix_ms);
      break;
    case RESOURCE_RESPONSE_PROP_EXPIRES_UNIX_MS:
      g_value_set_int64(value, self->expires_unix_ms);
      break;
    case RESOURCE_RESPONSE_PROP_ETAG:
      g_value_set_string(value, self->etag);
      break;
    case RESOURCE_RESPONSE_PROP_RETRY_AFTER_UNIX_MS:
      g_value_set_int64(value, self->retry_after_unix_ms);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_resource_response_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiResourceResponse* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE(object);
  switch (id) {
    case RESOURCE_RESPONSE_PROP_STATUS:
      maplibre_native_ffi_resource_response_set_status(
        self, g_value_get_enum(value)
      );
      break;
    case RESOURCE_RESPONSE_PROP_ERROR_REASON:
      maplibre_native_ffi_resource_response_set_error_reason(
        self, g_value_get_enum(value)
      );
      break;
    case RESOURCE_RESPONSE_PROP_BYTES:
      maplibre_native_ffi_resource_response_set_bytes(
        self, g_value_get_boxed(value)
      );
      break;
    case RESOURCE_RESPONSE_PROP_ERROR_MESSAGE:
      maplibre_native_ffi_resource_response_set_error_message(
        self, g_value_get_string(value)
      );
      break;
    case RESOURCE_RESPONSE_PROP_MUST_REVALIDATE:
      maplibre_native_ffi_resource_response_set_must_revalidate(
        self, g_value_get_boolean(value)
      );
      break;
    case RESOURCE_RESPONSE_PROP_MODIFIED_UNIX_MS:
      maplibre_native_ffi_resource_response_set_modified_unix_ms(
        self, g_value_get_int64(value)
      );
      break;
    case RESOURCE_RESPONSE_PROP_EXPIRES_UNIX_MS:
      maplibre_native_ffi_resource_response_set_expires_unix_ms(
        self, g_value_get_int64(value)
      );
      break;
    case RESOURCE_RESPONSE_PROP_ETAG:
      maplibre_native_ffi_resource_response_set_etag(
        self, g_value_get_string(value)
      );
      break;
    case RESOURCE_RESPONSE_PROP_RETRY_AFTER_UNIX_MS:
      maplibre_native_ffi_resource_response_set_retry_after_unix_ms(
        self, g_value_get_int64(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_resource_response_class_init(
  MaplibreNativeFfiResourceResponseClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_resource_response_finalize;
  object_class->get_property =
    maplibre_native_ffi_resource_response_get_property;
  object_class->set_property =
    maplibre_native_ffi_resource_response_set_property;
  resource_response_properties[RESOURCE_RESPONSE_PROP_STATUS] =
    g_param_spec_enum(
      "status", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_RESPONSE_STATUS,
      0, PROPERTY_FLAGS
    );
  resource_response_properties[RESOURCE_RESPONSE_PROP_ERROR_REASON] =
    g_param_spec_enum(
      "error-reason", NULL, NULL,
      MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_ERROR_REASON, 0, PROPERTY_FLAGS
    );
  resource_response_properties[RESOURCE_RESPONSE_PROP_BYTES] =
    g_param_spec_boxed("bytes", NULL, NULL, G_TYPE_BYTES, PROPERTY_FLAGS);
  resource_response_properties[RESOURCE_RESPONSE_PROP_ERROR_MESSAGE] =
    g_param_spec_string("error-message", NULL, NULL, "", PROPERTY_FLAGS);
  resource_response_properties[RESOURCE_RESPONSE_PROP_MUST_REVALIDATE] =
    g_param_spec_boolean("must-revalidate", NULL, NULL, FALSE, PROPERTY_FLAGS);
  resource_response_properties[RESOURCE_RESPONSE_PROP_MODIFIED_UNIX_MS] =
    g_param_spec_int64(
      "modified-unix-ms", NULL, NULL, G_MININT64, G_MAXINT64, 0, PROPERTY_FLAGS
    );
  resource_response_properties[RESOURCE_RESPONSE_PROP_EXPIRES_UNIX_MS] =
    g_param_spec_int64(
      "expires-unix-ms", NULL, NULL, G_MININT64, G_MAXINT64, 0, PROPERTY_FLAGS
    );
  resource_response_properties[RESOURCE_RESPONSE_PROP_ETAG] =
    g_param_spec_string("etag", NULL, NULL, "", PROPERTY_FLAGS);
  resource_response_properties[RESOURCE_RESPONSE_PROP_RETRY_AFTER_UNIX_MS] =
    g_param_spec_int64(
      "retry-after-unix-ms", NULL, NULL, G_MININT64, G_MAXINT64, 0,
      PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, RESOURCE_RESPONSE_N_PROPERTIES, resource_response_properties
  );
}

static void maplibre_native_ffi_resource_response_init(
  MaplibreNativeFfiResourceResponse* self
) {
  self->error_message = g_strdup("");
  self->etag = g_strdup("");
}

MaplibreNativeFfiResourceResponse* maplibre_native_ffi_resource_response_new(
  void
) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_RESPONSE, NULL);
}

MaplibreNativeFfiResourceResponseStatus
maplibre_native_ffi_resource_response_get_status(
  MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self),
    (MaplibreNativeFfiResourceResponseStatus)0
  );
  return self->status;
}

void maplibre_native_ffi_resource_response_set_status(
  MaplibreNativeFfiResourceResponse* self,
  MaplibreNativeFfiResourceResponseStatus value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self));
  gboolean changed = self->status != value;
  self->status = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      resource_response_properties[RESOURCE_RESPONSE_PROP_STATUS]
    );
}

MaplibreNativeFfiResourceErrorReason
maplibre_native_ffi_resource_response_get_error_reason(
  MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self),
    (MaplibreNativeFfiResourceErrorReason)0
  );
  return self->error_reason;
}

void maplibre_native_ffi_resource_response_set_error_reason(
  MaplibreNativeFfiResourceResponse* self,
  MaplibreNativeFfiResourceErrorReason value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self));
  gboolean changed = self->error_reason != value;
  self->error_reason = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      resource_response_properties[RESOURCE_RESPONSE_PROP_ERROR_REASON]
    );
}

GBytes* maplibre_native_ffi_resource_response_get_bytes(
  MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self), NULL);
  return self->bytes;
}

void maplibre_native_ffi_resource_response_set_bytes(
  MaplibreNativeFfiResourceResponse* self, GBytes* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self));
  GBytes* previous = self->bytes;
  self->bytes = value != NULL ? g_bytes_ref(value) : NULL;
  gboolean changed = !maplibre_native_ffi_bytes_equal(previous, self->bytes);
  if (previous != NULL) g_bytes_unref(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), resource_response_properties[RESOURCE_RESPONSE_PROP_BYTES]
    );
}

const char* maplibre_native_ffi_resource_response_get_error_message(
  MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self), NULL);
  return self->error_message;
}

void maplibre_native_ffi_resource_response_set_error_message(
  MaplibreNativeFfiResourceResponse* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self));
  char* previous = self->error_message;
  self->error_message = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->error_message) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      resource_response_properties[RESOURCE_RESPONSE_PROP_ERROR_MESSAGE]
    );
}

gboolean maplibre_native_ffi_resource_response_get_must_revalidate(
  MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self), FALSE);
  return self->must_revalidate;
}

void maplibre_native_ffi_resource_response_set_must_revalidate(
  MaplibreNativeFfiResourceResponse* self, gboolean value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self));
  gboolean changed = self->must_revalidate != value;
  self->must_revalidate = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      resource_response_properties[RESOURCE_RESPONSE_PROP_MUST_REVALIDATE]
    );
}

gint64 maplibre_native_ffi_resource_response_get_modified_unix_ms(
  MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self), 0);
  return self->modified_unix_ms;
}

gboolean maplibre_native_ffi_resource_response_has_modified_unix_ms(
  MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self), FALSE);
  return (self->fields & MLN_RESOURCE_RESPONSE_MODIFIED) != 0;
}

void maplibre_native_ffi_resource_response_set_modified_unix_ms(
  MaplibreNativeFfiResourceResponse* self, gint64 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self));
  gboolean was_set = (self->fields & MLN_RESOURCE_RESPONSE_MODIFIED) != 0;
  gboolean changed = self->modified_unix_ms != value;
  self->modified_unix_ms = value;
  self->fields |= MLN_RESOURCE_RESPONSE_MODIFIED;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      resource_response_properties[RESOURCE_RESPONSE_PROP_MODIFIED_UNIX_MS]
    );
}

gint64 maplibre_native_ffi_resource_response_get_expires_unix_ms(
  MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self), 0);
  return self->expires_unix_ms;
}

gboolean maplibre_native_ffi_resource_response_has_expires_unix_ms(
  MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self), FALSE);
  return (self->fields & MLN_RESOURCE_RESPONSE_EXPIRES) != 0;
}

void maplibre_native_ffi_resource_response_set_expires_unix_ms(
  MaplibreNativeFfiResourceResponse* self, gint64 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self));
  gboolean was_set = (self->fields & MLN_RESOURCE_RESPONSE_EXPIRES) != 0;
  gboolean changed = self->expires_unix_ms != value;
  self->expires_unix_ms = value;
  self->fields |= MLN_RESOURCE_RESPONSE_EXPIRES;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      resource_response_properties[RESOURCE_RESPONSE_PROP_EXPIRES_UNIX_MS]
    );
}

const char* maplibre_native_ffi_resource_response_get_etag(
  MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self), NULL);
  return self->etag;
}

void maplibre_native_ffi_resource_response_set_etag(
  MaplibreNativeFfiResourceResponse* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self));
  char* previous = self->etag;
  self->etag = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->etag) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), resource_response_properties[RESOURCE_RESPONSE_PROP_ETAG]
    );
}

gint64 maplibre_native_ffi_resource_response_get_retry_after_unix_ms(
  MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self), 0);
  return self->retry_after_unix_ms;
}

gboolean maplibre_native_ffi_resource_response_has_retry_after_unix_ms(
  MaplibreNativeFfiResourceResponse* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self), FALSE);
  return (self->fields & MLN_RESOURCE_RESPONSE_RETRY_AFTER) != 0;
}

void maplibre_native_ffi_resource_response_set_retry_after_unix_ms(
  MaplibreNativeFfiResourceResponse* self, gint64 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(self));
  gboolean was_set = (self->fields & MLN_RESOURCE_RESPONSE_RETRY_AFTER) != 0;
  gboolean changed = self->retry_after_unix_ms != value;
  self->retry_after_unix_ms = value;
  self->fields |= MLN_RESOURCE_RESPONSE_RETRY_AFTER;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      resource_response_properties[RESOURCE_RESPONSE_PROP_RETRY_AFTER_UNIX_MS]
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

struct _MaplibreNativeFfiUrlQueryParameter {
  GObject parent_instance;
  char* name;
  char* value;
};

enum {
  URL_QUERY_PARAMETER_PROP_0,
  URL_QUERY_PARAMETER_PROP_NAME,
  URL_QUERY_PARAMETER_PROP_VALUE,
  URL_QUERY_PARAMETER_N_PROPERTIES
};

static GParamSpec*
  url_query_parameter_properties[URL_QUERY_PARAMETER_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiUrlQueryParameter, maplibre_native_ffi_url_query_parameter,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_url_query_parameter_finalize(GObject* object) {
  MaplibreNativeFfiUrlQueryParameter* self =
    MAPLIBRE_NATIVE_FFI_URL_QUERY_PARAMETER(object);
  g_free(self->name);
  g_free(self->value);
  G_OBJECT_CLASS(maplibre_native_ffi_url_query_parameter_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_url_query_parameter_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiUrlQueryParameter* self =
    MAPLIBRE_NATIVE_FFI_URL_QUERY_PARAMETER(object);
  switch (id) {
    case URL_QUERY_PARAMETER_PROP_NAME:
      g_value_set_string(value, self->name);
      break;
    case URL_QUERY_PARAMETER_PROP_VALUE:
      g_value_set_string(value, self->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_url_query_parameter_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiUrlQueryParameter* self =
    MAPLIBRE_NATIVE_FFI_URL_QUERY_PARAMETER(object);
  switch (id) {
    case URL_QUERY_PARAMETER_PROP_NAME:
      maplibre_native_ffi_url_query_parameter_set_name(
        self, g_value_get_string(value)
      );
      break;
    case URL_QUERY_PARAMETER_PROP_VALUE:
      maplibre_native_ffi_url_query_parameter_set_value(
        self, g_value_get_string(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_url_query_parameter_class_init(
  MaplibreNativeFfiUrlQueryParameterClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_url_query_parameter_finalize;
  object_class->get_property =
    maplibre_native_ffi_url_query_parameter_get_property;
  object_class->set_property =
    maplibre_native_ffi_url_query_parameter_set_property;
  url_query_parameter_properties[URL_QUERY_PARAMETER_PROP_NAME] =
    g_param_spec_string("name", NULL, NULL, "", PROPERTY_FLAGS);
  url_query_parameter_properties[URL_QUERY_PARAMETER_PROP_VALUE] =
    g_param_spec_string("value", NULL, NULL, "", PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, URL_QUERY_PARAMETER_N_PROPERTIES,
    url_query_parameter_properties
  );
}

static void maplibre_native_ffi_url_query_parameter_init(
  MaplibreNativeFfiUrlQueryParameter* self
) {
  self->name = g_strdup("");
  self->value = g_strdup("");
}

MaplibreNativeFfiUrlQueryParameter* maplibre_native_ffi_url_query_parameter_new(
  void
) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_URL_QUERY_PARAMETER, NULL);
}

const char* maplibre_native_ffi_url_query_parameter_get_name(
  MaplibreNativeFfiUrlQueryParameter* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_URL_QUERY_PARAMETER(self), NULL);
  return self->name;
}

void maplibre_native_ffi_url_query_parameter_set_name(
  MaplibreNativeFfiUrlQueryParameter* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_URL_QUERY_PARAMETER(self));
  char* previous = self->name;
  self->name = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->name) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      url_query_parameter_properties[URL_QUERY_PARAMETER_PROP_NAME]
    );
}

const char* maplibre_native_ffi_url_query_parameter_get_value(
  MaplibreNativeFfiUrlQueryParameter* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_URL_QUERY_PARAMETER(self), NULL);
  return self->value;
}

void maplibre_native_ffi_url_query_parameter_set_value(
  MaplibreNativeFfiUrlQueryParameter* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_URL_QUERY_PARAMETER(self));
  char* previous = self->value;
  self->value = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->value) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      url_query_parameter_properties[URL_QUERY_PARAMETER_PROP_VALUE]
    );
}

MaplibreNativeFfiUrlQueryParameter*
maplibre_native_ffi_url_query_parameter_copy(
  const MaplibreNativeFfiUrlQueryParameter* self
) {
  MaplibreNativeFfiUrlQueryParameter* copy =
    maplibre_native_ffi_url_query_parameter_new();
  g_free(copy->name);
  copy->name = g_strdup(self->name);
  g_free(copy->value);
  copy->value = g_strdup(self->value);
  return copy;
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

struct _MaplibreNativeFfiResourceTransformRule {
  GObject parent_instance;
  MaplibreNativeFfiResourceKindFlag kinds;
  MaplibreNativeFfiUrlMatchFlag flags;
  char* url;
  GPtrArray* parameters;
};

enum {
  RESOURCE_TRANSFORM_RULE_PROP_0,
  RESOURCE_TRANSFORM_RULE_PROP_KINDS,
  RESOURCE_TRANSFORM_RULE_PROP_FLAGS,
  RESOURCE_TRANSFORM_RULE_PROP_URL,
  RESOURCE_TRANSFORM_RULE_N_PROPERTIES
};

static GParamSpec*
  resource_transform_rule_properties[RESOURCE_TRANSFORM_RULE_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiResourceTransformRule,
  maplibre_native_ffi_resource_transform_rule, G_TYPE_OBJECT
)

static void maplibre_native_ffi_resource_transform_rule_finalize(
  GObject* object
) {
  MaplibreNativeFfiResourceTransformRule* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_TRANSFORM_RULE(object);
  g_free(self->url);
  g_clear_pointer(&self->parameters, g_ptr_array_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_resource_transform_rule_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_resource_transform_rule_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiResourceTransformRule* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_TRANSFORM_RULE(object);
  switch (id) {
    case RESOURCE_TRANSFORM_RULE_PROP_KINDS:
      g_value_set_flags(value, self->kinds);
      break;
    case RESOURCE_TRANSFORM_RULE_PROP_FLAGS:
      g_value_set_flags(value, self->flags);
      break;
    case RESOURCE_TRANSFORM_RULE_PROP_URL:
      g_value_set_string(value, self->url);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_resource_transform_rule_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiResourceTransformRule* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_TRANSFORM_RULE(object);
  switch (id) {
    case RESOURCE_TRANSFORM_RULE_PROP_KINDS:
      maplibre_native_ffi_resource_transform_rule_set_kinds(
        self, g_value_get_flags(value)
      );
      break;
    case RESOURCE_TRANSFORM_RULE_PROP_FLAGS:
      maplibre_native_ffi_resource_transform_rule_set_flags(
        self, g_value_get_flags(value)
      );
      break;
    case RESOURCE_TRANSFORM_RULE_PROP_URL:
      maplibre_native_ffi_resource_transform_rule_set_url(
        self, g_value_get_string(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_resource_transform_rule_class_init(
  MaplibreNativeFfiResourceTransformRuleClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_resource_transform_rule_finalize;
  object_class->get_property =
    maplibre_native_ffi_resource_transform_rule_get_property;
  object_class->set_property =
    maplibre_native_ffi_resource_transform_rule_set_property;
  resource_transform_rule_properties[RESOURCE_TRANSFORM_RULE_PROP_KINDS] =
    g_param_spec_flags(
      "kinds", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_KIND_FLAG, 0,
      PROPERTY_FLAGS
    );
  resource_transform_rule_properties[RESOURCE_TRANSFORM_RULE_PROP_FLAGS] =
    g_param_spec_flags(
      "flags", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_URL_MATCH_FLAG, 0,
      PROPERTY_FLAGS
    );
  resource_transform_rule_properties[RESOURCE_TRANSFORM_RULE_PROP_URL] =
    g_param_spec_string("url", NULL, NULL, "", PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, RESOURCE_TRANSFORM_RULE_N_PROPERTIES,
    resource_transform_rule_properties
  );
}

static void maplibre_native_ffi_resource_transform_rule_init(
  MaplibreNativeFfiResourceTransformRule* self
) {
  self->url = g_strdup("");
  self->parameters =
    g_ptr_array_new_with_free_func((GDestroyNotify)g_object_unref);
}

MaplibreNativeFfiResourceTransformRule*
maplibre_native_ffi_resource_transform_rule_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_TRANSFORM_RULE, NULL);
}

MaplibreNativeFfiResourceKindFlag
maplibre_native_ffi_resource_transform_rule_get_kinds(
  MaplibreNativeFfiResourceTransformRule* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_RULE(self),
    (MaplibreNativeFfiResourceKindFlag)0
  );
  return self->kinds;
}

void maplibre_native_ffi_resource_transform_rule_set_kinds(
  MaplibreNativeFfiResourceTransformRule* self,
  MaplibreNativeFfiResourceKindFlag value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_RULE(self));
  gboolean changed = self->kinds != value;
  self->kinds = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      resource_transform_rule_properties[RESOURCE_TRANSFORM_RULE_PROP_KINDS]
    );
}

MaplibreNativeFfiUrlMatchFlag
maplibre_native_ffi_resource_transform_rule_get_flags(
  MaplibreNativeFfiResourceTransformRule* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_RULE(self),
    (MaplibreNativeFfiUrlMatchFlag)0
  );
  return self->flags;
}

void maplibre_native_ffi_resource_transform_rule_set_flags(
  MaplibreNativeFfiResourceTransformRule* self,
  MaplibreNativeFfiUrlMatchFlag value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_RULE(self));
  gboolean changed = self->flags != value;
  self->flags = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      resource_transform_rule_properties[RESOURCE_TRANSFORM_RULE_PROP_FLAGS]
    );
}

const char* maplibre_native_ffi_resource_transform_rule_get_url(
  MaplibreNativeFfiResourceTransformRule* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_RULE(self), NULL
  );
  return self->url;
}

void maplibre_native_ffi_resource_transform_rule_set_url(
  MaplibreNativeFfiResourceTransformRule* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_RULE(self));
  char* previous = self->url;
  self->url = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->url) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      resource_transform_rule_properties[RESOURCE_TRANSFORM_RULE_PROP_URL]
    );
}

GPtrArray* maplibre_native_ffi_resource_transform_rule_get_parameters(
  MaplibreNativeFfiResourceTransformRule* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_RULE(self), NULL
  );
  return self->parameters;
}

void maplibre_native_ffi_resource_transform_rule_set_parameters(
  MaplibreNativeFfiResourceTransformRule* self,
  MaplibreNativeFfiUrlQueryParameter* const* value, gsize value_count
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_RULE(self));
  for (gsize index = 0; index < value_count; index++) {
    g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_URL_QUERY_PARAMETER(value[index]));
  }
  GPtrArray* previous = self->parameters;
  self->parameters =
    g_ptr_array_new_full((guint)value_count, (GDestroyNotify)g_object_unref);
  for (gsize index = 0; index < value_count; index++) {
    g_ptr_array_add(
      self->parameters,
      maplibre_native_ffi_url_query_parameter_copy(value[index])
    );
  }
  if (previous != NULL) g_ptr_array_unref(previous);
}

MaplibreNativeFfiResourceTransformRule*
maplibre_native_ffi_resource_transform_rule_copy(
  const MaplibreNativeFfiResourceTransformRule* self
) {
  MaplibreNativeFfiResourceTransformRule* copy =
    maplibre_native_ffi_resource_transform_rule_new();
  copy->kinds = self->kinds;
  copy->flags = self->flags;
  g_free(copy->url);
  copy->url = g_strdup(self->url);
  g_clear_pointer(&copy->parameters, g_ptr_array_unref);
  copy->parameters =
    self->parameters != NULL
      ? g_ptr_array_copy(
          self->parameters, maplibre_native_ffi_url_query_parameter_copy_func,
          NULL
        )
      : NULL;
  if (copy->parameters != NULL)
    g_ptr_array_set_free_func(copy->parameters, (GDestroyNotify)g_object_unref);
  return copy;
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
  self->rules = g_ptr_array_new_with_free_func((GDestroyNotify)g_object_unref);
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
    g_ptr_array_set_free_func(copy->rules, (GDestroyNotify)g_object_unref);
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
  for (gsize index = 0; index < value_count; index++) {
    g_return_if_fail(
      MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_RULE(value[index])
    );
  }
  GPtrArray* previous = self->rules;
  self->rules =
    g_ptr_array_new_full((guint)value_count, (GDestroyNotify)g_object_unref);
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

struct _MaplibreNativeFfiResourceTransformError {
  GObject parent_instance;
  guint64 fields;
  MaplibreNativeFfiResourceErrorReason reason;
  char* message;
  gint64 retry_after_unix_ms;
};

enum {
  RESOURCE_TRANSFORM_ERROR_PROP_0,
  RESOURCE_TRANSFORM_ERROR_PROP_REASON,
  RESOURCE_TRANSFORM_ERROR_PROP_MESSAGE,
  RESOURCE_TRANSFORM_ERROR_PROP_RETRY_AFTER_UNIX_MS,
  RESOURCE_TRANSFORM_ERROR_N_PROPERTIES
};

static GParamSpec*
  resource_transform_error_properties[RESOURCE_TRANSFORM_ERROR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiResourceTransformError,
  maplibre_native_ffi_resource_transform_error, G_TYPE_OBJECT
)

static void maplibre_native_ffi_resource_transform_error_finalize(
  GObject* object
) {
  MaplibreNativeFfiResourceTransformError* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_TRANSFORM_ERROR(object);
  g_free(self->message);
  G_OBJECT_CLASS(maplibre_native_ffi_resource_transform_error_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_resource_transform_error_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiResourceTransformError* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_TRANSFORM_ERROR(object);
  switch (id) {
    case RESOURCE_TRANSFORM_ERROR_PROP_REASON:
      g_value_set_enum(value, self->reason);
      break;
    case RESOURCE_TRANSFORM_ERROR_PROP_MESSAGE:
      g_value_set_string(value, self->message);
      break;
    case RESOURCE_TRANSFORM_ERROR_PROP_RETRY_AFTER_UNIX_MS:
      g_value_set_int64(value, self->retry_after_unix_ms);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_resource_transform_error_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiResourceTransformError* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_TRANSFORM_ERROR(object);
  switch (id) {
    case RESOURCE_TRANSFORM_ERROR_PROP_REASON:
      maplibre_native_ffi_resource_transform_error_set_reason(
        self, g_value_get_enum(value)
      );
      break;
    case RESOURCE_TRANSFORM_ERROR_PROP_MESSAGE:
      maplibre_native_ffi_resource_transform_error_set_message(
        self, g_value_get_string(value)
      );
      break;
    case RESOURCE_TRANSFORM_ERROR_PROP_RETRY_AFTER_UNIX_MS:
      maplibre_native_ffi_resource_transform_error_set_retry_after_unix_ms(
        self, g_value_get_int64(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_resource_transform_error_class_init(
  MaplibreNativeFfiResourceTransformErrorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_resource_transform_error_finalize;
  object_class->get_property =
    maplibre_native_ffi_resource_transform_error_get_property;
  object_class->set_property =
    maplibre_native_ffi_resource_transform_error_set_property;
  resource_transform_error_properties[RESOURCE_TRANSFORM_ERROR_PROP_REASON] =
    g_param_spec_enum(
      "reason", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_ERROR_REASON, 0,
      PROPERTY_FLAGS
    );
  resource_transform_error_properties[RESOURCE_TRANSFORM_ERROR_PROP_MESSAGE] =
    g_param_spec_string("message", NULL, NULL, "", PROPERTY_FLAGS);
  resource_transform_error_properties
    [RESOURCE_TRANSFORM_ERROR_PROP_RETRY_AFTER_UNIX_MS] = g_param_spec_int64(
      "retry-after-unix-ms", NULL, NULL, G_MININT64, G_MAXINT64, 0,
      PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, RESOURCE_TRANSFORM_ERROR_N_PROPERTIES,
    resource_transform_error_properties
  );
}

static void maplibre_native_ffi_resource_transform_error_init(
  MaplibreNativeFfiResourceTransformError* self
) {
  self->message = g_strdup("");
}

MaplibreNativeFfiResourceTransformError*
maplibre_native_ffi_resource_transform_error_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_TRANSFORM_ERROR, NULL);
}

MaplibreNativeFfiResourceErrorReason
maplibre_native_ffi_resource_transform_error_get_reason(
  MaplibreNativeFfiResourceTransformError* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_ERROR(self),
    (MaplibreNativeFfiResourceErrorReason)0
  );
  return self->reason;
}

void maplibre_native_ffi_resource_transform_error_set_reason(
  MaplibreNativeFfiResourceTransformError* self,
  MaplibreNativeFfiResourceErrorReason value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_ERROR(self));
  gboolean changed = self->reason != value;
  self->reason = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      resource_transform_error_properties[RESOURCE_TRANSFORM_ERROR_PROP_REASON]
    );
}

const char* maplibre_native_ffi_resource_transform_error_get_message(
  MaplibreNativeFfiResourceTransformError* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_ERROR(self), NULL
  );
  return self->message;
}

void maplibre_native_ffi_resource_transform_error_set_message(
  MaplibreNativeFfiResourceTransformError* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_ERROR(self));
  char* previous = self->message;
  self->message = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->message) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      resource_transform_error_properties[RESOURCE_TRANSFORM_ERROR_PROP_MESSAGE]
    );
}

gint64 maplibre_native_ffi_resource_transform_error_get_retry_after_unix_ms(
  MaplibreNativeFfiResourceTransformError* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_ERROR(self), 0
  );
  return self->retry_after_unix_ms;
}

gboolean maplibre_native_ffi_resource_transform_error_has_retry_after_unix_ms(
  MaplibreNativeFfiResourceTransformError* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_ERROR(self), FALSE
  );
  return (self->fields & MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER) != 0;
}

void maplibre_native_ffi_resource_transform_error_set_retry_after_unix_ms(
  MaplibreNativeFfiResourceTransformError* self, gint64 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_ERROR(self));
  gboolean was_set =
    (self->fields & MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER) != 0;
  gboolean changed = self->retry_after_unix_ms != value;
  self->retry_after_unix_ms = value;
  self->fields |= MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), resource_transform_error_properties
                        [RESOURCE_TRANSFORM_ERROR_PROP_RETRY_AFTER_UNIX_MS]
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiRuntimeEventCameraChange) ==
  sizeof(mln_runtime_event_camera_change)
);
SAME_MEMBER(
  MaplibreNativeFfiRuntimeEventCameraChange, mln_runtime_event_camera_change,
  mode
);
SAME_MEMBER(
  MaplibreNativeFfiRuntimeEventCameraChange, mln_runtime_event_camera_change,
  source
);

void maplibre_native_ffi_runtime_event_camera_change_read(
  MaplibreNativeFfiRuntimeEventCameraChange* out,
  const mln_runtime_event_camera_change* in
) {
  memcpy(out, in, sizeof *out);
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiRuntimeEventLoadingFailed) ==
  sizeof(mln_runtime_event_loading_failed)
);
SAME_MEMBER(
  MaplibreNativeFfiRuntimeEventLoadingFailed, mln_runtime_event_loading_failed,
  error
);

void maplibre_native_ffi_runtime_event_loading_failed_read(
  MaplibreNativeFfiRuntimeEventLoadingFailed* out,
  const mln_runtime_event_loading_failed* in
) {
  memcpy(out, in, sizeof *out);
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

G_STATIC_ASSERT(sizeof(MaplibreNativeFfiTileId) == sizeof(mln_tile_id));
SAME_MEMBER(MaplibreNativeFfiTileId, mln_tile_id, overscaled_z);
SAME_MEMBER(MaplibreNativeFfiTileId, mln_tile_id, wrap);
SAME_MEMBER(MaplibreNativeFfiTileId, mln_tile_id, canonical_z);
SAME_MEMBER(MaplibreNativeFfiTileId, mln_tile_id, canonical_x);
SAME_MEMBER(MaplibreNativeFfiTileId, mln_tile_id, canonical_y);

void maplibre_native_ffi_tile_id_read(
  MaplibreNativeFfiTileId* out, const mln_tile_id* in
) {
  memcpy(out, in, sizeof *out);
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiRuntimeEventOfflineRegionResponseError) ==
  sizeof(mln_runtime_event_offline_region_response_error)
);
SAME_MEMBER(
  MaplibreNativeFfiRuntimeEventOfflineRegionResponseError,
  mln_runtime_event_offline_region_response_error, region_id
);
SAME_MEMBER(
  MaplibreNativeFfiRuntimeEventOfflineRegionResponseError,
  mln_runtime_event_offline_region_response_error, reason
);

void maplibre_native_ffi_runtime_event_offline_region_response_error_read(
  MaplibreNativeFfiRuntimeEventOfflineRegionResponseError* out,
  const mln_runtime_event_offline_region_response_error* in
) {
  memcpy(out, in, sizeof *out);
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit) ==
  sizeof(mln_runtime_event_offline_region_tile_count_limit)
);
SAME_MEMBER(
  MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit,
  mln_runtime_event_offline_region_tile_count_limit, region_id
);
SAME_MEMBER(
  MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit,
  mln_runtime_event_offline_region_tile_count_limit, limit
);

void maplibre_native_ffi_runtime_event_offline_region_tile_count_limit_read(
  MaplibreNativeFfiRuntimeEventOfflineRegionTileCountLimit* out,
  const mln_runtime_event_offline_region_tile_count_limit* in
) {
  memcpy(out, in, sizeof *out);
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

struct _MaplibreNativeFfiRuntimeOptions {
  GObject parent_instance;
  char* asset_path;
  char* cache_path;
};

enum {
  RUNTIME_OPTIONS_PROP_0,
  RUNTIME_OPTIONS_PROP_ASSET_PATH,
  RUNTIME_OPTIONS_PROP_CACHE_PATH,
  RUNTIME_OPTIONS_N_PROPERTIES
};

static GParamSpec* runtime_options_properties[RUNTIME_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiRuntimeOptions, maplibre_native_ffi_runtime_options,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_runtime_options_finalize(GObject* object) {
  MaplibreNativeFfiRuntimeOptions* self =
    MAPLIBRE_NATIVE_FFI_RUNTIME_OPTIONS(object);
  g_free(self->asset_path);
  g_free(self->cache_path);
  G_OBJECT_CLASS(maplibre_native_ffi_runtime_options_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_runtime_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiRuntimeOptions* self =
    MAPLIBRE_NATIVE_FFI_RUNTIME_OPTIONS(object);
  switch (id) {
    case RUNTIME_OPTIONS_PROP_ASSET_PATH:
      g_value_set_string(value, self->asset_path);
      break;
    case RUNTIME_OPTIONS_PROP_CACHE_PATH:
      g_value_set_string(value, self->cache_path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_runtime_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiRuntimeOptions* self =
    MAPLIBRE_NATIVE_FFI_RUNTIME_OPTIONS(object);
  switch (id) {
    case RUNTIME_OPTIONS_PROP_ASSET_PATH:
      maplibre_native_ffi_runtime_options_set_asset_path(
        self, g_value_get_string(value)
      );
      break;
    case RUNTIME_OPTIONS_PROP_CACHE_PATH:
      maplibre_native_ffi_runtime_options_set_cache_path(
        self, g_value_get_string(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_runtime_options_class_init(
  MaplibreNativeFfiRuntimeOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_runtime_options_finalize;
  object_class->get_property = maplibre_native_ffi_runtime_options_get_property;
  object_class->set_property = maplibre_native_ffi_runtime_options_set_property;
  runtime_options_properties[RUNTIME_OPTIONS_PROP_ASSET_PATH] =
    g_param_spec_string("asset-path", NULL, NULL, "", PROPERTY_FLAGS);
  runtime_options_properties[RUNTIME_OPTIONS_PROP_CACHE_PATH] =
    g_param_spec_string("cache-path", NULL, NULL, "", PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, RUNTIME_OPTIONS_N_PROPERTIES, runtime_options_properties
  );
}

static void maplibre_native_ffi_runtime_options_init(
  MaplibreNativeFfiRuntimeOptions* self
) {
  self->asset_path = g_strdup("");
  self->cache_path = g_strdup("");
}

MaplibreNativeFfiRuntimeOptions* maplibre_native_ffi_runtime_options_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_OPTIONS, NULL);
}

const char* maplibre_native_ffi_runtime_options_get_asset_path(
  MaplibreNativeFfiRuntimeOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME_OPTIONS(self), NULL);
  return self->asset_path;
}

void maplibre_native_ffi_runtime_options_set_asset_path(
  MaplibreNativeFfiRuntimeOptions* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME_OPTIONS(self));
  char* previous = self->asset_path;
  self->asset_path = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->asset_path) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      runtime_options_properties[RUNTIME_OPTIONS_PROP_ASSET_PATH]
    );
}

const char* maplibre_native_ffi_runtime_options_get_cache_path(
  MaplibreNativeFfiRuntimeOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME_OPTIONS(self), NULL);
  return self->cache_path;
}

void maplibre_native_ffi_runtime_options_set_cache_path(
  MaplibreNativeFfiRuntimeOptions* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME_OPTIONS(self));
  char* previous = self->cache_path;
  self->cache_path = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->cache_path) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      runtime_options_properties[RUNTIME_OPTIONS_PROP_CACHE_PATH]
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
  GObject parent_instance;
  guint64 fields;
  char** source_layer_ids;
  char* filter;
};

enum {
  SOURCE_FEATURE_QUERY_OPTIONS_PROP_0,
  SOURCE_FEATURE_QUERY_OPTIONS_PROP_SOURCE_LAYER_IDS,
  SOURCE_FEATURE_QUERY_OPTIONS_PROP_FILTER,
  SOURCE_FEATURE_QUERY_OPTIONS_N_PROPERTIES
};

static GParamSpec* source_feature_query_options_properties
  [SOURCE_FEATURE_QUERY_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiSourceFeatureQueryOptions,
  maplibre_native_ffi_source_feature_query_options, G_TYPE_OBJECT
)

static void maplibre_native_ffi_source_feature_query_options_finalize(
  GObject* object
) {
  MaplibreNativeFfiSourceFeatureQueryOptions* self =
    MAPLIBRE_NATIVE_FFI_SOURCE_FEATURE_QUERY_OPTIONS(object);
  g_strfreev(self->source_layer_ids);
  g_free(self->filter);
  G_OBJECT_CLASS(maplibre_native_ffi_source_feature_query_options_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_source_feature_query_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiSourceFeatureQueryOptions* self =
    MAPLIBRE_NATIVE_FFI_SOURCE_FEATURE_QUERY_OPTIONS(object);
  switch (id) {
    case SOURCE_FEATURE_QUERY_OPTIONS_PROP_SOURCE_LAYER_IDS:
      g_value_set_boxed(value, self->source_layer_ids);
      break;
    case SOURCE_FEATURE_QUERY_OPTIONS_PROP_FILTER:
      g_value_set_string(value, self->filter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_source_feature_query_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiSourceFeatureQueryOptions* self =
    MAPLIBRE_NATIVE_FFI_SOURCE_FEATURE_QUERY_OPTIONS(object);
  switch (id) {
    case SOURCE_FEATURE_QUERY_OPTIONS_PROP_SOURCE_LAYER_IDS:
      maplibre_native_ffi_source_feature_query_options_set_source_layer_ids(
        self, g_value_get_boxed(value)
      );
      break;
    case SOURCE_FEATURE_QUERY_OPTIONS_PROP_FILTER:
      maplibre_native_ffi_source_feature_query_options_set_filter(
        self, g_value_get_string(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_source_feature_query_options_class_init(
  MaplibreNativeFfiSourceFeatureQueryOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_source_feature_query_options_finalize;
  object_class->get_property =
    maplibre_native_ffi_source_feature_query_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_source_feature_query_options_set_property;
  source_feature_query_options_properties
    [SOURCE_FEATURE_QUERY_OPTIONS_PROP_SOURCE_LAYER_IDS] = g_param_spec_boxed(
      "source-layer-ids", NULL, NULL, G_TYPE_STRV, PROPERTY_FLAGS
    );
  source_feature_query_options_properties
    [SOURCE_FEATURE_QUERY_OPTIONS_PROP_FILTER] =
      g_param_spec_string("filter", NULL, NULL, "", PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, SOURCE_FEATURE_QUERY_OPTIONS_N_PROPERTIES,
    source_feature_query_options_properties
  );
}

static void maplibre_native_ffi_source_feature_query_options_init(
  MaplibreNativeFfiSourceFeatureQueryOptions* self
) {
  self->source_layer_ids = g_new0(char*, 1);
  self->filter = g_strdup("");
}

MaplibreNativeFfiSourceFeatureQueryOptions*
maplibre_native_ffi_source_feature_query_options_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_SOURCE_FEATURE_QUERY_OPTIONS, NULL
  );
}

const char* const*
maplibre_native_ffi_source_feature_query_options_get_source_layer_ids(
  MaplibreNativeFfiSourceFeatureQueryOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_SOURCE_FEATURE_QUERY_OPTIONS(self), NULL
  );
  return (const char* const*)self->source_layer_ids;
}

gboolean maplibre_native_ffi_source_feature_query_options_has_source_layer_ids(
  MaplibreNativeFfiSourceFeatureQueryOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_SOURCE_FEATURE_QUERY_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS) != 0;
}

void maplibre_native_ffi_source_feature_query_options_set_source_layer_ids(
  MaplibreNativeFfiSourceFeatureQueryOptions* self, const char* const* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_SOURCE_FEATURE_QUERY_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS) != 0;
  char** previous = self->source_layer_ids;
  self->source_layer_ids =
    value != NULL ? g_strdupv((char**)value) : g_new0(char*, 1);
  gboolean changed =
    !maplibre_native_ffi_strv_equal(previous, self->source_layer_ids);
  g_strfreev(previous);
  self->fields |= MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), source_feature_query_options_properties
                        [SOURCE_FEATURE_QUERY_OPTIONS_PROP_SOURCE_LAYER_IDS]
    );
}

const char* maplibre_native_ffi_source_feature_query_options_get_filter(
  MaplibreNativeFfiSourceFeatureQueryOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_SOURCE_FEATURE_QUERY_OPTIONS(self), NULL
  );
  return self->filter;
}

void maplibre_native_ffi_source_feature_query_options_set_filter(
  MaplibreNativeFfiSourceFeatureQueryOptions* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_SOURCE_FEATURE_QUERY_OPTIONS(self));
  char* previous = self->filter;
  self->filter = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->filter) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), source_feature_query_options_properties
                        [SOURCE_FEATURE_QUERY_OPTIONS_PROP_FILTER]
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
  GObject parent_instance;
  guint64 fields;
  gdouble zoom;
  gdouble color_ramp_parameter;
  char* feature;
  char* feature_state;
  char* global_state;
  MaplibreNativeFfiCanonicalTileId canonical_tile_id;
  char** available_images;
};

enum {
  STYLE_EXPRESSION_INPUTS_PROP_0,
  STYLE_EXPRESSION_INPUTS_PROP_ZOOM,
  STYLE_EXPRESSION_INPUTS_PROP_COLOR_RAMP_PARAMETER,
  STYLE_EXPRESSION_INPUTS_PROP_FEATURE,
  STYLE_EXPRESSION_INPUTS_PROP_FEATURE_STATE,
  STYLE_EXPRESSION_INPUTS_PROP_GLOBAL_STATE,
  STYLE_EXPRESSION_INPUTS_PROP_CANONICAL_TILE_ID,
  STYLE_EXPRESSION_INPUTS_PROP_AVAILABLE_IMAGES,
  STYLE_EXPRESSION_INPUTS_N_PROPERTIES
};

static GParamSpec*
  style_expression_inputs_properties[STYLE_EXPRESSION_INPUTS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiStyleExpressionInputs,
  maplibre_native_ffi_style_expression_inputs, G_TYPE_OBJECT
)

static void maplibre_native_ffi_style_expression_inputs_finalize(
  GObject* object
) {
  MaplibreNativeFfiStyleExpressionInputs* self =
    MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_INPUTS(object);
  g_free(self->feature);
  g_free(self->feature_state);
  g_free(self->global_state);
  g_strfreev(self->available_images);
  G_OBJECT_CLASS(maplibre_native_ffi_style_expression_inputs_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_style_expression_inputs_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiStyleExpressionInputs* self =
    MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_INPUTS(object);
  switch (id) {
    case STYLE_EXPRESSION_INPUTS_PROP_ZOOM:
      g_value_set_double(value, self->zoom);
      break;
    case STYLE_EXPRESSION_INPUTS_PROP_COLOR_RAMP_PARAMETER:
      g_value_set_double(value, self->color_ramp_parameter);
      break;
    case STYLE_EXPRESSION_INPUTS_PROP_FEATURE:
      g_value_set_string(value, self->feature);
      break;
    case STYLE_EXPRESSION_INPUTS_PROP_FEATURE_STATE:
      g_value_set_string(value, self->feature_state);
      break;
    case STYLE_EXPRESSION_INPUTS_PROP_GLOBAL_STATE:
      g_value_set_string(value, self->global_state);
      break;
    case STYLE_EXPRESSION_INPUTS_PROP_CANONICAL_TILE_ID:
      g_value_set_boxed(value, &self->canonical_tile_id);
      break;
    case STYLE_EXPRESSION_INPUTS_PROP_AVAILABLE_IMAGES:
      g_value_set_boxed(value, self->available_images);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_style_expression_inputs_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiStyleExpressionInputs* self =
    MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_INPUTS(object);
  switch (id) {
    case STYLE_EXPRESSION_INPUTS_PROP_ZOOM:
      maplibre_native_ffi_style_expression_inputs_set_zoom(
        self, g_value_get_double(value)
      );
      break;
    case STYLE_EXPRESSION_INPUTS_PROP_COLOR_RAMP_PARAMETER:
      maplibre_native_ffi_style_expression_inputs_set_color_ramp_parameter(
        self, g_value_get_double(value)
      );
      break;
    case STYLE_EXPRESSION_INPUTS_PROP_FEATURE:
      maplibre_native_ffi_style_expression_inputs_set_feature(
        self, g_value_get_string(value)
      );
      break;
    case STYLE_EXPRESSION_INPUTS_PROP_FEATURE_STATE:
      maplibre_native_ffi_style_expression_inputs_set_feature_state(
        self, g_value_get_string(value)
      );
      break;
    case STYLE_EXPRESSION_INPUTS_PROP_GLOBAL_STATE:
      maplibre_native_ffi_style_expression_inputs_set_global_state(
        self, g_value_get_string(value)
      );
      break;
    case STYLE_EXPRESSION_INPUTS_PROP_CANONICAL_TILE_ID:
      maplibre_native_ffi_style_expression_inputs_set_canonical_tile_id(
        self, g_value_get_boxed(value)
      );
      break;
    case STYLE_EXPRESSION_INPUTS_PROP_AVAILABLE_IMAGES:
      maplibre_native_ffi_style_expression_inputs_set_available_images(
        self, g_value_get_boxed(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_style_expression_inputs_class_init(
  MaplibreNativeFfiStyleExpressionInputsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_style_expression_inputs_finalize;
  object_class->get_property =
    maplibre_native_ffi_style_expression_inputs_get_property;
  object_class->set_property =
    maplibre_native_ffi_style_expression_inputs_set_property;
  style_expression_inputs_properties[STYLE_EXPRESSION_INPUTS_PROP_ZOOM] =
    g_param_spec_double(
      "zoom", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  style_expression_inputs_properties
    [STYLE_EXPRESSION_INPUTS_PROP_COLOR_RAMP_PARAMETER] = g_param_spec_double(
      "color-ramp-parameter", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  style_expression_inputs_properties[STYLE_EXPRESSION_INPUTS_PROP_FEATURE] =
    g_param_spec_string("feature", NULL, NULL, "", PROPERTY_FLAGS);
  style_expression_inputs_properties
    [STYLE_EXPRESSION_INPUTS_PROP_FEATURE_STATE] =
      g_param_spec_string("feature-state", NULL, NULL, "", PROPERTY_FLAGS);
  style_expression_inputs_properties
    [STYLE_EXPRESSION_INPUTS_PROP_GLOBAL_STATE] =
      g_param_spec_string("global-state", NULL, NULL, "", PROPERTY_FLAGS);
  style_expression_inputs_properties
    [STYLE_EXPRESSION_INPUTS_PROP_CANONICAL_TILE_ID] = g_param_spec_boxed(
      "canonical-tile-id", NULL, NULL,
      MAPLIBRE_NATIVE_FFI_TYPE_CANONICAL_TILE_ID, PROPERTY_FLAGS
    );
  style_expression_inputs_properties
    [STYLE_EXPRESSION_INPUTS_PROP_AVAILABLE_IMAGES] = g_param_spec_boxed(
      "available-images", NULL, NULL, G_TYPE_STRV, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, STYLE_EXPRESSION_INPUTS_N_PROPERTIES,
    style_expression_inputs_properties
  );
}

static void maplibre_native_ffi_style_expression_inputs_init(
  MaplibreNativeFfiStyleExpressionInputs* self
) {
  self->feature = g_strdup("");
  self->feature_state = g_strdup("");
  self->global_state = g_strdup("");
  self->available_images = g_new0(char*, 1);
}

MaplibreNativeFfiStyleExpressionInputs*
maplibre_native_ffi_style_expression_inputs_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_STYLE_EXPRESSION_INPUTS, NULL);
}

gdouble maplibre_native_ffi_style_expression_inputs_get_zoom(
  MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self), 0);
  return self->zoom;
}

gboolean maplibre_native_ffi_style_expression_inputs_has_zoom(
  MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self), FALSE
  );
  return (self->fields & MLN_STYLE_EXPRESSION_INPUT_ZOOM) != 0;
}

void maplibre_native_ffi_style_expression_inputs_set_zoom(
  MaplibreNativeFfiStyleExpressionInputs* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self));
  gboolean was_set = (self->fields & MLN_STYLE_EXPRESSION_INPUT_ZOOM) != 0;
  gboolean changed = self->zoom != value;
  self->zoom = value;
  self->fields |= MLN_STYLE_EXPRESSION_INPUT_ZOOM;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      style_expression_inputs_properties[STYLE_EXPRESSION_INPUTS_PROP_ZOOM]
    );
}

gdouble maplibre_native_ffi_style_expression_inputs_get_color_ramp_parameter(
  MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self), 0);
  return self->color_ramp_parameter;
}

gboolean maplibre_native_ffi_style_expression_inputs_has_color_ramp_parameter(
  MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self), FALSE
  );
  return (self->fields & MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER) != 0;
}

void maplibre_native_ffi_style_expression_inputs_set_color_ramp_parameter(
  MaplibreNativeFfiStyleExpressionInputs* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self));
  gboolean was_set =
    (self->fields & MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER) != 0;
  gboolean changed = self->color_ramp_parameter != value;
  self->color_ramp_parameter = value;
  self->fields |= MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), style_expression_inputs_properties
                        [STYLE_EXPRESSION_INPUTS_PROP_COLOR_RAMP_PARAMETER]
    );
}

const char* maplibre_native_ffi_style_expression_inputs_get_feature(
  MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self), NULL
  );
  return self->feature;
}

void maplibre_native_ffi_style_expression_inputs_set_feature(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self));
  char* previous = self->feature;
  self->feature = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->feature) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      style_expression_inputs_properties[STYLE_EXPRESSION_INPUTS_PROP_FEATURE]
    );
}

const char* maplibre_native_ffi_style_expression_inputs_get_feature_state(
  MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self), NULL
  );
  return self->feature_state;
}

void maplibre_native_ffi_style_expression_inputs_set_feature_state(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self));
  char* previous = self->feature_state;
  self->feature_state = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->feature_state) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), style_expression_inputs_properties
                        [STYLE_EXPRESSION_INPUTS_PROP_FEATURE_STATE]
    );
}

const char* maplibre_native_ffi_style_expression_inputs_get_global_state(
  MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self), NULL
  );
  return self->global_state;
}

void maplibre_native_ffi_style_expression_inputs_set_global_state(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self));
  char* previous = self->global_state;
  self->global_state = g_strdup(value != NULL ? value : "");
  gboolean changed = g_strcmp0(previous, self->global_state) != 0;
  g_free(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), style_expression_inputs_properties
                        [STYLE_EXPRESSION_INPUTS_PROP_GLOBAL_STATE]
    );
}

MaplibreNativeFfiCanonicalTileId*
maplibre_native_ffi_style_expression_inputs_get_canonical_tile_id(
  MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self), NULL
  );
  return (MaplibreNativeFfiCanonicalTileId*)&self->canonical_tile_id;
}

gboolean maplibre_native_ffi_style_expression_inputs_has_canonical_tile_id(
  MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self), FALSE
  );
  return (self->fields & MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID) != 0;
}

void maplibre_native_ffi_style_expression_inputs_set_canonical_tile_id(
  MaplibreNativeFfiStyleExpressionInputs* self,
  const MaplibreNativeFfiCanonicalTileId* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self));
  g_return_if_fail(value != NULL);
  gboolean was_set =
    (self->fields & MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID) != 0;
  gboolean changed =
    memcmp(&self->canonical_tile_id, value, sizeof self->canonical_tile_id) !=
    0;
  self->canonical_tile_id = *value;
  self->fields |= MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), style_expression_inputs_properties
                        [STYLE_EXPRESSION_INPUTS_PROP_CANONICAL_TILE_ID]
    );
}

const char* const*
maplibre_native_ffi_style_expression_inputs_get_available_images(
  MaplibreNativeFfiStyleExpressionInputs* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self), NULL
  );
  return (const char* const*)self->available_images;
}

void maplibre_native_ffi_style_expression_inputs_set_available_images(
  MaplibreNativeFfiStyleExpressionInputs* self, const char* const* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(self));
  char** previous = self->available_images;
  self->available_images =
    value != NULL ? g_strdupv((char**)value) : g_new0(char*, 1);
  gboolean changed =
    !maplibre_native_ffi_strv_equal(previous, self->available_images);
  g_strfreev(previous);
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), style_expression_inputs_properties
                        [STYLE_EXPRESSION_INPUTS_PROP_AVAILABLE_IMAGES]
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiStyleExpressionType) ==
  sizeof(mln_style_expression_type)
);
SAME_MEMBER(
  MaplibreNativeFfiStyleExpressionType, mln_style_expression_type, kind
);
SAME_MEMBER(
  MaplibreNativeFfiStyleExpressionType, mln_style_expression_type, item_kind
);
SAME_MEMBER(
  MaplibreNativeFfiStyleExpressionType, mln_style_expression_type, item_count
);

void maplibre_native_ffi_style_expression_type_to_native(
  const MaplibreNativeFfiStyleExpressionType* self,
  mln_style_expression_type* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

struct _MaplibreNativeFfiStyleExpressionOptions {
  GObject parent_instance;
  guint64 fields;
  MaplibreNativeFfiStyleExpressionType expected_type;
};

enum {
  STYLE_EXPRESSION_OPTIONS_PROP_0,
  STYLE_EXPRESSION_OPTIONS_PROP_EXPECTED_TYPE,
  STYLE_EXPRESSION_OPTIONS_N_PROPERTIES
};

static GParamSpec*
  style_expression_options_properties[STYLE_EXPRESSION_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiStyleExpressionOptions,
  maplibre_native_ffi_style_expression_options, G_TYPE_OBJECT
)

static void maplibre_native_ffi_style_expression_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiStyleExpressionOptions* self =
    MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_OPTIONS(object);
  switch (id) {
    case STYLE_EXPRESSION_OPTIONS_PROP_EXPECTED_TYPE:
      g_value_set_boxed(value, &self->expected_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_style_expression_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiStyleExpressionOptions* self =
    MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_OPTIONS(object);
  switch (id) {
    case STYLE_EXPRESSION_OPTIONS_PROP_EXPECTED_TYPE:
      maplibre_native_ffi_style_expression_options_set_expected_type(
        self, g_value_get_boxed(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_style_expression_options_class_init(
  MaplibreNativeFfiStyleExpressionOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_style_expression_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_style_expression_options_set_property;
  style_expression_options_properties
    [STYLE_EXPRESSION_OPTIONS_PROP_EXPECTED_TYPE] = g_param_spec_boxed(
      "expected-type", NULL, NULL,
      MAPLIBRE_NATIVE_FFI_TYPE_STYLE_EXPRESSION_TYPE, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, STYLE_EXPRESSION_OPTIONS_N_PROPERTIES,
    style_expression_options_properties
  );
}

static void maplibre_native_ffi_style_expression_options_init(
  MaplibreNativeFfiStyleExpressionOptions* self
) {
  (void)self;
}

MaplibreNativeFfiStyleExpressionOptions*
maplibre_native_ffi_style_expression_options_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_STYLE_EXPRESSION_OPTIONS, NULL);
}

MaplibreNativeFfiStyleExpressionType*
maplibre_native_ffi_style_expression_options_get_expected_type(
  MaplibreNativeFfiStyleExpressionOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_OPTIONS(self), NULL
  );
  return (MaplibreNativeFfiStyleExpressionType*)&self->expected_type;
}

gboolean maplibre_native_ffi_style_expression_options_has_expected_type(
  MaplibreNativeFfiStyleExpressionOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE) != 0;
}

void maplibre_native_ffi_style_expression_options_set_expected_type(
  MaplibreNativeFfiStyleExpressionOptions* self,
  const MaplibreNativeFfiStyleExpressionType* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_OPTIONS(self));
  g_return_if_fail(value != NULL);
  gboolean was_set =
    (self->fields & MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE) != 0;
  gboolean changed =
    memcmp(&self->expected_type, value, sizeof self->expected_type) != 0;
  self->expected_type = *value;
  self->fields |= MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), style_expression_options_properties
                        [STYLE_EXPRESSION_OPTIONS_PROP_EXPECTED_TYPE]
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
  GObject parent_instance;
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

enum {
  STYLE_IMAGE_OPTIONS_PROP_0,
  STYLE_IMAGE_OPTIONS_PROP_CONTENT,
  STYLE_IMAGE_OPTIONS_PROP_TEXT_FIT_WIDTH,
  STYLE_IMAGE_OPTIONS_PROP_TEXT_FIT_HEIGHT,
  STYLE_IMAGE_OPTIONS_PROP_PIXEL_RATIO,
  STYLE_IMAGE_OPTIONS_PROP_SDF,
  STYLE_IMAGE_OPTIONS_PROP_KEEP_UNUSED,
  STYLE_IMAGE_OPTIONS_N_PROPERTIES
};

static GParamSpec*
  style_image_options_properties[STYLE_IMAGE_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiStyleImageOptions, maplibre_native_ffi_style_image_options,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_style_image_options_finalize(GObject* object) {
  MaplibreNativeFfiStyleImageOptions* self =
    MAPLIBRE_NATIVE_FFI_STYLE_IMAGE_OPTIONS(object);
  g_clear_pointer(&self->stretch_x, g_ptr_array_unref);
  g_clear_pointer(&self->stretch_y, g_ptr_array_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_style_image_options_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_style_image_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiStyleImageOptions* self =
    MAPLIBRE_NATIVE_FFI_STYLE_IMAGE_OPTIONS(object);
  switch (id) {
    case STYLE_IMAGE_OPTIONS_PROP_CONTENT:
      g_value_set_boxed(value, &self->content);
      break;
    case STYLE_IMAGE_OPTIONS_PROP_TEXT_FIT_WIDTH:
      g_value_set_enum(value, self->text_fit_width);
      break;
    case STYLE_IMAGE_OPTIONS_PROP_TEXT_FIT_HEIGHT:
      g_value_set_enum(value, self->text_fit_height);
      break;
    case STYLE_IMAGE_OPTIONS_PROP_PIXEL_RATIO:
      g_value_set_float(value, self->pixel_ratio);
      break;
    case STYLE_IMAGE_OPTIONS_PROP_SDF:
      g_value_set_boolean(value, self->sdf);
      break;
    case STYLE_IMAGE_OPTIONS_PROP_KEEP_UNUSED:
      g_value_set_boolean(value, self->keep_unused);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_style_image_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiStyleImageOptions* self =
    MAPLIBRE_NATIVE_FFI_STYLE_IMAGE_OPTIONS(object);
  switch (id) {
    case STYLE_IMAGE_OPTIONS_PROP_CONTENT:
      maplibre_native_ffi_style_image_options_set_content(
        self, g_value_get_boxed(value)
      );
      break;
    case STYLE_IMAGE_OPTIONS_PROP_TEXT_FIT_WIDTH:
      maplibre_native_ffi_style_image_options_set_text_fit_width(
        self, g_value_get_enum(value)
      );
      break;
    case STYLE_IMAGE_OPTIONS_PROP_TEXT_FIT_HEIGHT:
      maplibre_native_ffi_style_image_options_set_text_fit_height(
        self, g_value_get_enum(value)
      );
      break;
    case STYLE_IMAGE_OPTIONS_PROP_PIXEL_RATIO:
      maplibre_native_ffi_style_image_options_set_pixel_ratio(
        self, g_value_get_float(value)
      );
      break;
    case STYLE_IMAGE_OPTIONS_PROP_SDF:
      maplibre_native_ffi_style_image_options_set_sdf(
        self, g_value_get_boolean(value)
      );
      break;
    case STYLE_IMAGE_OPTIONS_PROP_KEEP_UNUSED:
      maplibre_native_ffi_style_image_options_set_keep_unused(
        self, g_value_get_boolean(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_style_image_options_class_init(
  MaplibreNativeFfiStyleImageOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_style_image_options_finalize;
  object_class->get_property =
    maplibre_native_ffi_style_image_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_style_image_options_set_property;
  style_image_options_properties[STYLE_IMAGE_OPTIONS_PROP_CONTENT] =
    g_param_spec_boxed(
      "content", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_IMAGE_CONTENT,
      PROPERTY_FLAGS
    );
  style_image_options_properties[STYLE_IMAGE_OPTIONS_PROP_TEXT_FIT_WIDTH] =
    g_param_spec_enum(
      "text-fit-width", NULL, NULL,
      MAPLIBRE_NATIVE_FFI_TYPE_STYLE_IMAGE_TEXT_FIT, 0, PROPERTY_FLAGS
    );
  style_image_options_properties[STYLE_IMAGE_OPTIONS_PROP_TEXT_FIT_HEIGHT] =
    g_param_spec_enum(
      "text-fit-height", NULL, NULL,
      MAPLIBRE_NATIVE_FFI_TYPE_STYLE_IMAGE_TEXT_FIT, 0, PROPERTY_FLAGS
    );
  style_image_options_properties[STYLE_IMAGE_OPTIONS_PROP_PIXEL_RATIO] =
    g_param_spec_float(
      "pixel-ratio", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  style_image_options_properties[STYLE_IMAGE_OPTIONS_PROP_SDF] =
    g_param_spec_boolean("sdf", NULL, NULL, FALSE, PROPERTY_FLAGS);
  style_image_options_properties[STYLE_IMAGE_OPTIONS_PROP_KEEP_UNUSED] =
    g_param_spec_boolean("keep-unused", NULL, NULL, FALSE, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, STYLE_IMAGE_OPTIONS_N_PROPERTIES,
    style_image_options_properties
  );
}

static void maplibre_native_ffi_style_image_options_init(
  MaplibreNativeFfiStyleImageOptions* self
) {
  self->stretch_x = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_image_stretch_free
  );
  self->stretch_y = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_image_stretch_free
  );
}

MaplibreNativeFfiStyleImageOptions* maplibre_native_ffi_style_image_options_new(
  void
) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_STYLE_IMAGE_OPTIONS, NULL);
}

GPtrArray* maplibre_native_ffi_style_image_options_get_stretch_x(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), NULL);
  return self->stretch_x;
}

gboolean maplibre_native_ffi_style_image_options_has_stretch_x(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_STRETCH_X) != 0;
}

void maplibre_native_ffi_style_image_options_set_stretch_x(
  MaplibreNativeFfiStyleImageOptions* self,
  const MaplibreNativeFfiImageStretch* value, gsize value_count
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self));
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
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), NULL);
  return self->stretch_y;
}

gboolean maplibre_native_ffi_style_image_options_has_stretch_y(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_STRETCH_Y) != 0;
}

void maplibre_native_ffi_style_image_options_set_stretch_y(
  MaplibreNativeFfiStyleImageOptions* self,
  const MaplibreNativeFfiImageStretch* value, gsize value_count
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self));
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
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), NULL);
  return (MaplibreNativeFfiImageContent*)&self->content;
}

gboolean maplibre_native_ffi_style_image_options_has_content(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_CONTENT) != 0;
}

void maplibre_native_ffi_style_image_options_set_content(
  MaplibreNativeFfiStyleImageOptions* self,
  const MaplibreNativeFfiImageContent* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self));
  g_return_if_fail(value != NULL);
  gboolean was_set = (self->fields & MLN_STYLE_IMAGE_OPTION_CONTENT) != 0;
  gboolean changed = memcmp(&self->content, value, sizeof self->content) != 0;
  self->content = *value;
  self->fields |= MLN_STYLE_IMAGE_OPTION_CONTENT;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      style_image_options_properties[STYLE_IMAGE_OPTIONS_PROP_CONTENT]
    );
}

MaplibreNativeFfiStyleImageTextFit
maplibre_native_ffi_style_image_options_get_text_fit_width(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self),
    (MaplibreNativeFfiStyleImageTextFit)0
  );
  return self->text_fit_width;
}

gboolean maplibre_native_ffi_style_image_options_has_text_fit_width(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH) != 0;
}

void maplibre_native_ffi_style_image_options_set_text_fit_width(
  MaplibreNativeFfiStyleImageOptions* self,
  MaplibreNativeFfiStyleImageTextFit value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH) != 0;
  gboolean changed = self->text_fit_width != value;
  self->text_fit_width = value;
  self->fields |= MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      style_image_options_properties[STYLE_IMAGE_OPTIONS_PROP_TEXT_FIT_WIDTH]
    );
}

MaplibreNativeFfiStyleImageTextFit
maplibre_native_ffi_style_image_options_get_text_fit_height(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self),
    (MaplibreNativeFfiStyleImageTextFit)0
  );
  return self->text_fit_height;
}

gboolean maplibre_native_ffi_style_image_options_has_text_fit_height(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT) != 0;
}

void maplibre_native_ffi_style_image_options_set_text_fit_height(
  MaplibreNativeFfiStyleImageOptions* self,
  MaplibreNativeFfiStyleImageTextFit value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT) != 0;
  gboolean changed = self->text_fit_height != value;
  self->text_fit_height = value;
  self->fields |= MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      style_image_options_properties[STYLE_IMAGE_OPTIONS_PROP_TEXT_FIT_HEIGHT]
    );
}

gfloat maplibre_native_ffi_style_image_options_get_pixel_ratio(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), 0);
  return self->pixel_ratio;
}

gboolean maplibre_native_ffi_style_image_options_has_pixel_ratio(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO) != 0;
}

void maplibre_native_ffi_style_image_options_set_pixel_ratio(
  MaplibreNativeFfiStyleImageOptions* self, gfloat value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO) != 0;
  gboolean changed = self->pixel_ratio != value;
  self->pixel_ratio = value;
  self->fields |= MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      style_image_options_properties[STYLE_IMAGE_OPTIONS_PROP_PIXEL_RATIO]
    );
}

gboolean maplibre_native_ffi_style_image_options_get_sdf(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), FALSE);
  return self->sdf;
}

gboolean maplibre_native_ffi_style_image_options_has_sdf(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_SDF) != 0;
}

void maplibre_native_ffi_style_image_options_set_sdf(
  MaplibreNativeFfiStyleImageOptions* self, gboolean value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_STYLE_IMAGE_OPTION_SDF) != 0;
  gboolean changed = self->sdf != value;
  self->sdf = value;
  self->fields |= MLN_STYLE_IMAGE_OPTION_SDF;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      style_image_options_properties[STYLE_IMAGE_OPTIONS_PROP_SDF]
    );
}

gboolean maplibre_native_ffi_style_image_options_get_keep_unused(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), FALSE);
  return self->keep_unused;
}

gboolean maplibre_native_ffi_style_image_options_has_keep_unused(
  MaplibreNativeFfiStyleImageOptions* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self), FALSE);
  return (self->fields & MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED) != 0;
}

void maplibre_native_ffi_style_image_options_set_keep_unused(
  MaplibreNativeFfiStyleImageOptions* self, gboolean value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED) != 0;
  gboolean changed = self->keep_unused != value;
  self->keep_unused = value;
  self->fields |= MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      style_image_options_properties[STYLE_IMAGE_OPTIONS_PROP_KEEP_UNUSED]
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
  g_clear_object(&self->geo_json);
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
    g_clear_object(&self->geo_json);
    self->geo_json =
      maplibre_native_ffi_geo_json_source_options_from_native(&in->geo_json);
  }
  return self;
}

struct _MaplibreNativeFfiStyleTransitionOptions {
  GObject parent_instance;
  guint64 fields;
  gdouble duration_ms;
  gdouble delay_ms;
  gboolean enable_placement_transitions;
};

enum {
  STYLE_TRANSITION_OPTIONS_PROP_0,
  STYLE_TRANSITION_OPTIONS_PROP_DURATION_MS,
  STYLE_TRANSITION_OPTIONS_PROP_DELAY_MS,
  STYLE_TRANSITION_OPTIONS_PROP_ENABLE_PLACEMENT_TRANSITIONS,
  STYLE_TRANSITION_OPTIONS_N_PROPERTIES
};

static GParamSpec*
  style_transition_options_properties[STYLE_TRANSITION_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiStyleTransitionOptions,
  maplibre_native_ffi_style_transition_options, G_TYPE_OBJECT
)

static void maplibre_native_ffi_style_transition_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiStyleTransitionOptions* self =
    MAPLIBRE_NATIVE_FFI_STYLE_TRANSITION_OPTIONS(object);
  switch (id) {
    case STYLE_TRANSITION_OPTIONS_PROP_DURATION_MS:
      g_value_set_double(value, self->duration_ms);
      break;
    case STYLE_TRANSITION_OPTIONS_PROP_DELAY_MS:
      g_value_set_double(value, self->delay_ms);
      break;
    case STYLE_TRANSITION_OPTIONS_PROP_ENABLE_PLACEMENT_TRANSITIONS:
      g_value_set_boolean(value, self->enable_placement_transitions);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_style_transition_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiStyleTransitionOptions* self =
    MAPLIBRE_NATIVE_FFI_STYLE_TRANSITION_OPTIONS(object);
  switch (id) {
    case STYLE_TRANSITION_OPTIONS_PROP_DURATION_MS:
      maplibre_native_ffi_style_transition_options_set_duration_ms(
        self, g_value_get_double(value)
      );
      break;
    case STYLE_TRANSITION_OPTIONS_PROP_DELAY_MS:
      maplibre_native_ffi_style_transition_options_set_delay_ms(
        self, g_value_get_double(value)
      );
      break;
    case STYLE_TRANSITION_OPTIONS_PROP_ENABLE_PLACEMENT_TRANSITIONS:
      maplibre_native_ffi_style_transition_options_set_enable_placement_transitions(
        self, g_value_get_boolean(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_style_transition_options_class_init(
  MaplibreNativeFfiStyleTransitionOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_style_transition_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_style_transition_options_set_property;
  style_transition_options_properties
    [STYLE_TRANSITION_OPTIONS_PROP_DURATION_MS] = g_param_spec_double(
      "duration-ms", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  style_transition_options_properties[STYLE_TRANSITION_OPTIONS_PROP_DELAY_MS] =
    g_param_spec_double(
      "delay-ms", NULL, NULL, -INFINITY, INFINITY, 0, PROPERTY_FLAGS
    );
  style_transition_options_properties
    [STYLE_TRANSITION_OPTIONS_PROP_ENABLE_PLACEMENT_TRANSITIONS] =
      g_param_spec_boolean(
        "enable-placement-transitions", NULL, NULL, FALSE, PROPERTY_FLAGS
      );
  g_object_class_install_properties(
    object_class, STYLE_TRANSITION_OPTIONS_N_PROPERTIES,
    style_transition_options_properties
  );
}

static void maplibre_native_ffi_style_transition_options_init(
  MaplibreNativeFfiStyleTransitionOptions* self
) {
  (void)self;
}

MaplibreNativeFfiStyleTransitionOptions*
maplibre_native_ffi_style_transition_options_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_STYLE_TRANSITION_OPTIONS, NULL);
}

gdouble maplibre_native_ffi_style_transition_options_get_duration_ms(
  MaplibreNativeFfiStyleTransitionOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_TRANSITION_OPTIONS(self), 0
  );
  return self->duration_ms;
}

gboolean maplibre_native_ffi_style_transition_options_has_duration_ms(
  MaplibreNativeFfiStyleTransitionOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_TRANSITION_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_STYLE_TRANSITION_OPTION_DURATION) != 0;
}

void maplibre_native_ffi_style_transition_options_set_duration_ms(
  MaplibreNativeFfiStyleTransitionOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_TRANSITION_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_STYLE_TRANSITION_OPTION_DURATION) != 0;
  gboolean changed = self->duration_ms != value;
  self->duration_ms = value;
  self->fields |= MLN_STYLE_TRANSITION_OPTION_DURATION;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), style_transition_options_properties
                        [STYLE_TRANSITION_OPTIONS_PROP_DURATION_MS]
    );
}

gdouble maplibre_native_ffi_style_transition_options_get_delay_ms(
  MaplibreNativeFfiStyleTransitionOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_TRANSITION_OPTIONS(self), 0
  );
  return self->delay_ms;
}

gboolean maplibre_native_ffi_style_transition_options_has_delay_ms(
  MaplibreNativeFfiStyleTransitionOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_TRANSITION_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_STYLE_TRANSITION_OPTION_DELAY) != 0;
}

void maplibre_native_ffi_style_transition_options_set_delay_ms(
  MaplibreNativeFfiStyleTransitionOptions* self, gdouble value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_TRANSITION_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_STYLE_TRANSITION_OPTION_DELAY) != 0;
  gboolean changed = self->delay_ms != value;
  self->delay_ms = value;
  self->fields |= MLN_STYLE_TRANSITION_OPTION_DELAY;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self), style_transition_options_properties
                        [STYLE_TRANSITION_OPTIONS_PROP_DELAY_MS]
    );
}

gboolean
maplibre_native_ffi_style_transition_options_get_enable_placement_transitions(
  MaplibreNativeFfiStyleTransitionOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_TRANSITION_OPTIONS(self), FALSE
  );
  return self->enable_placement_transitions;
}

gboolean
maplibre_native_ffi_style_transition_options_has_enable_placement_transitions(
  MaplibreNativeFfiStyleTransitionOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_STYLE_TRANSITION_OPTIONS(self), FALSE
  );
  return (self->fields &
          MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS) != 0;
}

void maplibre_native_ffi_style_transition_options_set_enable_placement_transitions(
  MaplibreNativeFfiStyleTransitionOptions* self, gboolean value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_TRANSITION_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS) !=
    0;
  gboolean changed = self->enable_placement_transitions != value;
  self->enable_placement_transitions = value;
  self->fields |= MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      style_transition_options_properties
        [STYLE_TRANSITION_OPTIONS_PROP_ENABLE_PLACEMENT_TRANSITIONS]
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiTextureImageInfo) == sizeof(mln_texture_image_info)
);
SAME_MEMBER(MaplibreNativeFfiTextureImageInfo, mln_texture_image_info, width);
SAME_MEMBER(MaplibreNativeFfiTextureImageInfo, mln_texture_image_info, height);
SAME_MEMBER(MaplibreNativeFfiTextureImageInfo, mln_texture_image_info, stride);
SAME_MEMBER(
  MaplibreNativeFfiTextureImageInfo, mln_texture_image_info, byte_length
);

void maplibre_native_ffi_texture_image_info_read(
  MaplibreNativeFfiTextureImageInfo* out, const mln_texture_image_info* in
) {
  memcpy(out, in, sizeof *out);
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

G_STATIC_ASSERT(
  sizeof(MaplibreNativeFfiVulkanBorrowedTexture) ==
  sizeof(mln_vulkan_borrowed_texture)
);
SAME_MEMBER(
  MaplibreNativeFfiVulkanBorrowedTexture, mln_vulkan_borrowed_texture, image
);
SAME_MEMBER(
  MaplibreNativeFfiVulkanBorrowedTexture, mln_vulkan_borrowed_texture,
  image_view
);

void maplibre_native_ffi_vulkan_borrowed_texture_to_native(
  const MaplibreNativeFfiVulkanBorrowedTexture* self,
  mln_vulkan_borrowed_texture* out, MaplibreNativeFfiCall* call
) {
  (void)call;
  memcpy(out, self, sizeof *out);
}

struct _MaplibreNativeFfiVulkanContextDescriptor {
  GObject parent_instance;
  gpointer instance;
  gpointer physical_device;
  gpointer device;
  gpointer graphics_queue;
  guint32 graphics_queue_family_index;
  gpointer get_instance_proc_addr;
  gpointer get_device_proc_addr;
};

enum {
  VULKAN_CONTEXT_DESCRIPTOR_PROP_0,
  VULKAN_CONTEXT_DESCRIPTOR_PROP_INSTANCE,
  VULKAN_CONTEXT_DESCRIPTOR_PROP_PHYSICAL_DEVICE,
  VULKAN_CONTEXT_DESCRIPTOR_PROP_DEVICE,
  VULKAN_CONTEXT_DESCRIPTOR_PROP_GRAPHICS_QUEUE,
  VULKAN_CONTEXT_DESCRIPTOR_PROP_GRAPHICS_QUEUE_FAMILY_INDEX,
  VULKAN_CONTEXT_DESCRIPTOR_PROP_GET_INSTANCE_PROC_ADDR,
  VULKAN_CONTEXT_DESCRIPTOR_PROP_GET_DEVICE_PROC_ADDR,
  VULKAN_CONTEXT_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec*
  vulkan_context_descriptor_properties[VULKAN_CONTEXT_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiVulkanContextDescriptor,
  maplibre_native_ffi_vulkan_context_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_vulkan_context_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiVulkanContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_CONTEXT_DESCRIPTOR(object);
  switch (id) {
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_INSTANCE:
      g_value_set_pointer(value, self->instance);
      break;
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_PHYSICAL_DEVICE:
      g_value_set_pointer(value, self->physical_device);
      break;
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_DEVICE:
      g_value_set_pointer(value, self->device);
      break;
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_GRAPHICS_QUEUE:
      g_value_set_pointer(value, self->graphics_queue);
      break;
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_GRAPHICS_QUEUE_FAMILY_INDEX:
      g_value_set_uint(value, self->graphics_queue_family_index);
      break;
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_GET_INSTANCE_PROC_ADDR:
      g_value_set_pointer(value, self->get_instance_proc_addr);
      break;
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_GET_DEVICE_PROC_ADDR:
      g_value_set_pointer(value, self->get_device_proc_addr);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_vulkan_context_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiVulkanContextDescriptor* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_CONTEXT_DESCRIPTOR(object);
  switch (id) {
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_INSTANCE:
      maplibre_native_ffi_vulkan_context_descriptor_set_instance(
        self, g_value_get_pointer(value)
      );
      break;
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_PHYSICAL_DEVICE:
      maplibre_native_ffi_vulkan_context_descriptor_set_physical_device(
        self, g_value_get_pointer(value)
      );
      break;
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_DEVICE:
      maplibre_native_ffi_vulkan_context_descriptor_set_device(
        self, g_value_get_pointer(value)
      );
      break;
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_GRAPHICS_QUEUE:
      maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue(
        self, g_value_get_pointer(value)
      );
      break;
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_GRAPHICS_QUEUE_FAMILY_INDEX:
      maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue_family_index(
        self, g_value_get_uint(value)
      );
      break;
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_GET_INSTANCE_PROC_ADDR:
      maplibre_native_ffi_vulkan_context_descriptor_set_get_instance_proc_addr(
        self, g_value_get_pointer(value)
      );
      break;
    case VULKAN_CONTEXT_DESCRIPTOR_PROP_GET_DEVICE_PROC_ADDR:
      maplibre_native_ffi_vulkan_context_descriptor_set_get_device_proc_addr(
        self, g_value_get_pointer(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_vulkan_context_descriptor_class_init(
  MaplibreNativeFfiVulkanContextDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_vulkan_context_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_vulkan_context_descriptor_set_property;
  vulkan_context_descriptor_properties
    [VULKAN_CONTEXT_DESCRIPTOR_PROP_INSTANCE] =
      g_param_spec_pointer("instance", NULL, NULL, PROPERTY_FLAGS);
  vulkan_context_descriptor_properties
    [VULKAN_CONTEXT_DESCRIPTOR_PROP_PHYSICAL_DEVICE] =
      g_param_spec_pointer("physical-device", NULL, NULL, PROPERTY_FLAGS);
  vulkan_context_descriptor_properties[VULKAN_CONTEXT_DESCRIPTOR_PROP_DEVICE] =
    g_param_spec_pointer("device", NULL, NULL, PROPERTY_FLAGS);
  vulkan_context_descriptor_properties
    [VULKAN_CONTEXT_DESCRIPTOR_PROP_GRAPHICS_QUEUE] =
      g_param_spec_pointer("graphics-queue", NULL, NULL, PROPERTY_FLAGS);
  vulkan_context_descriptor_properties
    [VULKAN_CONTEXT_DESCRIPTOR_PROP_GRAPHICS_QUEUE_FAMILY_INDEX] =
      g_param_spec_uint(
        "graphics-queue-family-index", NULL, NULL, 0, G_MAXUINT32, 0,
        PROPERTY_FLAGS
      );
  vulkan_context_descriptor_properties
    [VULKAN_CONTEXT_DESCRIPTOR_PROP_GET_INSTANCE_PROC_ADDR] =
      g_param_spec_pointer(
        "get-instance-proc-addr", NULL, NULL, PROPERTY_FLAGS
      );
  vulkan_context_descriptor_properties
    [VULKAN_CONTEXT_DESCRIPTOR_PROP_GET_DEVICE_PROC_ADDR] =
      g_param_spec_pointer("get-device-proc-addr", NULL, NULL, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, VULKAN_CONTEXT_DESCRIPTOR_N_PROPERTIES,
    vulkan_context_descriptor_properties
  );
}

static void maplibre_native_ffi_vulkan_context_descriptor_init(
  MaplibreNativeFfiVulkanContextDescriptor* self
) {
  (void)self;
}

MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_context_descriptor_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_CONTEXT_DESCRIPTOR, NULL);
}

gpointer maplibre_native_ffi_vulkan_context_descriptor_get_instance(
  MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self), 0
  );
  return self->instance;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_instance(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->instance != value;
  self->instance = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_context_descriptor_properties
                        [VULKAN_CONTEXT_DESCRIPTOR_PROP_INSTANCE]
    );
}

gpointer maplibre_native_ffi_vulkan_context_descriptor_get_physical_device(
  MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self), 0
  );
  return self->physical_device;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_physical_device(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->physical_device != value;
  self->physical_device = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_context_descriptor_properties
                        [VULKAN_CONTEXT_DESCRIPTOR_PROP_PHYSICAL_DEVICE]
    );
}

gpointer maplibre_native_ffi_vulkan_context_descriptor_get_device(
  MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self), 0
  );
  return self->device;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_device(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->device != value;
  self->device = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_context_descriptor_properties
                        [VULKAN_CONTEXT_DESCRIPTOR_PROP_DEVICE]
    );
}

gpointer maplibre_native_ffi_vulkan_context_descriptor_get_graphics_queue(
  MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self), 0
  );
  return self->graphics_queue;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->graphics_queue != value;
  self->graphics_queue = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_context_descriptor_properties
                        [VULKAN_CONTEXT_DESCRIPTOR_PROP_GRAPHICS_QUEUE]
    );
}

guint32
maplibre_native_ffi_vulkan_context_descriptor_get_graphics_queue_family_index(
  MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self), 0
  );
  return self->graphics_queue_family_index;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue_family_index(
  MaplibreNativeFfiVulkanContextDescriptor* self, guint32 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->graphics_queue_family_index != value;
  self->graphics_queue_family_index = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      vulkan_context_descriptor_properties
        [VULKAN_CONTEXT_DESCRIPTOR_PROP_GRAPHICS_QUEUE_FAMILY_INDEX]
    );
}

gpointer
maplibre_native_ffi_vulkan_context_descriptor_get_get_instance_proc_addr(
  MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self), 0
  );
  return self->get_instance_proc_addr;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_get_instance_proc_addr(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->get_instance_proc_addr != value;
  self->get_instance_proc_addr = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_context_descriptor_properties
                        [VULKAN_CONTEXT_DESCRIPTOR_PROP_GET_INSTANCE_PROC_ADDR]
    );
}

gpointer maplibre_native_ffi_vulkan_context_descriptor_get_get_device_proc_addr(
  MaplibreNativeFfiVulkanContextDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self), 0
  );
  return self->get_device_proc_addr;
}

void maplibre_native_ffi_vulkan_context_descriptor_set_get_device_proc_addr(
  MaplibreNativeFfiVulkanContextDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(self));
  gboolean changed = self->get_device_proc_addr != value;
  self->get_device_proc_addr = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_context_descriptor_properties
                        [VULKAN_CONTEXT_DESCRIPTOR_PROP_GET_DEVICE_PROC_ADDR]
    );
}

MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_context_descriptor_copy(
  const MaplibreNativeFfiVulkanContextDescriptor* self
) {
  MaplibreNativeFfiVulkanContextDescriptor* copy =
    maplibre_native_ffi_vulkan_context_descriptor_new();
  copy->instance = self->instance;
  copy->physical_device = self->physical_device;
  copy->device = self->device;
  copy->graphics_queue = self->graphics_queue;
  copy->graphics_queue_family_index = self->graphics_queue_family_index;
  copy->get_instance_proc_addr = self->get_instance_proc_addr;
  copy->get_device_proc_addr = self->get_device_proc_addr;
  return copy;
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
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 physical_width;
  guint32 physical_height;
  MaplibreNativeFfiVulkanContextDescriptor* context;
  GPtrArray* textures;
  guint32 format;
  guint32 final_layout;
  MaplibreNativeFfiVulkanImageOwner owner;
};

enum {
  VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_0,
  VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE,
  VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH,
  VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT,
  VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_CONTEXT,
  VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_FORMAT,
  VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_FINAL_LAYOUT,
  VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_OWNER,
  VULKAN_BORROWED_TEXTURE_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec* vulkan_borrowed_texture_descriptor_properties
  [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor,
  maplibre_native_ffi_vulkan_borrowed_texture_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_finalize(
  GObject* object
) {
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(object);
  g_clear_object(&self->context);
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  G_OBJECT_CLASS(
    maplibre_native_ffi_vulkan_borrowed_texture_descriptor_parent_class
  )
    ->finalize(object);
}

static void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(object);
  switch (id) {
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH:
      g_value_set_uint(value, self->physical_width);
      break;
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT:
      g_value_set_uint(value, self->physical_height);
      break;
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_CONTEXT:
      g_value_set_object(value, self->context);
      break;
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_FORMAT:
      g_value_set_uint(value, self->format);
      break;
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_FINAL_LAYOUT:
      g_value_set_uint(value, self->final_layout);
      break;
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_OWNER:
      g_value_set_enum(value, self->owner);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(object);
  switch (id) {
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH:
      maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_width(
        self, g_value_get_uint(value)
      );
      break;
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT:
      maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_height(
        self, g_value_get_uint(value)
      );
      break;
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_CONTEXT:
      maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_context(
        self, g_value_get_object(value)
      );
      break;
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_FORMAT:
      maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_format(
        self, g_value_get_uint(value)
      );
      break;
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_FINAL_LAYOUT:
      maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_final_layout(
        self, g_value_get_uint(value)
      );
      break;
    case VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_OWNER:
      maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_owner(
        self, g_value_get_enum(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_class_init(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_vulkan_borrowed_texture_descriptor_finalize;
  object_class->get_property =
    maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_property;
  vulkan_borrowed_texture_descriptor_properties
    [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE] = g_param_spec_boxed(
      "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
      PROPERTY_FLAGS
    );
  vulkan_borrowed_texture_descriptor_properties
    [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH] =
      g_param_spec_uint(
        "physical-width", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
      );
  vulkan_borrowed_texture_descriptor_properties
    [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT] =
      g_param_spec_uint(
        "physical-height", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
      );
  vulkan_borrowed_texture_descriptor_properties
    [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_CONTEXT] = g_param_spec_object(
      "context", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_CONTEXT_DESCRIPTOR,
      PROPERTY_FLAGS
    );
  vulkan_borrowed_texture_descriptor_properties
    [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_FORMAT] = g_param_spec_uint(
      "format", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
    );
  vulkan_borrowed_texture_descriptor_properties
    [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_FINAL_LAYOUT] = g_param_spec_uint(
      "final-layout", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
    );
  vulkan_borrowed_texture_descriptor_properties
    [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_OWNER] = g_param_spec_enum(
      "owner", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_IMAGE_OWNER, 0,
      PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, VULKAN_BORROWED_TEXTURE_DESCRIPTOR_N_PROPERTIES,
    vulkan_borrowed_texture_descriptor_properties
  );
}

static void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_init(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  self->context = maplibre_native_ffi_vulkan_context_descriptor_new();
  self->textures = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_vulkan_borrowed_texture_free
  );
}

MaplibreNativeFfiVulkanBorrowedTextureDescriptor*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_BORROWED_TEXTURE_DESCRIPTOR, NULL
  );
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_logical_size(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_logical_size(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_borrowed_texture_descriptor_properties
                        [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_LOGICAL_SIZE]
    );
}

guint32
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_physical_width(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self), 0
  );
  return self->physical_width;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_width(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  gboolean changed = self->physical_width != value;
  self->physical_width = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_borrowed_texture_descriptor_properties
                        [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_WIDTH]
    );
}

guint32
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_physical_height(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self), 0
  );
  return self->physical_height;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_height(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  gboolean changed = self->physical_height != value;
  self->physical_height = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      vulkan_borrowed_texture_descriptor_properties
        [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_PHYSICAL_HEIGHT]
    );
}

MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_context(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self), NULL
  );
  return self->context;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_context(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  MaplibreNativeFfiVulkanContextDescriptor* value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(value));
  MaplibreNativeFfiVulkanContextDescriptor* previous = self->context;
  self->context = maplibre_native_ffi_vulkan_context_descriptor_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self), vulkan_borrowed_texture_descriptor_properties
                      [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_CONTEXT]
  );
}

GPtrArray* maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_textures(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self), NULL
  );
  return self->textures;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_textures(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  const MaplibreNativeFfiVulkanBorrowedTexture* value, gsize value_count
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
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
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self), 0
  );
  return self->format;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_format(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  gboolean changed = self->format != value;
  self->format = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_borrowed_texture_descriptor_properties
                        [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_FORMAT]
    );
}

guint32 maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_final_layout(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self), 0
  );
  return self->final_layout;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_final_layout(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  gboolean changed = self->final_layout != value;
  self->final_layout = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_borrowed_texture_descriptor_properties
                        [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_FINAL_LAYOUT]
    );
}

MaplibreNativeFfiVulkanImageOwner
maplibre_native_ffi_vulkan_borrowed_texture_descriptor_get_owner(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self),
    (MaplibreNativeFfiVulkanImageOwner)0
  );
  return self->owner;
}

void maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_owner(
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* self,
  MaplibreNativeFfiVulkanImageOwner value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(self)
  );
  gboolean changed = self->owner != value;
  self->owner = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_borrowed_texture_descriptor_properties
                        [VULKAN_BORROWED_TEXTURE_DESCRIPTOR_PROP_OWNER]
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

struct _MaplibreNativeFfiVulkanBorrowedTextureReplacement {
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  guint32 physical_width;
  guint32 physical_height;
  GPtrArray* textures;
};

enum {
  VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_0,
  VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE,
  VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH,
  VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT,
  VULKAN_BORROWED_TEXTURE_REPLACEMENT_N_PROPERTIES
};

static GParamSpec* vulkan_borrowed_texture_replacement_properties
  [VULKAN_BORROWED_TEXTURE_REPLACEMENT_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement,
  maplibre_native_ffi_vulkan_borrowed_texture_replacement, G_TYPE_OBJECT
)

static void maplibre_native_ffi_vulkan_borrowed_texture_replacement_finalize(
  GObject* object
) {
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_BORROWED_TEXTURE_REPLACEMENT(object);
  g_clear_pointer(&self->textures, g_ptr_array_unref);
  G_OBJECT_CLASS(
    maplibre_native_ffi_vulkan_borrowed_texture_replacement_parent_class
  )
    ->finalize(object);
}

static void
maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_BORROWED_TEXTURE_REPLACEMENT(object);
  switch (id) {
    case VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH:
      g_value_set_uint(value, self->physical_width);
      break;
    case VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT:
      g_value_set_uint(value, self->physical_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void
maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_BORROWED_TEXTURE_REPLACEMENT(object);
  switch (id) {
    case VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH:
      maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_width(
        self, g_value_get_uint(value)
      );
      break;
    case VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT:
      maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_height(
        self, g_value_get_uint(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_vulkan_borrowed_texture_replacement_class_init(
  MaplibreNativeFfiVulkanBorrowedTextureReplacementClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_vulkan_borrowed_texture_replacement_finalize;
  object_class->get_property =
    maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_property;
  object_class->set_property =
    maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_property;
  vulkan_borrowed_texture_replacement_properties
    [VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE] =
      g_param_spec_boxed(
        "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
        PROPERTY_FLAGS
      );
  vulkan_borrowed_texture_replacement_properties
    [VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH] =
      g_param_spec_uint(
        "physical-width", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
      );
  vulkan_borrowed_texture_replacement_properties
    [VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT] =
      g_param_spec_uint(
        "physical-height", NULL, NULL, 0, G_MAXUINT32, 0, PROPERTY_FLAGS
      );
  g_object_class_install_properties(
    object_class, VULKAN_BORROWED_TEXTURE_REPLACEMENT_N_PROPERTIES,
    vulkan_borrowed_texture_replacement_properties
  );
}

static void maplibre_native_ffi_vulkan_borrowed_texture_replacement_init(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
) {
  self->textures = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_vulkan_borrowed_texture_free
  );
}

MaplibreNativeFfiVulkanBorrowedTextureReplacement*
maplibre_native_ffi_vulkan_borrowed_texture_replacement_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_BORROWED_TEXTURE_REPLACEMENT, NULL
  );
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_logical_size(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_REPLACEMENT(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_logical_size(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_REPLACEMENT(self)
  );
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_borrowed_texture_replacement_properties
                        [VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_LOGICAL_SIZE]
    );
}

guint32
maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_physical_width(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_REPLACEMENT(self), 0
  );
  return self->physical_width;
}

void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_width(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_REPLACEMENT(self)
  );
  gboolean changed = self->physical_width != value;
  self->physical_width = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      vulkan_borrowed_texture_replacement_properties
        [VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_WIDTH]
    );
}

guint32
maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_physical_height(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_REPLACEMENT(self), 0
  );
  return self->physical_height;
}

void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_physical_height(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self, guint32 value
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_REPLACEMENT(self)
  );
  gboolean changed = self->physical_height != value;
  self->physical_height = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      vulkan_borrowed_texture_replacement_properties
        [VULKAN_BORROWED_TEXTURE_REPLACEMENT_PROP_PHYSICAL_HEIGHT]
    );
}

GPtrArray* maplibre_native_ffi_vulkan_borrowed_texture_replacement_get_textures(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_REPLACEMENT(self), NULL
  );
  return self->textures;
}

void maplibre_native_ffi_vulkan_borrowed_texture_replacement_set_textures(
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* self,
  const MaplibreNativeFfiVulkanBorrowedTexture* value, gsize value_count
) {
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_REPLACEMENT(self)
  );
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
  g_clear_object(&self->context);
  g_free(self);
}

MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_device_context_result_get_context(
  const MaplibreNativeFfiVulkanDeviceContextResult* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return self->context;
}

MaplibreNativeFfiVulkanDeviceContextResult*
maplibre_native_ffi_vulkan_device_context_result_from_native(
  const mln_vulkan_device_context_result* in
) {
  MaplibreNativeFfiVulkanDeviceContextResult* self =
    maplibre_native_ffi_vulkan_device_context_result_new();
  g_clear_object(&self->context);
  self->context =
    maplibre_native_ffi_vulkan_context_descriptor_from_native(&in->context);
  return self;
}

struct _MaplibreNativeFfiVulkanDeviceOptions {
  GObject parent_instance;
  guint64 fields;
  gboolean presentation;
  GBytes* device_uuid;
  GBytes* device_luid;
};

enum {
  VULKAN_DEVICE_OPTIONS_PROP_0,
  VULKAN_DEVICE_OPTIONS_PROP_PRESENTATION,
  VULKAN_DEVICE_OPTIONS_PROP_DEVICE_UUID,
  VULKAN_DEVICE_OPTIONS_PROP_DEVICE_LUID,
  VULKAN_DEVICE_OPTIONS_N_PROPERTIES
};

static GParamSpec*
  vulkan_device_options_properties[VULKAN_DEVICE_OPTIONS_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiVulkanDeviceOptions,
  maplibre_native_ffi_vulkan_device_options, G_TYPE_OBJECT
)

static void maplibre_native_ffi_vulkan_device_options_finalize(
  GObject* object
) {
  MaplibreNativeFfiVulkanDeviceOptions* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_DEVICE_OPTIONS(object);
  g_clear_pointer(&self->device_uuid, g_bytes_unref);
  g_clear_pointer(&self->device_luid, g_bytes_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_vulkan_device_options_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_vulkan_device_options_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiVulkanDeviceOptions* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_DEVICE_OPTIONS(object);
  switch (id) {
    case VULKAN_DEVICE_OPTIONS_PROP_PRESENTATION:
      g_value_set_boolean(value, self->presentation);
      break;
    case VULKAN_DEVICE_OPTIONS_PROP_DEVICE_UUID:
      g_value_set_boxed(value, self->device_uuid);
      break;
    case VULKAN_DEVICE_OPTIONS_PROP_DEVICE_LUID:
      g_value_set_boxed(value, self->device_luid);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_vulkan_device_options_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiVulkanDeviceOptions* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_DEVICE_OPTIONS(object);
  switch (id) {
    case VULKAN_DEVICE_OPTIONS_PROP_PRESENTATION:
      maplibre_native_ffi_vulkan_device_options_set_presentation(
        self, g_value_get_boolean(value)
      );
      break;
    case VULKAN_DEVICE_OPTIONS_PROP_DEVICE_UUID:
      maplibre_native_ffi_vulkan_device_options_set_device_uuid(
        self, g_value_get_boxed(value)
      );
      break;
    case VULKAN_DEVICE_OPTIONS_PROP_DEVICE_LUID:
      maplibre_native_ffi_vulkan_device_options_set_device_luid(
        self, g_value_get_boxed(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_vulkan_device_options_class_init(
  MaplibreNativeFfiVulkanDeviceOptionsClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = maplibre_native_ffi_vulkan_device_options_finalize;
  object_class->get_property =
    maplibre_native_ffi_vulkan_device_options_get_property;
  object_class->set_property =
    maplibre_native_ffi_vulkan_device_options_set_property;
  vulkan_device_options_properties[VULKAN_DEVICE_OPTIONS_PROP_PRESENTATION] =
    g_param_spec_boolean("presentation", NULL, NULL, FALSE, PROPERTY_FLAGS);
  vulkan_device_options_properties[VULKAN_DEVICE_OPTIONS_PROP_DEVICE_UUID] =
    g_param_spec_boxed("device-uuid", NULL, NULL, G_TYPE_BYTES, PROPERTY_FLAGS);
  vulkan_device_options_properties[VULKAN_DEVICE_OPTIONS_PROP_DEVICE_LUID] =
    g_param_spec_boxed("device-luid", NULL, NULL, G_TYPE_BYTES, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, VULKAN_DEVICE_OPTIONS_N_PROPERTIES,
    vulkan_device_options_properties
  );
}

static void maplibre_native_ffi_vulkan_device_options_init(
  MaplibreNativeFfiVulkanDeviceOptions* self
) {
  (void)self;
}

MaplibreNativeFfiVulkanDeviceOptions*
maplibre_native_ffi_vulkan_device_options_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_DEVICE_OPTIONS, NULL);
}

gboolean maplibre_native_ffi_vulkan_device_options_get_presentation(
  MaplibreNativeFfiVulkanDeviceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_DEVICE_OPTIONS(self), FALSE
  );
  return self->presentation;
}

gboolean maplibre_native_ffi_vulkan_device_options_has_presentation(
  MaplibreNativeFfiVulkanDeviceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_DEVICE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_VULKAN_DEVICE_OPTION_PRESENTATION) != 0;
}

void maplibre_native_ffi_vulkan_device_options_set_presentation(
  MaplibreNativeFfiVulkanDeviceOptions* self, gboolean value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_DEVICE_OPTIONS(self));
  gboolean was_set =
    (self->fields & MLN_VULKAN_DEVICE_OPTION_PRESENTATION) != 0;
  gboolean changed = self->presentation != value;
  self->presentation = value;
  self->fields |= MLN_VULKAN_DEVICE_OPTION_PRESENTATION;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      vulkan_device_options_properties[VULKAN_DEVICE_OPTIONS_PROP_PRESENTATION]
    );
}

GBytes* maplibre_native_ffi_vulkan_device_options_get_device_uuid(
  MaplibreNativeFfiVulkanDeviceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_DEVICE_OPTIONS(self), NULL
  );
  return self->device_uuid;
}

gboolean maplibre_native_ffi_vulkan_device_options_has_device_uuid(
  MaplibreNativeFfiVulkanDeviceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_DEVICE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_VULKAN_DEVICE_OPTION_DEVICE_UUID) != 0;
}

void maplibre_native_ffi_vulkan_device_options_set_device_uuid(
  MaplibreNativeFfiVulkanDeviceOptions* self, GBytes* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_DEVICE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_VULKAN_DEVICE_OPTION_DEVICE_UUID) != 0;
  GBytes* previous = self->device_uuid;
  self->device_uuid = value != NULL ? g_bytes_ref(value) : NULL;
  gboolean changed =
    !maplibre_native_ffi_bytes_equal(previous, self->device_uuid);
  if (previous != NULL) g_bytes_unref(previous);
  self->fields |= MLN_VULKAN_DEVICE_OPTION_DEVICE_UUID;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      vulkan_device_options_properties[VULKAN_DEVICE_OPTIONS_PROP_DEVICE_UUID]
    );
}

GBytes* maplibre_native_ffi_vulkan_device_options_get_device_luid(
  MaplibreNativeFfiVulkanDeviceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_DEVICE_OPTIONS(self), NULL
  );
  return self->device_luid;
}

gboolean maplibre_native_ffi_vulkan_device_options_has_device_luid(
  MaplibreNativeFfiVulkanDeviceOptions* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_DEVICE_OPTIONS(self), FALSE
  );
  return (self->fields & MLN_VULKAN_DEVICE_OPTION_DEVICE_LUID) != 0;
}

void maplibre_native_ffi_vulkan_device_options_set_device_luid(
  MaplibreNativeFfiVulkanDeviceOptions* self, GBytes* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_DEVICE_OPTIONS(self));
  gboolean was_set = (self->fields & MLN_VULKAN_DEVICE_OPTION_DEVICE_LUID) != 0;
  GBytes* previous = self->device_luid;
  self->device_luid = value != NULL ? g_bytes_ref(value) : NULL;
  gboolean changed =
    !maplibre_native_ffi_bytes_equal(previous, self->device_luid);
  if (previous != NULL) g_bytes_unref(previous);
  self->fields |= MLN_VULKAN_DEVICE_OPTION_DEVICE_LUID;
  if (changed || !was_set)
    g_object_notify_by_pspec(
      G_OBJECT(self),
      vulkan_device_options_properties[VULKAN_DEVICE_OPTIONS_PROP_DEVICE_LUID]
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

struct _MaplibreNativeFfiVulkanSurfaceDescriptor {
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  MaplibreNativeFfiVulkanContextDescriptor* context;
  guint64 surface;
};

enum {
  VULKAN_SURFACE_DESCRIPTOR_PROP_0,
  VULKAN_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE,
  VULKAN_SURFACE_DESCRIPTOR_PROP_CONTEXT,
  VULKAN_SURFACE_DESCRIPTOR_PROP_SURFACE,
  VULKAN_SURFACE_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec*
  vulkan_surface_descriptor_properties[VULKAN_SURFACE_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiVulkanSurfaceDescriptor,
  maplibre_native_ffi_vulkan_surface_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_vulkan_surface_descriptor_finalize(
  GObject* object
) {
  MaplibreNativeFfiVulkanSurfaceDescriptor* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_SURFACE_DESCRIPTOR(object);
  g_clear_object(&self->context);
  G_OBJECT_CLASS(maplibre_native_ffi_vulkan_surface_descriptor_parent_class)
    ->finalize(object);
}

static void maplibre_native_ffi_vulkan_surface_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiVulkanSurfaceDescriptor* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_SURFACE_DESCRIPTOR(object);
  switch (id) {
    case VULKAN_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case VULKAN_SURFACE_DESCRIPTOR_PROP_CONTEXT:
      g_value_set_object(value, self->context);
      break;
    case VULKAN_SURFACE_DESCRIPTOR_PROP_SURFACE:
      g_value_set_uint64(value, self->surface);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_vulkan_surface_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiVulkanSurfaceDescriptor* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_SURFACE_DESCRIPTOR(object);
  switch (id) {
    case VULKAN_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_vulkan_surface_descriptor_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case VULKAN_SURFACE_DESCRIPTOR_PROP_CONTEXT:
      maplibre_native_ffi_vulkan_surface_descriptor_set_context(
        self, g_value_get_object(value)
      );
      break;
    case VULKAN_SURFACE_DESCRIPTOR_PROP_SURFACE:
      maplibre_native_ffi_vulkan_surface_descriptor_set_surface(
        self, g_value_get_uint64(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_vulkan_surface_descriptor_class_init(
  MaplibreNativeFfiVulkanSurfaceDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize =
    maplibre_native_ffi_vulkan_surface_descriptor_finalize;
  object_class->get_property =
    maplibre_native_ffi_vulkan_surface_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_vulkan_surface_descriptor_set_property;
  vulkan_surface_descriptor_properties
    [VULKAN_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE] = g_param_spec_boxed(
      "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
      PROPERTY_FLAGS
    );
  vulkan_surface_descriptor_properties[VULKAN_SURFACE_DESCRIPTOR_PROP_CONTEXT] =
    g_param_spec_object(
      "context", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_CONTEXT_DESCRIPTOR,
      PROPERTY_FLAGS
    );
  vulkan_surface_descriptor_properties[VULKAN_SURFACE_DESCRIPTOR_PROP_SURFACE] =
    g_param_spec_uint64(
      "surface", NULL, NULL, 0, G_MAXUINT64, 0, PROPERTY_FLAGS
    );
  g_object_class_install_properties(
    object_class, VULKAN_SURFACE_DESCRIPTOR_N_PROPERTIES,
    vulkan_surface_descriptor_properties
  );
}

static void maplibre_native_ffi_vulkan_surface_descriptor_init(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self
) {
  self->context = maplibre_native_ffi_vulkan_context_descriptor_new();
}

MaplibreNativeFfiVulkanSurfaceDescriptor*
maplibre_native_ffi_vulkan_surface_descriptor_new(void) {
  return g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_SURFACE_DESCRIPTOR, NULL);
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_vulkan_surface_descriptor_get_logical_size(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_SURFACE_DESCRIPTOR(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_vulkan_surface_descriptor_set_logical_size(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_SURFACE_DESCRIPTOR(self));
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_surface_descriptor_properties
                        [VULKAN_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE]
    );
}

MaplibreNativeFfiVulkanContextDescriptor*
maplibre_native_ffi_vulkan_surface_descriptor_get_context(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_SURFACE_DESCRIPTOR(self), NULL
  );
  return self->context;
}

void maplibre_native_ffi_vulkan_surface_descriptor_set_context(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self,
  MaplibreNativeFfiVulkanContextDescriptor* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_SURFACE_DESCRIPTOR(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_CONTEXT_DESCRIPTOR(value));
  MaplibreNativeFfiVulkanContextDescriptor* previous = self->context;
  self->context = maplibre_native_ffi_vulkan_context_descriptor_copy(value);
  if (previous != NULL) g_object_unref(previous);
  g_object_notify_by_pspec(
    G_OBJECT(self),
    vulkan_surface_descriptor_properties[VULKAN_SURFACE_DESCRIPTOR_PROP_CONTEXT]
  );
}

guint64 maplibre_native_ffi_vulkan_surface_descriptor_get_surface(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_SURFACE_DESCRIPTOR(self), 0
  );
  return self->surface;
}

void maplibre_native_ffi_vulkan_surface_descriptor_set_surface(
  MaplibreNativeFfiVulkanSurfaceDescriptor* self, guint64 value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_SURFACE_DESCRIPTOR(self));
  gboolean changed = self->surface != value;
  self->surface = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), vulkan_surface_descriptor_properties
                        [VULKAN_SURFACE_DESCRIPTOR_PROP_SURFACE]
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
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  gpointer display;
  gpointer surface;
};

enum {
  WAYLAND_SURFACE_DESCRIPTOR_PROP_0,
  WAYLAND_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE,
  WAYLAND_SURFACE_DESCRIPTOR_PROP_DISPLAY,
  WAYLAND_SURFACE_DESCRIPTOR_PROP_SURFACE,
  WAYLAND_SURFACE_DESCRIPTOR_N_PROPERTIES
};

static GParamSpec* wayland_surface_descriptor_properties
  [WAYLAND_SURFACE_DESCRIPTOR_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiWaylandSurfaceDescriptor,
  maplibre_native_ffi_wayland_surface_descriptor, G_TYPE_OBJECT
)

static void maplibre_native_ffi_wayland_surface_descriptor_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiWaylandSurfaceDescriptor* self =
    MAPLIBRE_NATIVE_FFI_WAYLAND_SURFACE_DESCRIPTOR(object);
  switch (id) {
    case WAYLAND_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case WAYLAND_SURFACE_DESCRIPTOR_PROP_DISPLAY:
      g_value_set_pointer(value, self->display);
      break;
    case WAYLAND_SURFACE_DESCRIPTOR_PROP_SURFACE:
      g_value_set_pointer(value, self->surface);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_wayland_surface_descriptor_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiWaylandSurfaceDescriptor* self =
    MAPLIBRE_NATIVE_FFI_WAYLAND_SURFACE_DESCRIPTOR(object);
  switch (id) {
    case WAYLAND_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_wayland_surface_descriptor_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case WAYLAND_SURFACE_DESCRIPTOR_PROP_DISPLAY:
      maplibre_native_ffi_wayland_surface_descriptor_set_display(
        self, g_value_get_pointer(value)
      );
      break;
    case WAYLAND_SURFACE_DESCRIPTOR_PROP_SURFACE:
      maplibre_native_ffi_wayland_surface_descriptor_set_surface(
        self, g_value_get_pointer(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_wayland_surface_descriptor_class_init(
  MaplibreNativeFfiWaylandSurfaceDescriptorClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_wayland_surface_descriptor_get_property;
  object_class->set_property =
    maplibre_native_ffi_wayland_surface_descriptor_set_property;
  wayland_surface_descriptor_properties
    [WAYLAND_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE] = g_param_spec_boxed(
      "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
      PROPERTY_FLAGS
    );
  wayland_surface_descriptor_properties
    [WAYLAND_SURFACE_DESCRIPTOR_PROP_DISPLAY] =
      g_param_spec_pointer("display", NULL, NULL, PROPERTY_FLAGS);
  wayland_surface_descriptor_properties
    [WAYLAND_SURFACE_DESCRIPTOR_PROP_SURFACE] =
      g_param_spec_pointer("surface", NULL, NULL, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, WAYLAND_SURFACE_DESCRIPTOR_N_PROPERTIES,
    wayland_surface_descriptor_properties
  );
}

static void maplibre_native_ffi_wayland_surface_descriptor_init(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self
) {
  (void)self;
}

MaplibreNativeFfiWaylandSurfaceDescriptor*
maplibre_native_ffi_wayland_surface_descriptor_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_WAYLAND_SURFACE_DESCRIPTOR, NULL
  );
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_wayland_surface_descriptor_get_logical_size(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_WAYLAND_SURFACE_DESCRIPTOR(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_wayland_surface_descriptor_set_logical_size(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_WAYLAND_SURFACE_DESCRIPTOR(self));
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), wayland_surface_descriptor_properties
                        [WAYLAND_SURFACE_DESCRIPTOR_PROP_LOGICAL_SIZE]
    );
}

gpointer maplibre_native_ffi_wayland_surface_descriptor_get_display(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_WAYLAND_SURFACE_DESCRIPTOR(self), 0
  );
  return self->display;
}

void maplibre_native_ffi_wayland_surface_descriptor_set_display(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_WAYLAND_SURFACE_DESCRIPTOR(self));
  gboolean changed = self->display != value;
  self->display = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), wayland_surface_descriptor_properties
                        [WAYLAND_SURFACE_DESCRIPTOR_PROP_DISPLAY]
    );
}

gpointer maplibre_native_ffi_wayland_surface_descriptor_get_surface(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_WAYLAND_SURFACE_DESCRIPTOR(self), 0
  );
  return self->surface;
}

void maplibre_native_ffi_wayland_surface_descriptor_set_surface(
  MaplibreNativeFfiWaylandSurfaceDescriptor* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_WAYLAND_SURFACE_DESCRIPTOR(self));
  gboolean changed = self->surface != value;
  self->surface = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), wayland_surface_descriptor_properties
                        [WAYLAND_SURFACE_DESCRIPTOR_PROP_SURFACE]
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

struct _MaplibreNativeFfiWaylandSurfaceReplacement {
  GObject parent_instance;
  MaplibreNativeFfiLogicalSize logical_size;
  gpointer surface;
};

enum {
  WAYLAND_SURFACE_REPLACEMENT_PROP_0,
  WAYLAND_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE,
  WAYLAND_SURFACE_REPLACEMENT_PROP_SURFACE,
  WAYLAND_SURFACE_REPLACEMENT_N_PROPERTIES
};

static GParamSpec* wayland_surface_replacement_properties
  [WAYLAND_SURFACE_REPLACEMENT_N_PROPERTIES];

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiWaylandSurfaceReplacement,
  maplibre_native_ffi_wayland_surface_replacement, G_TYPE_OBJECT
)

static void maplibre_native_ffi_wayland_surface_replacement_get_property(
  GObject* object, guint id, GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiWaylandSurfaceReplacement* self =
    MAPLIBRE_NATIVE_FFI_WAYLAND_SURFACE_REPLACEMENT(object);
  switch (id) {
    case WAYLAND_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE:
      g_value_set_boxed(value, &self->logical_size);
      break;
    case WAYLAND_SURFACE_REPLACEMENT_PROP_SURFACE:
      g_value_set_pointer(value, self->surface);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_wayland_surface_replacement_set_property(
  GObject* object, guint id, const GValue* value, GParamSpec* pspec
) {
  MaplibreNativeFfiWaylandSurfaceReplacement* self =
    MAPLIBRE_NATIVE_FFI_WAYLAND_SURFACE_REPLACEMENT(object);
  switch (id) {
    case WAYLAND_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE:
      maplibre_native_ffi_wayland_surface_replacement_set_logical_size(
        self, g_value_get_boxed(value)
      );
      break;
    case WAYLAND_SURFACE_REPLACEMENT_PROP_SURFACE:
      maplibre_native_ffi_wayland_surface_replacement_set_surface(
        self, g_value_get_pointer(value)
      );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, pspec);
  }
}

static void maplibre_native_ffi_wayland_surface_replacement_class_init(
  MaplibreNativeFfiWaylandSurfaceReplacementClass* klass
) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->get_property =
    maplibre_native_ffi_wayland_surface_replacement_get_property;
  object_class->set_property =
    maplibre_native_ffi_wayland_surface_replacement_set_property;
  wayland_surface_replacement_properties
    [WAYLAND_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE] = g_param_spec_boxed(
      "logical-size", NULL, NULL, MAPLIBRE_NATIVE_FFI_TYPE_LOGICAL_SIZE,
      PROPERTY_FLAGS
    );
  wayland_surface_replacement_properties
    [WAYLAND_SURFACE_REPLACEMENT_PROP_SURFACE] =
      g_param_spec_pointer("surface", NULL, NULL, PROPERTY_FLAGS);
  g_object_class_install_properties(
    object_class, WAYLAND_SURFACE_REPLACEMENT_N_PROPERTIES,
    wayland_surface_replacement_properties
  );
}

static void maplibre_native_ffi_wayland_surface_replacement_init(
  MaplibreNativeFfiWaylandSurfaceReplacement* self
) {
  (void)self;
}

MaplibreNativeFfiWaylandSurfaceReplacement*
maplibre_native_ffi_wayland_surface_replacement_new(void) {
  return g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_WAYLAND_SURFACE_REPLACEMENT, NULL
  );
}

MaplibreNativeFfiLogicalSize*
maplibre_native_ffi_wayland_surface_replacement_get_logical_size(
  MaplibreNativeFfiWaylandSurfaceReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_WAYLAND_SURFACE_REPLACEMENT(self), NULL
  );
  return (MaplibreNativeFfiLogicalSize*)&self->logical_size;
}

void maplibre_native_ffi_wayland_surface_replacement_set_logical_size(
  MaplibreNativeFfiWaylandSurfaceReplacement* self,
  const MaplibreNativeFfiLogicalSize* value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_WAYLAND_SURFACE_REPLACEMENT(self));
  g_return_if_fail(value != NULL);
  gboolean changed =
    memcmp(&self->logical_size, value, sizeof self->logical_size) != 0;
  self->logical_size = *value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), wayland_surface_replacement_properties
                        [WAYLAND_SURFACE_REPLACEMENT_PROP_LOGICAL_SIZE]
    );
}

gpointer maplibre_native_ffi_wayland_surface_replacement_get_surface(
  MaplibreNativeFfiWaylandSurfaceReplacement* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_WAYLAND_SURFACE_REPLACEMENT(self), 0
  );
  return self->surface;
}

void maplibre_native_ffi_wayland_surface_replacement_set_surface(
  MaplibreNativeFfiWaylandSurfaceReplacement* self, gpointer value
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_WAYLAND_SURFACE_REPLACEMENT(self));
  gboolean changed = self->surface != value;
  self->surface = value;
  if (changed)
    g_object_notify_by_pspec(
      G_OBJECT(self), wayland_surface_replacement_properties
                        [WAYLAND_SURFACE_REPLACEMENT_PROP_SURFACE]
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
