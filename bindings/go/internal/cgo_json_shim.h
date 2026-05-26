#ifndef MLN_GO_CGO_JSON_SHIM_H
#define MLN_GO_CGO_JSON_SHIM_H

#include "maplibre_native_c.h"

static inline mln_json_value mln_go_json_null(void) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_NULL;
  return value;
}

static inline mln_json_value mln_go_json_bool(bool raw) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_BOOL;
  value.data.bool_value = raw;
  return value;
}

static inline mln_json_value mln_go_json_uint(uint64_t raw) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_UINT;
  value.data.uint_value = raw;
  return value;
}

static inline mln_json_value mln_go_json_int(int64_t raw) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_INT;
  value.data.int_value = raw;
  return value;
}

static inline mln_json_value mln_go_json_double(double raw) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_DOUBLE;
  value.data.double_value = raw;
  return value;
}

static inline mln_json_value mln_go_json_string(mln_string_view raw) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_STRING;
  value.data.string_value = raw;
  return value;
}

static inline mln_json_value mln_go_json_array(
  const mln_json_value* values, size_t count
) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_ARRAY;
  value.data.array_value.values = values;
  value.data.array_value.value_count = count;
  return value;
}

static inline mln_json_member mln_go_json_member(
  mln_string_view key, const mln_json_value* raw
) {
  mln_json_member member;
  member.key = key;
  member.value = raw;
  return member;
}

static inline mln_json_value mln_go_json_object(
  const mln_json_member* members, size_t count
) {
  mln_json_value value;
  value.size = sizeof(mln_json_value);
  value.type = MLN_JSON_VALUE_TYPE_OBJECT;
  value.data.object_value.members = members;
  value.data.object_value.member_count = count;
  return value;
}

static inline uint32_t mln_go_json_type(const mln_json_value* value) {
  return value->type;
}
static inline bool mln_go_json_bool_value(const mln_json_value* value) {
  return value->data.bool_value;
}
static inline uint64_t mln_go_json_uint_value(const mln_json_value* value) {
  return value->data.uint_value;
}
static inline int64_t mln_go_json_int_value(const mln_json_value* value) {
  return value->data.int_value;
}
static inline double mln_go_json_double_value(const mln_json_value* value) {
  return value->data.double_value;
}
static inline mln_string_view mln_go_json_string_value(
  const mln_json_value* value
) {
  return value->data.string_value;
}
static inline size_t mln_go_json_array_count(const mln_json_value* value) {
  return value->data.array_value.value_count;
}
static inline const mln_json_value* mln_go_json_array_get(
  const mln_json_value* value, size_t index
) {
  return &value->data.array_value.values[index];
}
static inline size_t mln_go_json_object_count(const mln_json_value* value) {
  return value->data.object_value.member_count;
}
static inline mln_string_view mln_go_json_object_key(
  const mln_json_value* value, size_t index
) {
  return value->data.object_value.members[index].key;
}
static inline const mln_json_value* mln_go_json_object_value(
  const mln_json_value* value, size_t index
) {
  return value->data.object_value.members[index].value;
}

static inline mln_feature mln_go_feature_null(
  const mln_geometry* geometry, const mln_json_member* properties, size_t count
) {
  mln_feature feature;
  feature.size = sizeof(mln_feature);
  feature.geometry = geometry;
  feature.properties = properties;
  feature.property_count = count;
  feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_NULL;
  return feature;
}

static inline mln_feature mln_go_feature_uint(
  const mln_geometry* geometry, const mln_json_member* properties, size_t count,
  uint64_t id
) {
  mln_feature feature = mln_go_feature_null(geometry, properties, count);
  feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_UINT;
  feature.identifier.uint_value = id;
  return feature;
}

static inline mln_feature mln_go_feature_int(
  const mln_geometry* geometry, const mln_json_member* properties, size_t count,
  int64_t id
) {
  mln_feature feature = mln_go_feature_null(geometry, properties, count);
  feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_INT;
  feature.identifier.int_value = id;
  return feature;
}

static inline mln_feature mln_go_feature_double(
  const mln_geometry* geometry, const mln_json_member* properties, size_t count,
  double id
) {
  mln_feature feature = mln_go_feature_null(geometry, properties, count);
  feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE;
  feature.identifier.double_value = id;
  return feature;
}

static inline mln_feature mln_go_feature_string(
  const mln_geometry* geometry, const mln_json_member* properties, size_t count,
  mln_string_view id
) {
  mln_feature feature = mln_go_feature_null(geometry, properties, count);
  feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_STRING;
  feature.identifier.string_value = id;
  return feature;
}

static inline mln_geojson mln_go_geojson_geometry(
  const mln_geometry* geometry
) {
  mln_geojson geojson;
  geojson.size = sizeof(mln_geojson);
  geojson.type = MLN_GEOJSON_TYPE_GEOMETRY;
  geojson.data.geometry = geometry;
  return geojson;
}

static inline mln_geojson mln_go_geojson_feature(const mln_feature* feature) {
  mln_geojson geojson;
  geojson.size = sizeof(mln_geojson);
  geojson.type = MLN_GEOJSON_TYPE_FEATURE;
  geojson.data.feature = feature;
  return geojson;
}

static inline mln_geojson mln_go_geojson_feature_collection(
  const mln_feature* features, size_t count
) {
  mln_geojson geojson;
  geojson.size = sizeof(mln_geojson);
  geojson.type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION;
  geojson.data.feature_collection.features = features;
  geojson.data.feature_collection.feature_count = count;
  return geojson;
}

static inline const mln_geometry* mln_go_feature_geometry(
  const mln_feature* feature
) {
  return feature->geometry;
}
static inline size_t mln_go_feature_property_count(const mln_feature* feature) {
  return feature->property_count;
}
static inline mln_string_view mln_go_feature_property_key(
  const mln_feature* feature, size_t index
) {
  return feature->properties[index].key;
}
static inline const mln_json_value* mln_go_feature_property_value(
  const mln_feature* feature, size_t index
) {
  return feature->properties[index].value;
}
static inline uint32_t mln_go_feature_identifier_type(
  const mln_feature* feature
) {
  return feature->identifier_type;
}
static inline uint64_t mln_go_feature_identifier_uint(
  const mln_feature* feature
) {
  return feature->identifier.uint_value;
}
static inline int64_t mln_go_feature_identifier_int(
  const mln_feature* feature
) {
  return feature->identifier.int_value;
}
static inline double mln_go_feature_identifier_double(
  const mln_feature* feature
) {
  return feature->identifier.double_value;
}
static inline mln_string_view mln_go_feature_identifier_string(
  const mln_feature* feature
) {
  return feature->identifier.string_value;
}

static inline uint32_t mln_go_feature_extension_result_type(
  const mln_feature_extension_result_info* info
) {
  return info->type;
}
static inline const mln_json_value* mln_go_feature_extension_result_value(
  const mln_feature_extension_result_info* info
) {
  return info->data.value;
}
static inline size_t mln_go_feature_extension_result_feature_count(
  const mln_feature_extension_result_info* info
) {
  return info->data.feature_collection.feature_count;
}
static inline const mln_feature* mln_go_feature_extension_result_feature_get(
  const mln_feature_extension_result_info* info, size_t index
) {
  return &info->data.feature_collection.features[index];
}

#endif
