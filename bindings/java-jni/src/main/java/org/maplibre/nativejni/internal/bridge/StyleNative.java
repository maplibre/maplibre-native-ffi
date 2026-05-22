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

  public static native int mln_map_get_style_source_info();

  public static native int mln_map_copy_style_source_attribution();

  public static native int mln_map_list_style_source_ids();

  public static native int mln_map_add_geojson_source_url(long map, String sourceId, String url);

  public static native int mln_map_add_geojson_source_data();

  public static native int mln_map_set_geojson_source_url(long map, String sourceId, String url);

  public static native int mln_map_set_geojson_source_data();

  public static native int mln_map_add_vector_source_url(long map, String sourceId, String url);

  public static native int mln_map_add_vector_source_tiles();

  public static native int mln_map_add_raster_source_url(long map, String sourceId, String url);

  public static native int mln_map_add_raster_source_tiles();

  public static native int mln_map_add_raster_dem_source_url(long map, String sourceId, String url);

  public static native int mln_map_add_raster_dem_source_tiles();

  public static native int mln_map_add_custom_geometry_source();

  public static native int mln_map_set_custom_geometry_source_tile_data();

  public static native int mln_map_invalidate_custom_geometry_source_tile();

  public static native int mln_map_invalidate_custom_geometry_source_region();

  public static native int mln_map_set_style_image();

  public static native int mln_map_remove_style_image();

  public static native int mln_map_style_image_exists();

  public static native int mln_map_get_style_image_info();

  public static native int mln_map_copy_style_image_premultiplied_rgba8();

  public static native int mln_map_add_image_source_url();

  public static native int mln_map_add_image_source_image();

  public static native int mln_map_set_image_source_url();

  public static native int mln_map_set_image_source_image();

  public static native int mln_map_set_image_source_coordinates();

  public static native int mln_map_get_image_source_coordinates();

  public static native int mln_map_add_hillshade_layer();

  public static native int mln_map_add_color_relief_layer();

  public static native int mln_map_add_location_indicator_layer();

  public static native int mln_map_set_location_indicator_location();

  public static native int mln_map_set_location_indicator_bearing();

  public static native int mln_map_set_location_indicator_accuracy_radius();

  public static native int mln_map_set_location_indicator_image_name();

  public static native int mln_map_add_style_layer_json();

  public static native int mln_map_remove_style_layer(
      long map, String layerId, boolean[] outRemoved);

  public static native int mln_map_style_layer_exists(
      long map, String layerId, boolean[] outExists);

  public static native int mln_map_get_style_layer_type(
      long map, String layerId, String[] outLayerType, boolean[] outFound);

  public static native int mln_map_list_style_layer_ids();

  public static native int mln_map_move_style_layer();

  public static native int mln_map_get_style_layer_json();

  public static native int mln_map_set_style_light_json();

  public static native int mln_map_set_style_light_property();

  public static native int mln_map_get_style_light_property();

  public static native int mln_map_set_layer_property();

  public static native int mln_map_get_layer_property();

  public static native int mln_map_set_layer_filter();

  public static native int mln_map_get_layer_filter();
}
