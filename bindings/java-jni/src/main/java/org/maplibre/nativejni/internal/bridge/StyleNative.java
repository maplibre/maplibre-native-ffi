package org.maplibre.nativejni.internal.bridge;

import java.util.ArrayList;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import org.bytedeco.javacpp.BoolPointer;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.bytedeco.javacpp.PointerPointer;
import org.bytedeco.javacpp.SizeTPointer;
import org.maplibre.nativejni.geo.CanonicalTileId;
import org.maplibre.nativejni.geo.Feature;
import org.maplibre.nativejni.geo.FeatureIdentifier;
import org.maplibre.nativejni.geo.GeoJson;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.JavaCppValues;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.json.JsonValue;
import org.maplibre.nativejni.style.CustomGeometrySourceCallback;

/** JavaCPP-era adapter declarations for the StyleNative C API coverage group. */
public final class StyleNative {
  private static final Map<Long, CustomGeometryCallbackState> CUSTOM_GEOMETRY_STATES =
      new ConcurrentHashMap<>();
  private static final AtomicLong NEXT_CALLBACK_STATE = new AtomicLong(1);

  private StyleNative() {}

  public static int mln_map_add_style_source_json(long map, String sourceId, JsonValue sourceJson) {
    try (var source = JavaCppValues.stringView(sourceId);
        var json = JavaCppValues.json(sourceJson)) {
      return MaplibreNativeC.mln_map_add_style_source_json(
          JavaCppSupport.map(map), source.view(), json.value());
    }
  }

  public static int mln_map_remove_style_source(long map, String sourceId, boolean[] outRemoved) {
    try (var source = JavaCppValues.stringView(sourceId)) {
      return MaplibreNativeC.mln_map_remove_style_source(
          JavaCppSupport.map(map), source.view(), outRemoved);
    }
  }

  public static int mln_map_style_source_exists(long map, String sourceId, boolean[] outExists) {
    try (var source = JavaCppValues.stringView(sourceId)) {
      return MaplibreNativeC.mln_map_style_source_exists(
          JavaCppSupport.map(map), source.view(), outExists);
    }
  }

  public static int mln_map_get_style_source_type(
      long map, String sourceId, int[] outSourceType, boolean[] outFound) {
    try (var source = JavaCppValues.stringView(sourceId)) {
      return MaplibreNativeC.mln_map_get_style_source_type(
          JavaCppSupport.map(map), source.view(), outSourceType, outFound);
    }
  }

  public static int mln_map_get_style_source_info(
      long map, String sourceId, int[] outInfo, boolean[] outFlags, long[] outSizes) {
    try (var source = JavaCppValues.stringView(sourceId)) {
      var info = new MaplibreNativeC.mln_style_source_info();
      info.size(info.sizeof());
      var status =
          MaplibreNativeC.mln_map_get_style_source_info(
              JavaCppSupport.map(map), source.view(), info, outFlags);
      if (status == MaplibreNativeC.MLN_STATUS_OK && outFlags[0]) {
        outInfo[0] = info.type();
        outFlags[1] = info.is_volatile();
        outFlags[2] = info.has_attribution();
        outSizes[0] = info.attribution_size();
      }
      return status;
    }
  }

  public static int mln_map_copy_style_source_attribution(
      long map,
      String sourceId,
      byte[] outAttribution,
      long[] outAttributionSize,
      boolean[] outFound) {
    try (var source = JavaCppValues.stringView(sourceId);
        var size = new SizeTPointer(1)) {
      BytePointer out = outAttribution == null ? null : new BytePointer(outAttribution.length);
      var status =
          MaplibreNativeC.mln_map_copy_style_source_attribution(
              JavaCppSupport.map(map),
              source.view(),
              out,
              outAttribution == null ? 0 : outAttribution.length,
              size,
              outFound);
      outAttributionSize[0] = size.get();
      if (status == MaplibreNativeC.MLN_STATUS_OK && out != null && outAttributionSize[0] > 0) {
        out.get(
            outAttribution,
            0,
            Math.toIntExact(Math.min(outAttribution.length, outAttributionSize[0])));
      }
      if (out != null) {
        out.close();
      }
      return status;
    }
  }

  public static int mln_map_list_style_source_ids(long map, Object[] outSourceIds) {
    var outList = new PointerPointer<MaplibreNativeC.mln_style_id_list>(1);
    var status = MaplibreNativeC.mln_map_list_style_source_ids(JavaCppSupport.map(map), outList);
    if (status != MaplibreNativeC.MLN_STATUS_OK) {
      return status;
    }
    var list =
        new MaplibreNativeC.mln_style_id_list(
            JavaCppSupport.pointer(
                JavaCppSupport.outAddress(outList, MaplibreNativeC.mln_style_id_list.class)));
    try {
      outSourceIds[0] = idList(list);
      return MaplibreNativeC.MLN_STATUS_OK;
    } finally {
      MaplibreNativeC.mln_style_id_list_destroy(list);
    }
  }

