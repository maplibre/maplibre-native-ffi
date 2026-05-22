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

  public static native int mln_map_add_style_source_json(
      long map, String sourceId, org.maplibre.nativejni.json.JsonValue sourceJson);

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

  public static native int mln_map_add_geojson_source_data(
      long map, String sourceId, org.maplibre.nativejni.geo.GeoJson data);

  public static native int mln_map_set_geojson_source_url(long map, String sourceId, String url);

  public static native int mln_map_set_geojson_source_data(
      long map, String sourceId, org.maplibre.nativejni.geo.GeoJson data);

  public static native int mln_map_add_vector_source_url(
      long map,
      String sourceId,
      String url,
      boolean[] optionFields,
      double[] optionValues,
      String attribution);

  public static native int mln_map_add_vector_source_tiles(
      long map,
      String sourceId,
      String[] tiles,
      boolean[] optionFields,
      double[] optionValues,
      String attribution);

  public static native int mln_map_add_raster_source_url(
      long map,
      String sourceId,
      String url,
      boolean[] optionFields,
      double[] optionValues,
      String attribution);

  public static native int mln_map_add_raster_source_tiles(
      long map,
      String sourceId,
      String[] tiles,
      boolean[] optionFields,
      double[] optionValues,
      String attribution);

  public static native int mln_map_add_raster_dem_source_url(
      long map,
      String sourceId,
      String url,
      boolean[] optionFields,
      double[] optionValues,
      String attribution);

  public static native int mln_map_add_raster_dem_source_tiles(
      long map,
      String sourceId,
      String[] tiles,
      boolean[] optionFields,
      double[] optionValues,
      String attribution);

  public static native int mln_map_add_custom_geometry_source(
      long map,
      String sourceId,
      org.maplibre.nativejni.style.CustomGeometrySourceCallback callback,
      boolean[] optionFields,
      double[] optionValues,
      long[] outState);

  public static native int mln_map_set_custom_geometry_source_tile_data(
      long map,
      String sourceId,
      int tileZ,
      long tileX,
      long tileY,
      org.maplibre.nativejni.geo.GeoJson data);

  public static native int mln_map_invalidate_custom_geometry_source_tile(
      long map, String sourceId, int tileZ, long tileX, long tileY);

  public static native int mln_map_invalidate_custom_geometry_source_region(
      long map,
      String sourceId,
      double southwestLatitude,
      double southwestLongitude,
      double northeastLatitude,
      double northeastLongitude);

  public static native void mln_custom_geometry_source_state_destroy(long state);

  public static native int mln_map_set_style_image(
      long map,
      String imageId,
      int width,
      int height,
      int stride,
      byte[] pixels,
      boolean hasPixelRatio,
      double pixelRatio,
      boolean hasSdf,
      boolean sdf);

  public static native int mln_map_remove_style_image(
      long map, String imageId, boolean[] outRemoved);

  public static native int mln_map_style_image_exists(
      long map, String imageId, boolean[] outExists);

  public static native int mln_map_get_style_image_info(
      long map,
      String imageId,
      int[] outInfo,
      long[] outByteLength,
      double[] outPixelRatio,
      boolean[] outFlags);

  public static native int mln_map_copy_style_image_premultiplied_rgba8(
      long map, String imageId, byte[] outPixels, long[] outByteLength, boolean[] outFound);

  public static native int mln_map_add_image_source_url(
      long map, String sourceId, double[] coordinates, String url);

  public static native int mln_map_add_image_source_image(
      long map,
      String sourceId,
      double[] coordinates,
      int width,
      int height,
      int stride,
      byte[] pixels);

  public static native int mln_map_set_image_source_url(long map, String sourceId, String url);

  public static native int mln_map_set_image_source_image(
      long map, String sourceId, int width, int height, int stride, byte[] pixels);

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

  public static native int mln_map_add_style_layer_json(
      long map, org.maplibre.nativejni.json.JsonValue layerJson, String beforeLayerId);

  public static native int mln_map_remove_style_layer(
      long map, String layerId, boolean[] outRemoved);

  public static native int mln_map_style_layer_exists(
      long map, String layerId, boolean[] outExists);

  public static native int mln_map_get_style_layer_type(
      long map, String layerId, String[] outLayerType, boolean[] outFound);

  public static native int mln_map_list_style_layer_ids(long map, Object[] outLayerIds);

  public static native int mln_map_move_style_layer(long map, String layerId, String beforeLayerId);

  public static native int mln_map_get_style_layer_json(
      long map, String layerId, Object[] outJson, boolean[] outFound);

  public static native int mln_map_set_style_light_json(
      long map, org.maplibre.nativejni.json.JsonValue lightJson);

  public static native int mln_map_set_style_light_property(
      long map, String propertyName, org.maplibre.nativejni.json.JsonValue value);

  public static native int mln_map_get_style_light_property(
      long map, String propertyName, Object[] outJson);

  public static native int mln_map_set_layer_property(
      long map, String layerId, String propertyName, org.maplibre.nativejni.json.JsonValue value);

  public static native int mln_map_get_layer_property(
      long map, String layerId, String propertyName, Object[] outJson);

  public static native int mln_map_set_layer_filter(
      long map, String layerId, org.maplibre.nativejni.json.JsonValue filter);

  public static native int mln_map_get_layer_filter(long map, String layerId, Object[] outJson);
}
