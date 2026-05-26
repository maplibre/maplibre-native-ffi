#ifndef MLN_GO_CGO_OFFLINE_SHIM_H
#define MLN_GO_CGO_OFFLINE_SHIM_H

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

static inline uint32_t mln_go_offline_region_info_definition_type(
  const mln_offline_region_info* info
) {
  return info->definition.type;
}

static inline mln_offline_tile_pyramid_region_definition
mln_go_offline_region_info_tile_pyramid(const mln_offline_region_info* info) {
  return info->definition.data.tile_pyramid;
}

#endif