  public static int mln_map_add_geojson_source_url(long map, String sourceId, String url) {
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeUrl = JavaCppValues.stringView(url)) {
      return MaplibreNativeC.mln_map_add_geojson_source_url(
          JavaCppSupport.map(map), source.view(), nativeUrl.view());
    }
  }

  public static int mln_map_add_geojson_source_data(long map, String sourceId, GeoJson data) {
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeData = geoJson(data)) {
      return MaplibreNativeC.mln_map_add_geojson_source_data(
          JavaCppSupport.map(map), source.view(), nativeData.value());
    }
  }

  public static int mln_map_set_geojson_source_url(long map, String sourceId, String url) {
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeUrl = JavaCppValues.stringView(url)) {
      return MaplibreNativeC.mln_map_set_geojson_source_url(
          JavaCppSupport.map(map), source.view(), nativeUrl.view());
    }
  }

  public static int mln_map_set_geojson_source_data(long map, String sourceId, GeoJson data) {
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeData = geoJson(data)) {
      return MaplibreNativeC.mln_map_set_geojson_source_data(
          JavaCppSupport.map(map), source.view(), nativeData.value());
    }
  }

  public static int mln_map_add_vector_source_url(
      long map,
      String sourceId,
      String url,
      boolean[] optionFields,
      double[] optionValues,
      String attribution) {
    var validation = validateTileOptions(optionFields, optionValues);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) return validation;
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeUrl = JavaCppValues.stringView(url);
        var options = tileOptions(optionFields, optionValues, attribution)) {
      var status =
          MaplibreNativeC.mln_map_add_vector_source_url(
              JavaCppSupport.map(map), source.view(), nativeUrl.view(), options.options());
      return status;
    }
  }

  public static int mln_map_add_vector_source_tiles(
      long map,
      String sourceId,
      String[] tiles,
      boolean[] optionFields,
      double[] optionValues,
      String attribution) {
    var validation = validateTileOptions(optionFields, optionValues);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) return validation;
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeTiles = JavaCppValues.stringViews(tiles);
        var options = tileOptions(optionFields, optionValues, attribution)) {
      var status =
          MaplibreNativeC.mln_map_add_vector_source_tiles(
              JavaCppSupport.map(map),
              source.view(),
              nativeTiles.views(),
              nativeTiles.count(),
              options.options());
      return status;
    }
  }

  public static int mln_map_add_raster_source_url(
      long map,
      String sourceId,
      String url,
      boolean[] optionFields,
      double[] optionValues,
      String attribution) {
    var validation = validateTileOptions(optionFields, optionValues);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) return validation;
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeUrl = JavaCppValues.stringView(url);
        var options = tileOptions(optionFields, optionValues, attribution)) {
      var status =
          MaplibreNativeC.mln_map_add_raster_source_url(
              JavaCppSupport.map(map), source.view(), nativeUrl.view(), options.options());
      return status;
    }
  }

  public static int mln_map_add_raster_source_tiles(
      long map,
      String sourceId,
      String[] tiles,
      boolean[] optionFields,
      double[] optionValues,
      String attribution) {
    var validation = validateTileOptions(optionFields, optionValues);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) return validation;
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeTiles = JavaCppValues.stringViews(tiles);
        var options = tileOptions(optionFields, optionValues, attribution)) {
      var status =
          MaplibreNativeC.mln_map_add_raster_source_tiles(
              JavaCppSupport.map(map),
              source.view(),
              nativeTiles.views(),
              nativeTiles.count(),
              options.options());
      return status;
    }
  }

  public static int mln_map_add_raster_dem_source_url(
      long map,
      String sourceId,
      String url,
      boolean[] optionFields,
      double[] optionValues,
      String attribution) {
    var validation = validateTileOptions(optionFields, optionValues);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) return validation;
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeUrl = JavaCppValues.stringView(url);
        var options = tileOptions(optionFields, optionValues, attribution)) {
      var status =
          MaplibreNativeC.mln_map_add_raster_dem_source_url(
              JavaCppSupport.map(map), source.view(), nativeUrl.view(), options.options());
      return status;
    }
  }

  public static int mln_map_add_raster_dem_source_tiles(
      long map,
      String sourceId,
      String[] tiles,
      boolean[] optionFields,
      double[] optionValues,
      String attribution) {
    var validation = validateTileOptions(optionFields, optionValues);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) return validation;
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeTiles = JavaCppValues.stringViews(tiles);
        var options = tileOptions(optionFields, optionValues, attribution)) {
      var status =
          MaplibreNativeC.mln_map_add_raster_dem_source_tiles(
              JavaCppSupport.map(map),
              source.view(),
              nativeTiles.views(),
              nativeTiles.count(),
              options.options());
      return status;
    }
  }

  public static int mln_map_add_custom_geometry_source(
      long map,
      String sourceId,
      CustomGeometrySourceCallback callback,
      boolean[] optionFields,
      double[] optionValues,
      long[] outState) {
    if ((optionFields[3] && optionValues[3] < 0) || (optionFields[4] && optionValues[4] < 0)) {
      BaseNative.setThreadDiagnostic(
          "custom geometry source unsigned options must be non-negative");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    var callbackState = new CustomGeometryCallbackState(callback);
    try (var source = JavaCppValues.stringView(sourceId);
        var options = new CustomGeometryOptionsScope(optionFields, optionValues, callbackState)) {
      var status =
          MaplibreNativeC.mln_map_add_custom_geometry_source(
              JavaCppSupport.map(map), source.view(), options.options());
      if (status == MaplibreNativeC.MLN_STATUS_OK) {
        CUSTOM_GEOMETRY_STATES.put(callbackState.id(), callbackState);
        outState[0] = callbackState.id();
      } else {
        callbackState.close();
      }
      return status;
    }
  }

  public static int mln_map_set_custom_geometry_source_tile_data(
      long map, String sourceId, int tileZ, long tileX, long tileY, GeoJson data) {
    var validation = validateCanonicalTile(tileX, tileY);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) return validation;
    try (var source = JavaCppValues.stringView(sourceId);
        var tileId = tileId(tileZ, tileX, tileY);
        var nativeData = geoJson(data)) {
      return MaplibreNativeC.mln_map_set_custom_geometry_source_tile_data(
          JavaCppSupport.map(map), source.view(), tileId.tileId(), nativeData.value());
    }
  }

  public static int mln_map_invalidate_custom_geometry_source_tile(
      long map, String sourceId, int tileZ, long tileX, long tileY) {
    var validation = validateCanonicalTile(tileX, tileY);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) return validation;
    try (var source = JavaCppValues.stringView(sourceId);
        var tileId = tileId(tileZ, tileX, tileY)) {
      return MaplibreNativeC.mln_map_invalidate_custom_geometry_source_tile(
          JavaCppSupport.map(map), source.view(), tileId.tileId());
    }
  }

  public static int mln_map_invalidate_custom_geometry_source_region(
      long map,
      String sourceId,
      double southwestLatitude,
      double southwestLongitude,
      double northeastLatitude,
      double northeastLongitude) {
    try (var source = JavaCppValues.stringView(sourceId);
        var bounds =
            bounds(southwestLatitude, southwestLongitude, northeastLatitude, northeastLongitude)) {
      return MaplibreNativeC.mln_map_invalidate_custom_geometry_source_region(
          JavaCppSupport.map(map), source.view(), bounds.bounds());
    }
  }

  public static void mln_custom_geometry_source_state_destroy(long state) {
    var callbackState = CUSTOM_GEOMETRY_STATES.remove(state);
    if (callbackState != null) {
      callbackState.close();
    }
  }

  public static int mln_map_set_style_image(
      long map,
      String imageId,
      int width,
      int height,
      int stride,
      byte[] pixels,
      boolean hasPixelRatio,
      double pixelRatio,
      boolean hasSdf,
      boolean sdf) {
    try (var id = JavaCppValues.stringView(imageId);
        var pixelBytes = new BytePointer(pixels.length)) {
      pixelBytes.put(pixels);
      var image = MaplibreNativeC.mln_premultiplied_rgba8_image_default();
      image.width(width);
      image.height(height);
      image.stride(stride);
      image.pixels(pixelBytes);
      image.byte_length(pixels.length);
      var options = MaplibreNativeC.mln_style_image_options_default();
      int fields = 0;
      if (hasPixelRatio) {
        fields |= MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
        options.pixel_ratio((float) pixelRatio);
      }
      if (hasSdf) {
        fields |= MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_SDF;
        options.sdf(sdf);
      }
      options.fields(fields);
      return MaplibreNativeC.mln_map_set_style_image(
          JavaCppSupport.map(map), id.view(), image, options);
    }
  }

  public static int mln_map_remove_style_image(long map, String imageId, boolean[] outRemoved) {
    try (var id = JavaCppValues.stringView(imageId)) {
      return MaplibreNativeC.mln_map_remove_style_image(
          JavaCppSupport.map(map), id.view(), outRemoved);
    }
  }

  public static int mln_map_style_image_exists(long map, String imageId, boolean[] outExists) {
    try (var id = JavaCppValues.stringView(imageId)) {
      return MaplibreNativeC.mln_map_style_image_exists(
          JavaCppSupport.map(map), id.view(), outExists);
    }
  }

  public static int mln_map_get_style_image_info(
      long map,
      String imageId,
      int[] outInfo,
      long[] outByteLength,
      double[] outPixelRatio,
      boolean[] outFlags) {
    try (var id = JavaCppValues.stringView(imageId)) {
      var info = MaplibreNativeC.mln_style_image_info_default();
      var status =
          MaplibreNativeC.mln_map_get_style_image_info(
              JavaCppSupport.map(map), id.view(), info, outFlags);
      if (status == MaplibreNativeC.MLN_STATUS_OK && outFlags[0]) {
        outInfo[0] = info.width();
        outInfo[1] = info.height();
        outInfo[2] = info.stride();
        outByteLength[0] = info.byte_length();
        outPixelRatio[0] = info.pixel_ratio();
        outFlags[1] = info.sdf();
      }
      return status;
    }
  }

  public static int mln_map_copy_style_image_premultiplied_rgba8(
      long map, String imageId, byte[] outPixels, long[] outByteLength, boolean[] outFound) {
    try (var id = JavaCppValues.stringView(imageId);
        var size = new SizeTPointer(1)) {
      BytePointer out = outPixels == null ? null : new BytePointer(outPixels.length);
      var status =
          MaplibreNativeC.mln_map_copy_style_image_premultiplied_rgba8(
              JavaCppSupport.map(map),
              id.view(),
              out,
              outPixels == null ? 0 : outPixels.length,
              size,
              outFound);
      outByteLength[0] = size.get();
      if (status == MaplibreNativeC.MLN_STATUS_OK && outFound[0] && out != null) {
        out.get(outPixels, 0, Math.toIntExact(Math.min(outPixels.length, outByteLength[0])));
      }
      if (out != null) {
        out.close();
      }
      return status;
    }
  }

  public static int mln_map_add_image_source_url(
      long map, String sourceId, double[] coordinates, String url) {
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeUrl = JavaCppValues.stringView(url);
        var nativeCoordinates = coordinates(coordinates)) {
      var status =
          MaplibreNativeC.mln_map_add_image_source_url(
              JavaCppSupport.map(map),
              source.view(),
              nativeCoordinates.coordinates(),
              nativeCoordinates.count(),
              nativeUrl.view());
      return status;
    }
  }

  public static int mln_map_add_image_source_image(
      long map,
      String sourceId,
      double[] coordinates,
      int width,
      int height,
      int stride,
      byte[] pixels) {
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeCoordinates = coordinates(coordinates);
        var image = image(width, height, stride, pixels)) {
      var status =
          MaplibreNativeC.mln_map_add_image_source_image(
              JavaCppSupport.map(map),
              source.view(),
              nativeCoordinates.coordinates(),
              nativeCoordinates.count(),
              image.image());
      return status;
    }
  }

  public static int mln_map_set_image_source_url(long map, String sourceId, String url) {
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeUrl = JavaCppValues.stringView(url)) {
      return MaplibreNativeC.mln_map_set_image_source_url(
          JavaCppSupport.map(map), source.view(), nativeUrl.view());
    }
  }

  public static int mln_map_set_image_source_image(
      long map, String sourceId, int width, int height, int stride, byte[] pixels) {
    try (var source = JavaCppValues.stringView(sourceId);
        var image = image(width, height, stride, pixels)) {
      return MaplibreNativeC.mln_map_set_image_source_image(
          JavaCppSupport.map(map), source.view(), image.image());
    }
  }

  public static int mln_map_set_image_source_coordinates(
      long map, String sourceId, double[] coordinates) {
    try (var source = JavaCppValues.stringView(sourceId);
        var nativeCoordinates = coordinates(coordinates)) {
      return MaplibreNativeC.mln_map_set_image_source_coordinates(
          JavaCppSupport.map(map),
          source.view(),
          nativeCoordinates.coordinates(),
          nativeCoordinates.count());
    }
  }

  public static int mln_map_get_image_source_coordinates(
      long map,
      String sourceId,
      double[] outCoordinates,
      long[] outCoordinateCount,
      boolean[] outFound) {
    try (var source = JavaCppValues.stringView(sourceId);
        var size = new SizeTPointer(1);
        var nativeCoordinates = new LatLngArrayScope(outCoordinates.length / 2)) {
      var status =
          MaplibreNativeC.mln_map_get_image_source_coordinates(
              JavaCppSupport.map(map),
              source.view(),
              nativeCoordinates.coordinates(),
              nativeCoordinates.count(),
              size,
              outFound);
      outCoordinateCount[0] = size.get();
      if (status == MaplibreNativeC.MLN_STATUS_OK && outFound[0]) {
        nativeCoordinates.copyTo(outCoordinates, outCoordinateCount[0]);
      }
      return status;
    }
  }

  public static int mln_map_add_hillshade_layer(
      long map, String layerId, String sourceId, String beforeLayerId) {
    try (var layer = JavaCppValues.stringView(layerId);
        var source = JavaCppValues.stringView(sourceId);
        var before = JavaCppValues.stringView(beforeLayerId)) {
      return MaplibreNativeC.mln_map_add_hillshade_layer(
          JavaCppSupport.map(map), layer.view(), source.view(), before.view());
    }
  }

  public static int mln_map_add_color_relief_layer(
      long map, String layerId, String sourceId, String beforeLayerId) {
    try (var layer = JavaCppValues.stringView(layerId);
        var source = JavaCppValues.stringView(sourceId);
        var before = JavaCppValues.stringView(beforeLayerId)) {
      return MaplibreNativeC.mln_map_add_color_relief_layer(
          JavaCppSupport.map(map), layer.view(), source.view(), before.view());
    }
  }

  public static int mln_map_add_location_indicator_layer(
      long map, String layerId, String beforeLayerId) {
    try (var layer = JavaCppValues.stringView(layerId);
        var before = JavaCppValues.stringView(beforeLayerId)) {
      return MaplibreNativeC.mln_map_add_location_indicator_layer(
          JavaCppSupport.map(map), layer.view(), before.view());
    }
  }

  public static int mln_map_set_location_indicator_location(
      long map, String layerId, double latitude, double longitude, double altitude) {
    try (var layer = JavaCppValues.stringView(layerId);
        var coordinate = new LatLngScope(latitude, longitude)) {
      return MaplibreNativeC.mln_map_set_location_indicator_location(
          JavaCppSupport.map(map), layer.view(), coordinate.coordinate(), altitude);
    }
  }

  public static int mln_map_set_location_indicator_bearing(
      long map, String layerId, double bearing) {
    try (var layer = JavaCppValues.stringView(layerId)) {
      return MaplibreNativeC.mln_map_set_location_indicator_bearing(
          JavaCppSupport.map(map), layer.view(), bearing);
    }
  }

  public static int mln_map_set_location_indicator_accuracy_radius(
      long map, String layerId, double radius) {
    try (var layer = JavaCppValues.stringView(layerId)) {
      return MaplibreNativeC.mln_map_set_location_indicator_accuracy_radius(
          JavaCppSupport.map(map), layer.view(), radius);
    }
  }

  public static int mln_map_set_location_indicator_image_name(
      long map, String layerId, int imageKind, String imageId) {
    try (var layer = JavaCppValues.stringView(layerId);
        var image = JavaCppValues.stringView(imageId)) {
      return MaplibreNativeC.mln_map_set_location_indicator_image_name(
          JavaCppSupport.map(map), layer.view(), imageKind, image.view());
    }
  }

  public static int mln_map_add_style_layer_json(
      long map, JsonValue layerJson, String beforeLayerId) {
    try (var layer = JavaCppValues.json(layerJson);
        var before = JavaCppValues.stringView(beforeLayerId)) {
      return MaplibreNativeC.mln_map_add_style_layer_json(
          JavaCppSupport.map(map), layer.value(), before.view());
    }
  }

  public static int mln_map_remove_style_layer(long map, String layerId, boolean[] outRemoved) {
    try (var layer = JavaCppValues.stringView(layerId)) {
      return MaplibreNativeC.mln_map_remove_style_layer(
          JavaCppSupport.map(map), layer.view(), outRemoved);
    }
  }

  public static int mln_map_style_layer_exists(long map, String layerId, boolean[] outExists) {
    try (var layer = JavaCppValues.stringView(layerId)) {
      return MaplibreNativeC.mln_map_style_layer_exists(
          JavaCppSupport.map(map), layer.view(), outExists);
    }
  }

  public static int mln_map_get_style_layer_type(
      long map, String layerId, String[] outLayerType, boolean[] outFound) {
    try (var layer = JavaCppValues.stringView(layerId)) {
      var outType = new MaplibreNativeC.mln_string_view();
      var status =
          MaplibreNativeC.mln_map_get_style_layer_type(
              JavaCppSupport.map(map), layer.view(), outType, outFound);
      if (status == MaplibreNativeC.MLN_STATUS_OK && outFound[0]) {
        outLayerType[0] = JavaCppValues.string(outType);
      }
      outType.close();
      return status;
    }
  }

  public static int mln_map_list_style_layer_ids(long map, Object[] outLayerIds) {
    var outList = new PointerPointer<MaplibreNativeC.mln_style_id_list>(1);
    var status = MaplibreNativeC.mln_map_list_style_layer_ids(JavaCppSupport.map(map), outList);
    if (status != MaplibreNativeC.MLN_STATUS_OK) {
      return status;
    }
    var listAddress = JavaCppSupport.outAddress(outList, MaplibreNativeC.mln_style_id_list.class);
    var list = new MaplibreNativeC.mln_style_id_list(JavaCppSupport.pointer(listAddress));
    try {
      outLayerIds[0] = idList(list);
      return MaplibreNativeC.MLN_STATUS_OK;
    } finally {
      MaplibreNativeC.mln_style_id_list_destroy(list);
    }
  }

  public static int mln_map_move_style_layer(long map, String layerId, String beforeLayerId) {
    try (var layer = JavaCppValues.stringView(layerId);
        var before = JavaCppValues.stringView(beforeLayerId)) {
      return MaplibreNativeC.mln_map_move_style_layer(
          JavaCppSupport.map(map), layer.view(), before.view());
    }
  }

  public static int mln_map_get_style_layer_json(
      long map, String layerId, Object[] outJson, boolean[] outFound) {
    try (var layer = JavaCppValues.stringView(layerId);
        var found = new BoolPointer(1)) {
      var outSnapshot = new PointerPointer<MaplibreNativeC.mln_json_snapshot>(1);
      var status =
          MaplibreNativeC.mln_map_get_style_layer_json(
              JavaCppSupport.map(map), layer.view(), outSnapshot, found);
      outFound[0] = found.get();
      if (status == MaplibreNativeC.MLN_STATUS_OK && outFound[0]) {
        status = copySnapshot(outSnapshot, outJson);
      }
      return status;
    }
  }

  public static int mln_map_set_style_light_json(long map, JsonValue lightJson) {
    try (var light = JavaCppValues.json(lightJson)) {
      return MaplibreNativeC.mln_map_set_style_light_json(JavaCppSupport.map(map), light.value());
    }
  }

  public static int mln_map_set_style_light_property(
      long map, String propertyName, JsonValue value) {
    try (var property = JavaCppValues.stringView(propertyName);
        var nativeValue = JavaCppValues.json(value)) {
      return MaplibreNativeC.mln_map_set_style_light_property(
          JavaCppSupport.map(map), property.view(), nativeValue.value());
    }
  }

  public static int mln_map_get_style_light_property(
      long map, String propertyName, Object[] outJson) {
    try (var property = JavaCppValues.stringView(propertyName)) {
      var outSnapshot = new PointerPointer<MaplibreNativeC.mln_json_snapshot>(1);
      var status =
          MaplibreNativeC.mln_map_get_style_light_property(
              JavaCppSupport.map(map), property.view(), outSnapshot);
      return status == MaplibreNativeC.MLN_STATUS_OK ? copySnapshot(outSnapshot, outJson) : status;
    }
  }

  public static int mln_map_set_layer_property(
      long map, String layerId, String propertyName, JsonValue value) {
    try (var layer = JavaCppValues.stringView(layerId);
        var property = JavaCppValues.stringView(propertyName);
        var nativeValue = JavaCppValues.json(value)) {
      return MaplibreNativeC.mln_map_set_layer_property(
          JavaCppSupport.map(map), layer.view(), property.view(), nativeValue.value());
    }
  }

  public static int mln_map_get_layer_property(
      long map, String layerId, String propertyName, Object[] outJson) {
    try (var layer = JavaCppValues.stringView(layerId);
        var property = JavaCppValues.stringView(propertyName)) {
      var outSnapshot = new PointerPointer<MaplibreNativeC.mln_json_snapshot>(1);
      var status =
          MaplibreNativeC.mln_map_get_layer_property(
              JavaCppSupport.map(map), layer.view(), property.view(), outSnapshot);
      return status == MaplibreNativeC.MLN_STATUS_OK ? copySnapshot(outSnapshot, outJson) : status;
    }
  }

  public static int mln_map_set_layer_filter(long map, String layerId, JsonValue filter) {
    try (var layer = JavaCppValues.stringView(layerId)) {
      if (filter == null) {
        return MaplibreNativeC.mln_map_set_layer_filter(
            JavaCppSupport.map(map), layer.view(), null);
      }
      try (var nativeFilter = JavaCppValues.json(filter)) {
        return MaplibreNativeC.mln_map_set_layer_filter(
            JavaCppSupport.map(map), layer.view(), nativeFilter.value());
      }
    }
  }

  public static int mln_map_get_layer_filter(long map, String layerId, Object[] outJson) {
    try (var layer = JavaCppValues.stringView(layerId)) {
      var outSnapshot = new PointerPointer<MaplibreNativeC.mln_json_snapshot>(1);
      var status =
          MaplibreNativeC.mln_map_get_layer_filter(
              JavaCppSupport.map(map), layer.view(), outSnapshot);
      return status == MaplibreNativeC.MLN_STATUS_OK ? copySnapshot(outSnapshot, outJson) : status;
    }
  }

  private static String[] idList(MaplibreNativeC.mln_style_id_list list) {
    try (var count = new SizeTPointer(1)) {
      var status = MaplibreNativeC.mln_style_id_list_count(list, count);
      if (status != MaplibreNativeC.MLN_STATUS_OK) {
        return new String[0];
      }
      var ids = new String[Math.toIntExact(count.get())];
      for (var i = 0; i < ids.length; i++) {
        var view = new MaplibreNativeC.mln_string_view();
        status = MaplibreNativeC.mln_style_id_list_get(list, i, view);
        ids[i] = status == MaplibreNativeC.MLN_STATUS_OK ? JavaCppValues.string(view) : "";
        view.close();
      }
      return ids;
    }
  }

  private static int copySnapshot(
      PointerPointer<MaplibreNativeC.mln_json_snapshot> outSnapshot, Object[] outJson) {
    var snapshotAddress =
        JavaCppSupport.outAddress(outSnapshot, MaplibreNativeC.mln_json_snapshot.class);
    if (snapshotAddress == 0) {
      outJson[0] = null;
      return MaplibreNativeC.MLN_STATUS_OK;
    }
    var snapshot = new MaplibreNativeC.mln_json_snapshot(JavaCppSupport.pointer(snapshotAddress));
    try {
      var outValue = new PointerPointer<MaplibreNativeC.mln_json_value>(1);
      var status = MaplibreNativeC.mln_json_snapshot_get(snapshot, outValue);
      if (status == MaplibreNativeC.MLN_STATUS_OK) {
        var valueAddress =
            JavaCppSupport.outAddress(outValue, MaplibreNativeC.mln_json_value.class);
        outJson[0] =
            valueAddress == 0
                ? null
                : JavaCppValues.jsonValue(
                    new MaplibreNativeC.mln_json_value(JavaCppSupport.pointer(valueAddress)));
      }
      return status;
    } finally {
      MaplibreNativeC.mln_json_snapshot_destroy(snapshot);
    }
  }

  private static GeoJsonScope geoJson(GeoJson value) {
    return new GeoJsonScope(value);
  }

  private static int validateCanonicalTile(long x, long y) {
    if (x < 0 || y < 0 || x > 0xffff_ffffL || y > 0xffff_ffffL) {
      BaseNative.setThreadDiagnostic("canonical tile x and y must fit uint32");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  private static TileIdScope tileId(int z, long x, long y) {
    return new TileIdScope(z, x, y);
  }

  private static BoundsScope bounds(
      double southwestLatitude,
      double southwestLongitude,
      double northeastLatitude,
      double northeastLongitude) {
    return new BoundsScope(
        southwestLatitude, southwestLongitude, northeastLatitude, northeastLongitude);
  }

  private static LatLngArrayScope coordinates(double[] coordinates) {
    return new LatLngArrayScope(coordinates);
  }

  private static PremultipliedImageScope image(int width, int height, int stride, byte[] pixels) {
    return new PremultipliedImageScope(width, height, stride, pixels);
  }

  private static int validateTileOptions(boolean[] fields, double[] values) {
    if (fields != null && fields.length > 5 && fields[5] && values[7] < 0) {
      BaseNative.setThreadDiagnostic("tile size must be non-negative");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  private static TileOptionsScope tileOptions(
      boolean[] fields, double[] values, String attribution) {
    return new TileOptionsScope(fields, values, attribution);
  }

  private static final class CustomGeometryCallbackState implements AutoCloseable {
    private final long id = NEXT_CALLBACK_STATE.getAndIncrement();
    private final CustomGeometrySourceCallback callback;
    private final MaplibreNativeC.mln_custom_geometry_source_tile_callback fetchTile;
    private final MaplibreNativeC.mln_custom_geometry_source_tile_callback cancelTile;

    CustomGeometryCallbackState(CustomGeometrySourceCallback callback) {
      this.callback = callback;
      this.fetchTile =
          new MaplibreNativeC.mln_custom_geometry_source_tile_callback() {
            @Override
            public void call(Pointer userData, MaplibreNativeC.mln_canonical_tile_id tileId) {
              CustomGeometryCallbackState.invokeFetch(userData, tileId);
            }
          };
      this.cancelTile =
          new MaplibreNativeC.mln_custom_geometry_source_tile_callback() {
            @Override
            public void call(Pointer userData, MaplibreNativeC.mln_canonical_tile_id tileId) {
              CustomGeometryCallbackState.invokeCancel(userData, tileId);
            }
          };
    }

    long id() {
      return id;
    }

    Pointer userData() {
      return JavaCppSupport.pointer(id);
    }

    MaplibreNativeC.mln_custom_geometry_source_tile_callback fetchTile() {
      return fetchTile;
    }

    MaplibreNativeC.mln_custom_geometry_source_tile_callback cancelTile() {
      return cancelTile;
    }

    @Override
    public void close() {
      fetchTile.close();
      cancelTile.close();
    }

    private static void invokeFetch(
        Pointer userData, MaplibreNativeC.mln_canonical_tile_id tileId) {
      var state = CUSTOM_GEOMETRY_STATES.get(userData.address());
      if (state == null) return;
      try {
        state.callback.fetchTile(canonicalTileId(tileId));
      } catch (Throwable ignored) {
        // Native callbacks must not unwind through the C ABI.
      }
    }

    private static void invokeCancel(
        Pointer userData, MaplibreNativeC.mln_canonical_tile_id tileId) {
      var state = CUSTOM_GEOMETRY_STATES.get(userData.address());
      if (state == null) return;
      try {
        state.callback.cancelTile(canonicalTileId(tileId));
      } catch (Throwable ignored) {
        // Native callbacks must not unwind through the C ABI.
      }
    }

    private static CanonicalTileId canonicalTileId(MaplibreNativeC.mln_canonical_tile_id tileId) {
      return new CanonicalTileId(
          tileId.z(), Integer.toUnsignedLong(tileId.x()), Integer.toUnsignedLong(tileId.y()));
    }
  }

  private static final class CustomGeometryOptionsScope implements AutoCloseable {
    private final MaplibreNativeC.mln_custom_geometry_source_options options;

    CustomGeometryOptionsScope(
        boolean[] fields, double[] values, CustomGeometryCallbackState callbackState) {
      options = MaplibreNativeC.mln_custom_geometry_source_options_default();
      int nativeFields = 0;
      options.fetch_tile(callbackState.fetchTile());
      options.cancel_tile(callbackState.cancelTile());
      options.user_data(callbackState.userData());
      if (fields[0]) {
        nativeFields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
        options.min_zoom(values[0]);
      }
      if (fields[1]) {
        nativeFields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
        options.max_zoom(values[1]);
      }
      if (fields[2]) {
        nativeFields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
        options.tolerance(values[2]);
      }
      if (fields[3]) {
        nativeFields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
        options.tile_size((int) values[3]);
      }
      if (fields[4]) {
        nativeFields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
        options.buffer((int) values[4]);
      }
      if (fields[5]) {
        nativeFields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
        options.clip(values[5] != 0);
      }
      if (fields[6]) {
        nativeFields |= MaplibreNativeC.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
        options.wrap(values[6] != 0);
      }
      options.fields(nativeFields);
    }

    MaplibreNativeC.mln_custom_geometry_source_options options() {
      return options;
    }

    @Override
    public void close() {
      options.close();
    }
  }

  private static final class TileIdScope implements AutoCloseable {
    private final MaplibreNativeC.mln_canonical_tile_id tileId;

    TileIdScope(int z, long x, long y) {
      tileId = new MaplibreNativeC.mln_canonical_tile_id();
      tileId.z(z);
      tileId.x((int) x);
      tileId.y((int) y);
    }

    MaplibreNativeC.mln_canonical_tile_id tileId() {
      return tileId;
    }

    @Override
    public void close() {
      tileId.close();
    }
  }

  private static final class BoundsScope implements AutoCloseable {
    private final MaplibreNativeC.mln_lat_lng_bounds bounds;

    BoundsScope(
        double southwestLatitude,
        double southwestLongitude,
        double northeastLatitude,
        double northeastLongitude) {
      bounds = new MaplibreNativeC.mln_lat_lng_bounds();
      bounds.southwest().latitude(southwestLatitude);
      bounds.southwest().longitude(southwestLongitude);
      bounds.northeast().latitude(northeastLatitude);
      bounds.northeast().longitude(northeastLongitude);
    }

    MaplibreNativeC.mln_lat_lng_bounds bounds() {
      return bounds;
    }

    @Override
    public void close() {
      bounds.close();
    }
  }

  private static final class GeoJsonScope implements AutoCloseable {
    private final ArrayList<Pointer> owned = new ArrayList<>();
    private final ArrayList<JavaCppValues.StringViewScope> strings = new ArrayList<>();
    private final ArrayList<JavaCppValues.JsonScope> jsonValues = new ArrayList<>();
    private final MaplibreNativeC.mln_geojson value;

    GeoJsonScope(GeoJson value) {
      this.value = geoJson(value);
    }

    MaplibreNativeC.mln_geojson value() {
      return value;
    }

    @Override
    public void close() {
      for (var i = jsonValues.size() - 1; i >= 0; i--) {
        jsonValues.get(i).close();
      }
      for (var i = owned.size() - 1; i >= 0; i--) {
        owned.get(i).close();
      }
      for (var i = strings.size() - 1; i >= 0; i--) {
        strings.get(i).close();
      }
    }

    private MaplibreNativeC.mln_geojson geoJson(GeoJson value) {
      var out = own(new MaplibreNativeC.mln_geojson());
      out.size(out.sizeof());
      switch (value) {
        case GeoJson.GeometryValue node -> {
          out.type(MaplibreNativeC.MLN_GEOJSON_TYPE_GEOMETRY);
          out.data_geometry(geometry(node.geometry()));
        }
        case GeoJson.FeatureValue node -> {
          out.type(MaplibreNativeC.MLN_GEOJSON_TYPE_FEATURE);
          out.data_feature(feature(node.feature()));
        }
        case GeoJson.FeatureCollection node -> {
          out.type(MaplibreNativeC.MLN_GEOJSON_TYPE_FEATURE_COLLECTION);
          var features = node.features();
          var collection = own(new MaplibreNativeC.mln_feature_collection());
          if (!features.isEmpty()) {
            var nativeFeatures = own(new MaplibreNativeC.mln_feature(features.size()));
            for (var i = 0; i < features.size(); i++) {
              nativeFeatures.position(i).put(feature(features.get(i)));
            }
            nativeFeatures.position(0);
            collection.features(nativeFeatures);
          }
          collection.feature_count(features.size());
          out.data_feature_collection(collection);
        }
      }
      return out;
    }

    private MaplibreNativeC.mln_geometry geometry(Geometry value) {
      var out = own(new MaplibreNativeC.mln_geometry());
      out.size(out.sizeof());
      switch (value) {
        case Geometry.Empty ignored -> out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_EMPTY);
        case Geometry.Point node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_POINT);
          out.data_point(coordinate(node.coordinate()));
        }
        case Geometry.LineString node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_LINE_STRING);
          out.data_line_string(coordinateSpan(node.coordinates()));
        }
        case Geometry.Polygon node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_POLYGON);
          out.data_polygon(polygon(node.rings()));
        }
        case Geometry.MultiPoint node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POINT);
          out.data_multi_point(coordinateSpan(node.coordinates()));
        }
        case Geometry.MultiLineString node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING);
          out.data_multi_line_string(multiLine(node.lines()));
        }
        case Geometry.MultiPolygon node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POLYGON);
          out.data_multi_polygon(multiPolygon(node.polygons()));
        }
        case Geometry.Collection node -> {
          out.type(MaplibreNativeC.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION);
          var geometries = node.geometries();
          var collection = own(new MaplibreNativeC.mln_geometry_collection());
          if (!geometries.isEmpty()) {
            var nativeGeometries = own(new MaplibreNativeC.mln_geometry(geometries.size()));
            for (var i = 0; i < geometries.size(); i++) {
              nativeGeometries.position(i).put(geometry(geometries.get(i)));
            }
            nativeGeometries.position(0);
            collection.geometries(nativeGeometries);
          }
          collection.geometry_count(geometries.size());
          out.data_geometry_collection(collection);
        }
      }
      return out;
    }

    private MaplibreNativeC.mln_feature feature(Feature value) {
      var out = own(new MaplibreNativeC.mln_feature());
      out.size(out.sizeof());
      out.geometry(geometry(value.geometry()));
      var properties = value.properties();
      if (!properties.isEmpty()) {
        var nativeProperties = own(new MaplibreNativeC.mln_json_member(properties.size()));
        for (var i = 0; i < properties.size(); i++) {
          var property = properties.get(i);
          nativeProperties.position(i);
          nativeProperties.key(string(property.key()));
          nativeProperties.value(json(property.value()));
        }
        nativeProperties.position(0);
        out.properties(nativeProperties);
      }
      out.property_count(properties.size());
      identifier(out, value.identifier());
      return out;
    }

    private void identifier(MaplibreNativeC.mln_feature out, FeatureIdentifier identifier) {
      switch (identifier) {
        case FeatureIdentifier.Null ignored ->
            out.identifier_type(MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_NULL);
        case FeatureIdentifier.UInt node -> {
          out.identifier_type(MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_UINT);
          out.identifier_uint_value(node.value());
        }
        case FeatureIdentifier.Int node -> {
          out.identifier_type(MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_INT);
          out.identifier_int_value(node.value());
        }
        case FeatureIdentifier.DoubleValue node -> {
          out.identifier_type(MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE);
          out.identifier_double_value(node.value());
        }
        case FeatureIdentifier.StringValue node -> {
          out.identifier_type(MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_STRING);
          out.identifier_string_value(string(node.value()));
        }
      }
    }

    private MaplibreNativeC.mln_coordinate_span coordinateSpan(java.util.List<LatLng> values) {
      var span = own(new MaplibreNativeC.mln_coordinate_span());
      if (!values.isEmpty()) {
        var nativeCoordinates = own(new MaplibreNativeC.mln_lat_lng(values.size()));
        for (var i = 0; i < values.size(); i++) {
          var coordinate = values.get(i);
          nativeCoordinates
              .position(i)
              .latitude(coordinate.latitude())
              .longitude(coordinate.longitude());
        }
        nativeCoordinates.position(0);
        span.coordinates(nativeCoordinates);
      }
      span.coordinate_count(values.size());
      return span;
    }

    private MaplibreNativeC.mln_polygon_geometry polygon(
        java.util.List<java.util.List<LatLng>> rings) {
      var out = own(new MaplibreNativeC.mln_polygon_geometry());
      if (!rings.isEmpty()) {
        var nativeRings = own(new MaplibreNativeC.mln_coordinate_span(rings.size()));
        for (var i = 0; i < rings.size(); i++) {
          nativeRings.position(i).put(coordinateSpan(rings.get(i)));
        }
        nativeRings.position(0);
        out.rings(nativeRings);
      }
      out.ring_count(rings.size());
      return out;
    }

    private MaplibreNativeC.mln_multi_line_geometry multiLine(
        java.util.List<java.util.List<LatLng>> lines) {
      var out = own(new MaplibreNativeC.mln_multi_line_geometry());
      if (!lines.isEmpty()) {
        var nativeLines = own(new MaplibreNativeC.mln_coordinate_span(lines.size()));
        for (var i = 0; i < lines.size(); i++) {
          nativeLines.position(i).put(coordinateSpan(lines.get(i)));
        }
        nativeLines.position(0);
        out.lines(nativeLines);
      }
      out.line_count(lines.size());
      return out;
    }

    private MaplibreNativeC.mln_multi_polygon_geometry multiPolygon(
        java.util.List<java.util.List<java.util.List<LatLng>>> polygons) {
      var out = own(new MaplibreNativeC.mln_multi_polygon_geometry());
      if (!polygons.isEmpty()) {
        var nativePolygons = own(new MaplibreNativeC.mln_polygon_geometry(polygons.size()));
        for (var i = 0; i < polygons.size(); i++) {
          nativePolygons.position(i).put(polygon(polygons.get(i)));
        }
        nativePolygons.position(0);
        out.polygons(nativePolygons);
      }
      out.polygon_count(polygons.size());
      return out;
    }

    private MaplibreNativeC.mln_lat_lng coordinate(LatLng value) {
      var out = own(new MaplibreNativeC.mln_lat_lng());
      out.latitude(value.latitude());
      out.longitude(value.longitude());
      return out;
    }

    private MaplibreNativeC.mln_json_value json(JsonValue value) {
      var json = JavaCppValues.json(value);
      jsonValues.add(json);
      return json.value();
    }

    private MaplibreNativeC.mln_string_view string(String value) {
      var string = JavaCppValues.stringView(value);
      strings.add(string);
      return string.view();
    }

    private <T extends Pointer> T own(T pointer) {
      owned.add(pointer);
      return pointer;
    }
  }

  private static final class LatLngScope implements AutoCloseable {
    private final MaplibreNativeC.mln_lat_lng coordinate;

    LatLngScope(double latitude, double longitude) {
      this.coordinate = new MaplibreNativeC.mln_lat_lng();
      coordinate.latitude(latitude);
      coordinate.longitude(longitude);
    }

    MaplibreNativeC.mln_lat_lng coordinate() {
      return coordinate;
    }

    @Override
    public void close() {
      coordinate.close();
    }
  }

  private static final class LatLngArrayScope implements AutoCloseable {
    private final MaplibreNativeC.mln_lat_lng coordinates;
    private final long count;

    LatLngArrayScope(double[] values) {
      this(values.length / 2);
      for (var i = 0; i < count; i++) {
        coordinates.position(i).latitude(values[i * 2]).longitude(values[i * 2 + 1]);
      }
      coordinates.position(0);
    }

    LatLngArrayScope(long count) {
      this.count = count;
      this.coordinates = new MaplibreNativeC.mln_lat_lng(count);
    }

    MaplibreNativeC.mln_lat_lng coordinates() {
      return coordinates;
    }

    long count() {
      return count;
    }

    void copyTo(double[] out, long coordinateCount) {
      for (var i = 0; i < coordinateCount && i * 2 + 1 < out.length; i++) {
        var coordinate = coordinates.getPointer(i);
        out[(int) i * 2] = coordinate.latitude();
        out[(int) i * 2 + 1] = coordinate.longitude();
      }
    }

    @Override
    public void close() {
      coordinates.close();
    }
  }

  private static final class PremultipliedImageScope implements AutoCloseable {
    private final BytePointer pixels;
    private final MaplibreNativeC.mln_premultiplied_rgba8_image image;

    PremultipliedImageScope(int width, int height, int stride, byte[] pixels) {
      this.pixels = new BytePointer(pixels.length);
      this.pixels.put(pixels);
      this.image = MaplibreNativeC.mln_premultiplied_rgba8_image_default();
      image.width(width);
      image.height(height);
      image.stride(stride);
      image.pixels(this.pixels);
      image.byte_length(pixels.length);
    }

    MaplibreNativeC.mln_premultiplied_rgba8_image image() {
      return image;
    }

    @Override
    public void close() {
      image.close();
      pixels.close();
    }
  }

  private static final class TileOptionsScope implements AutoCloseable {
    private final JavaCppValues.StringViewScope attribution;
    private final MaplibreNativeC.mln_style_tile_source_options options;

    TileOptionsScope(boolean[] fields, double[] values, String attribution) {
      this.attribution = JavaCppValues.stringView(attribution == null ? "" : attribution);
      this.options = MaplibreNativeC.mln_style_tile_source_options_default();
      int nativeFields = 0;
      if (fields != null) {
        if (fields[0]) {
          nativeFields |= MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
          options.min_zoom(values[0]);
        }
        if (fields[1]) {
          nativeFields |= MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
          options.max_zoom(values[1]);
        }
        if (fields[2]) {
          nativeFields |= MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
          options.attribution(this.attribution.view());
        }
        if (fields[3]) {
          nativeFields |= MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
          options.scheme((int) values[6]);
        }
        if (fields[4]) {
          nativeFields |= MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
          options.bounds().southwest().latitude(values[2]);
          options.bounds().southwest().longitude(values[3]);
          options.bounds().northeast().latitude(values[4]);
          options.bounds().northeast().longitude(values[5]);
        }
        if (fields[5]) {
          nativeFields |= MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
          options.tile_size((int) values[7]);
        }
        if (fields[6]) {
          nativeFields |= MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
          options.vector_encoding((int) values[8]);
        }
        if (fields[7]) {
          nativeFields |= MaplibreNativeC.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
          options.raster_encoding((int) values[9]);
        }
      }
      options.fields(nativeFields);
    }

    MaplibreNativeC.mln_style_tile_source_options options() {
      return options;
    }

    @Override
    public void close() {
      options.close();
      attribution.close();
    }
  }
}
