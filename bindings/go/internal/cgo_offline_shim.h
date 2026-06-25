#ifndef MLN_GO_CGO_OFFLINE_SHIM_H
#define MLN_GO_CGO_OFFLINE_SHIM_H

#include "cgo_geometry_shim.h"
#include "maplibre_native_c.h"

static inline mln_offline_region_definition
mln_go_offline_tile_pyramid_region_definition(
  const char* style_url, mln_lat_lng_bounds bounds, double min_zoom,
  double max_zoom, float pixel_ratio, bool include_ideographs
) {
  mln_offline_region_definition definition;
  definition.size = sizeof(mln_offline_region_definition);
  definition.type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
  definition.data.tile_pyramid.size =
    sizeof(mln_offline_tile_pyramid_region_definition);
  definition.data.tile_pyramid.style_url = style_url;
  definition.data.tile_pyramid.bounds = bounds;
  definition.data.tile_pyramid.min_zoom = min_zoom;
  definition.data.tile_pyramid.max_zoom = max_zoom;
  definition.data.tile_pyramid.pixel_ratio = pixel_ratio;
  definition.data.tile_pyramid.include_ideographs = include_ideographs;
  return definition;
}

static inline mln_offline_region_definition
mln_go_offline_geometry_region_definition(
  const char* style_url, const mln_geometry* geometry, double min_zoom,
  double max_zoom, float pixel_ratio, bool include_ideographs
) {
  mln_offline_region_definition definition;
  definition.size = sizeof(mln_offline_region_definition);
  definition.type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
  definition.data.geometry.size =
    sizeof(mln_offline_geometry_region_definition);
  definition.data.geometry.style_url = style_url;
  definition.data.geometry.geometry = geometry;
  definition.data.geometry.min_zoom = min_zoom;
  definition.data.geometry.max_zoom = max_zoom;
  definition.data.geometry.pixel_ratio = pixel_ratio;
  definition.data.geometry.include_ideographs = include_ideographs;
  return definition;
}

static inline uint32_t mln_go_offline_region_info_definition_type(
  const mln_offline_region_info* info
) {
  return info->definition.type;
}

static inline uint32_t mln_go_offline_region_definition_type(
  const mln_offline_region_definition* definition
) {
  return definition->type;
}

static inline mln_offline_tile_pyramid_region_definition
mln_go_offline_region_info_tile_pyramid(const mln_offline_region_info* info) {
  return info->definition.data.tile_pyramid;
}

static inline mln_offline_tile_pyramid_region_definition
mln_go_offline_region_definition_tile_pyramid(
  const mln_offline_region_definition* definition
) {
  return definition->data.tile_pyramid;
}

static inline mln_offline_geometry_region_definition
mln_go_offline_region_info_geometry(const mln_offline_region_info* info) {
  return info->definition.data.geometry;
}

static inline mln_offline_geometry_region_definition
mln_go_offline_region_definition_geometry(
  const mln_offline_region_definition* definition
) {
  return definition->data.geometry;
}

#endif
