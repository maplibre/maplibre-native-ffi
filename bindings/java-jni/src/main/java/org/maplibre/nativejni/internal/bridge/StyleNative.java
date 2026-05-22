package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the StyleNative C API coverage group. */
public final class StyleNative {
  private StyleNative() {}

  public static native int mln_style_tile_source_options_default();

  public static native int mln_custom_geometry_source_options_default();

  public static native int mln_premultiplied_rgba8_image_default();

  public static native int mln_style_image_options_default();

  public static native int mln_style_image_info_default();

  public static native int mln_style_id_list_count();

  public static native int mln_style_id_list_get();

  public static native int mln_style_id_list_destroy();

  public static native int mln_map_add_style_source_json();

  public static native int mln_map_remove_style_source(
      long map, String sourceId, boolean[] outRemoved);

  public static native int mln_map_style_source_exists(
      long map, String sourceId, boolean[] outExists);

  public static native int mln_map_get_style_source_type(
      long map, String sourceId, int[] outSourceType, boolean[] outFound);

  public static native int mln_map_get_style_source_info(
      long map, String sourceId, int[] outInfo, boolean[] outFlags, long[] outSizes);

  public static native int mln_map_copy_style_source_attribution(
      long map,
      String sourceId,
      byte[] outAttribution,
      long[] outAttributionSize,
      boolean[] outFound);

  public static native int mln_map_list_style_source_ids(long map, Object[] outSourceIds);

  public static native int mln_map_add_geojson_source_url(long map, String sourceId, String url);

  public static native int mln_map_add_geojson_source_data();

  public static native int mln_map_set_geojson_source_url(long map, String sourceId, String url);

  public static native int mln_map_set_geojson_source_data();

  public static native int mln_map_add_vector_source_url(long map, String sourceId, String url);

  public static native int mln_map_add_vector_source_tiles(
      long map, String sourceId, String[] tiles);

  public static native int mln_map_add_raster_source_url(long map, String sourceId, String url);

  public static native int mln_map_add_raster_source_tiles(
      long map, String sourceId, String[] tiles);

  public static native int mln_map_add_raster_dem_source_url(long map, String sourceId, String url);

  public static native int mln_map_add_raster_dem_source_tiles(
      long map, String sourceId, String[] tiles);

  public static native int mln_map_add_custom_geometry_source();

  public static native int mln_map_set_custom_geometry_source_tile_data();

  public static native int mln_map_invalidate_custom_geometry_source_tile();

  public static native int mln_map_invalidate_custom_geometry_source_region();

  public static native int mln_map_set_style_image();

  public static native int mln_map_remove_style_image();

  public static native int mln_map_style_image_exists();

  public static native int mln_map_get_style_image_info();

  public static native int mln_map_copy_style_image_premultiplied_rgba8();

  public static native int mln_map_add_image_source_url(
      long map, String sourceId, double[] coordinates, String url);

  public static native int mln_map_add_image_source_image();

  public static native int mln_map_set_image_source_url(long map, String sourceId, String url);

  public static native int mln_map_set_image_source_image();

  public static native int mln_map_set_image_source_coordinates(
      long map, String sourceId, double[] coordinates);

  public static native int mln_map_get_image_source_coordinates(
      long map,
      String sourceId,
      double[] outCoordinates,
      long[] outCoordinateCount,
      boolean[] outFound);

  public static native int mln_map_add_hillshade_layer(
      long map, String layerId, String sourceId, String beforeLayerId);

  public static native int mln_map_add_color_relief_layer(
      long map, String layerId, String sourceId, String beforeLayerId);

  public static native int mln_map_add_location_indicator_layer(
      long map, String layerId, String beforeLayerId);

  public static native int mln_map_set_location_indicator_location(
      long map, String layerId, double latitude, double longitude, double altitude);

  public static native int mln_map_set_location_indicator_bearing(
      long map, String layerId, double bearing);

  public static native int mln_map_set_location_indicator_accuracy_radius(
      long map, String layerId, double radius);

  public static native int mln_map_set_location_indicator_image_name(
      long map, String layerId, int imageKind, String imageId);

  public static native int mln_map_add_style_layer_json();

  public static native int mln_map_remove_style_layer(
      long map, String layerId, boolean[] outRemoved);

  public static native int mln_map_style_layer_exists(
      long map, String layerId, boolean[] outExists);

  public static native int mln_map_get_style_layer_type(
      long map, String layerId, String[] outLayerType, boolean[] outFound);

  public static native int mln_map_list_style_layer_ids(long map, Object[] outLayerIds);

  public static native int mln_map_move_style_layer(long map, String layerId, String beforeLayerId);

  public static native int mln_map_get_style_layer_json();

  public static native int mln_map_set_style_light_json();

  public static native int mln_map_set_style_light_property();

  public static native int mln_map_get_style_light_property();

  public static native int mln_map_set_layer_property();

  public static native int mln_map_get_layer_property();

  public static native int mln_map_set_layer_filter();

  public static native int mln_map_get_layer_filter();
}
