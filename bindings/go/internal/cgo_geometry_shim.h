#ifndef MLN_GO_CGO_GEOMETRY_SHIM_H
#define MLN_GO_CGO_GEOMETRY_SHIM_H

#include "maplibre_native_c.h"

static inline mln_coordinate_span mln_go_coordinate_span(
  const mln_lat_lng* coordinates, size_t count
) {
  mln_coordinate_span span;
  span.coordinates = coordinates;
  span.coordinate_count = count;
  return span;
}

static inline mln_geometry mln_go_geometry_empty(void) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_EMPTY;
  return geometry;
}

static inline mln_geometry mln_go_geometry_point(mln_lat_lng point) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_POINT;
  geometry.data.point = point;
  return geometry;
}

static inline mln_geometry mln_go_geometry_line_string(
  mln_coordinate_span span
) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_LINE_STRING;
  geometry.data.line_string = span;
  return geometry;
}

static inline mln_geometry mln_go_geometry_polygon(
  const mln_coordinate_span* rings, size_t count
) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_POLYGON;
  geometry.data.polygon.rings = rings;
  geometry.data.polygon.ring_count = count;
  return geometry;
}

static inline mln_geometry mln_go_geometry_multi_point(
  mln_coordinate_span span
) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_MULTI_POINT;
  geometry.data.multi_point = span;
  return geometry;
}

static inline mln_geometry mln_go_geometry_multi_line_string(
  const mln_coordinate_span* lines, size_t count
) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_MULTI_LINE_STRING;
  geometry.data.multi_line_string.lines = lines;
  geometry.data.multi_line_string.line_count = count;
  return geometry;
}

static inline mln_polygon_geometry mln_go_polygon_geometry(
  const mln_coordinate_span* rings, size_t count
) {
  mln_polygon_geometry polygon;
  polygon.rings = rings;
  polygon.ring_count = count;
  return polygon;
}

static inline mln_geometry mln_go_geometry_multi_polygon(
  const mln_polygon_geometry* polygons, size_t count
) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_MULTI_POLYGON;
  geometry.data.multi_polygon.polygons = polygons;
  geometry.data.multi_polygon.polygon_count = count;
  return geometry;
}

static inline mln_geometry mln_go_geometry_collection(
  const mln_geometry* geometries, size_t count
) {
  mln_geometry geometry;
  geometry.size = sizeof(mln_geometry);
  geometry.type = MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION;
  geometry.data.geometry_collection.geometries = geometries;
  geometry.data.geometry_collection.geometry_count = count;
  return geometry;
}

static inline uint32_t mln_go_geometry_type(const mln_geometry* geometry) {
  return geometry->type;
}
static inline mln_lat_lng mln_go_geometry_point_value(
  const mln_geometry* geometry
) {
  return geometry->data.point;
}
static inline size_t mln_go_coordinate_span_count(mln_coordinate_span span) {
  return span.coordinate_count;
}
static inline mln_lat_lng mln_go_coordinate_span_get(
  mln_coordinate_span span, size_t index
) {
  return span.coordinates[index];
}
static inline mln_coordinate_span mln_go_geometry_line_string_value(
  const mln_geometry* geometry
) {
  return geometry->data.line_string;
}
static inline mln_coordinate_span mln_go_geometry_multi_point_value(
  const mln_geometry* geometry
) {
  return geometry->data.multi_point;
}
static inline size_t mln_go_geometry_polygon_ring_count(
  const mln_geometry* geometry
) {
  return geometry->data.polygon.ring_count;
}
static inline mln_coordinate_span mln_go_geometry_polygon_ring_get(
  const mln_geometry* geometry, size_t index
) {
  return geometry->data.polygon.rings[index];
}
static inline size_t mln_go_geometry_multi_line_count(
  const mln_geometry* geometry
) {
  return geometry->data.multi_line_string.line_count;
}
static inline mln_coordinate_span mln_go_geometry_multi_line_get(
  const mln_geometry* geometry, size_t index
) {
  return geometry->data.multi_line_string.lines[index];
}
static inline size_t mln_go_geometry_multi_polygon_count(
  const mln_geometry* geometry
) {
  return geometry->data.multi_polygon.polygon_count;
}
static inline size_t mln_go_polygon_geometry_ring_count(
  mln_polygon_geometry polygon
) {
  return polygon.ring_count;
}
static inline mln_coordinate_span mln_go_polygon_geometry_ring_get(
  mln_polygon_geometry polygon, size_t index
) {
  return polygon.rings[index];
}
static inline mln_polygon_geometry mln_go_geometry_multi_polygon_get(
  const mln_geometry* geometry, size_t index
) {
  return geometry->data.multi_polygon.polygons[index];
}
static inline size_t mln_go_geometry_collection_count(
  const mln_geometry* geometry
) {
  return geometry->data.geometry_collection.geometry_count;
}
static inline const mln_geometry* mln_go_geometry_collection_get(
  const mln_geometry* geometry, size_t index
) {
  return &geometry->data.geometry_collection.geometries[index];
}

#endif
