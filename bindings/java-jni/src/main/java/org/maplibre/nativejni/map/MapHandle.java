package org.maplibre.nativejni.map;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import org.bytedeco.javacpp.BoolPointer;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.SizeTPointer;
import org.maplibre.nativejni.camera.AnimationOptions;
import org.maplibre.nativejni.camera.BoundOptions;
import org.maplibre.nativejni.camera.CameraFitOptions;
import org.maplibre.nativejni.camera.CameraOptions;
import org.maplibre.nativejni.camera.EdgeInsets;
import org.maplibre.nativejni.camera.FreeCameraOptions;
import org.maplibre.nativejni.geo.CanonicalTileId;
import org.maplibre.nativejni.geo.GeoJson;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.LatLngBounds;
import org.maplibre.nativejni.geo.Quaternion;
import org.maplibre.nativejni.geo.ScreenPoint;
import org.maplibre.nativejni.geo.Vec3;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.JavaCppValues;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.lifecycle.HandleState;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.internal.struct.MapStructs;
import org.maplibre.nativejni.internal.struct.StyleStructs;
import org.maplibre.nativejni.json.JsonValue;
import org.maplibre.nativejni.render.MetalBorrowedTextureDescriptor;
import org.maplibre.nativejni.render.MetalOwnedTextureDescriptor;
import org.maplibre.nativejni.render.MetalSurfaceDescriptor;
import org.maplibre.nativejni.render.OpenGLBorrowedTextureDescriptor;
import org.maplibre.nativejni.render.OpenGLOwnedTextureDescriptor;
import org.maplibre.nativejni.render.OpenGLSurfaceDescriptor;
import org.maplibre.nativejni.render.PremultipliedRgba8Image;
import org.maplibre.nativejni.render.RenderSessionHandle;
import org.maplibre.nativejni.render.VulkanBorrowedTextureDescriptor;
import org.maplibre.nativejni.render.VulkanOwnedTextureDescriptor;
import org.maplibre.nativejni.render.VulkanSurfaceDescriptor;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.style.CustomGeometrySourceOptions;
import org.maplibre.nativejni.style.LocationIndicatorImageKind;
import org.maplibre.nativejni.style.SourceInfo;
import org.maplibre.nativejni.style.SourceType;
import org.maplibre.nativejni.style.StyleImage;
import org.maplibre.nativejni.style.StyleImageInfo;
import org.maplibre.nativejni.style.StyleImageOptions;
import org.maplibre.nativejni.style.TileSourceOptions;

/** Owned native map handle. Close it on the map owner thread. */
public final class MapHandle implements AutoCloseable {
  static final int CAMERA_FIELD_COUNT = 9;
  static final int CAMERA_VALUE_COUNT = 14;
  private static final int BOUND_FIELD_COUNT = 5;
  private static final int BOUND_VALUE_COUNT = 8;
  private static final int FIT_FIELD_COUNT = 3;
  private static final int FIT_VALUE_COUNT = 6;
  private static final int FREE_CAMERA_FIELD_COUNT = 2;
  private static final int FREE_CAMERA_VALUE_COUNT = 7;
  private static final int PROJECTION_MODE_FIELD_COUNT = 3;
  private static final int PROJECTION_MODE_BOOLEAN_COUNT = 1;
  private static final int PROJECTION_MODE_VALUE_COUNT = 2;
  private static final int ANIMATION_FIELD_COUNT = 4;
  private static final int ANIMATION_VALUE_COUNT = 7;

  private final RuntimeHandle runtime;
  private final HandleState state;
  private final Map<String, CustomGeometrySourceState> customGeometrySources = new HashMap<>();

  private MapHandle(RuntimeHandle runtime, long handle) {
    this.runtime = Objects.requireNonNull(runtime, "runtime");
    this.state = new HandleState("MapHandle", handle, runtime);
    runtime.registerMap(InternalAccess.INSTANCE, this);
  }

  public static MapHandle create(RuntimeHandle runtime, MapOptions options) {
    Objects.requireNonNull(runtime, "runtime");
    Objects.requireNonNull(options, "options");
    NativeLibrary.ensureLoaded();
    if ((options.width() != null && options.width() < 0)
        || (options.height() != null && options.height() < 0)) {
      JavaCppSupport.setThreadDiagnostic("width and height must be non-negative");
      Status.check(MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT);
    }
    var nativeOptions = MaplibreNativeC.mln_map_options_default();
    if (options.width() != null) {
      nativeOptions.width(options.width());
    }
    if (options.height() != null) {
      nativeOptions.height(options.height());
    }
    if (options.scaleFactor() != null) {
      nativeOptions.scale_factor(options.scaleFactor());
    }
    if (options.mapMode() != null) {
      nativeOptions.map_mode(options.mapMode().nativeValue());
    }
    var outMap = JavaCppSupport.outPointer(MaplibreNativeC.mln_map.class);
    Status.check(
        MaplibreNativeC.mln_map_create(
            JavaCppSupport.runtime(runtime.nativeAddress(InternalAccess.INSTANCE)),
            nativeOptions,
            outMap));
    return new MapHandle(runtime, JavaCppSupport.outAddress(outMap, MaplibreNativeC.mln_map.class));
  }

  public void setStyleUrl(String url) {
    NativeLibrary.ensureLoaded();
    var nativeUrl = JavaCppSupport.utf8(Objects.requireNonNull(url, "url"));
    try {
      Status.check(
          MaplibreNativeC.mln_map_set_style_url(
              JavaCppSupport.map(state.requireLiveAddress()), nativeUrl));
    } finally {
      nativeUrl.close();
    }
  }

  public void setStyleJson(String json) {
    NativeLibrary.ensureLoaded();
    var nativeJson = JavaCppSupport.utf8(Objects.requireNonNull(json, "json"));
    try {
      Status.check(
          MaplibreNativeC.mln_map_set_style_json(
              JavaCppSupport.map(state.requireLiveAddress()), nativeJson));
      clearCustomGeometrySources();
    } finally {
      nativeJson.close();
    }
  }

  public void addStyleSourceJson(String sourceId, JsonValue sourceJson) {
    NativeLibrary.ensureLoaded();
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var json = JavaCppValues.json(Objects.requireNonNull(sourceJson, "sourceJson"))) {
      Status.check(
          MaplibreNativeC.mln_map_add_style_source_json(
              JavaCppSupport.map(state.requireLiveAddress()), source.view(), json.value()));
    }
  }

  public boolean removeStyleSource(String sourceId) {
    NativeLibrary.ensureLoaded();
    var outRemoved = new boolean[1];
    var sourceIdValue = Objects.requireNonNull(sourceId, "sourceId");
    try (var source = JavaCppValues.stringView(sourceIdValue)) {
      Status.check(
          MaplibreNativeC.mln_map_remove_style_source(
              JavaCppSupport.map(state.requireLiveAddress()), source.view(), outRemoved));
    }
    if (outRemoved[0]) {
      closeCustomGeometrySource(sourceIdValue);
    }
    return outRemoved[0];
  }

  public boolean styleSourceExists(String sourceId) {
    NativeLibrary.ensureLoaded();
    var outExists = new boolean[1];
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"))) {
      Status.check(
          MaplibreNativeC.mln_map_style_source_exists(
              JavaCppSupport.map(state.requireLiveAddress()), source.view(), outExists));
    }
    return outExists[0];
  }

  public Optional<SourceType> styleSourceType(String sourceId) {
    NativeLibrary.ensureLoaded();
    var outSourceType = new int[1];
    var outFound = new boolean[1];
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"))) {
      Status.check(
          MaplibreNativeC.mln_map_get_style_source_type(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              outSourceType,
              outFound));
    }
    return outFound[0] ? Optional.of(SourceType.fromNative(outSourceType[0])) : Optional.empty();
  }

  public Optional<SourceInfo> styleSourceInfo(String sourceId) {
    NativeLibrary.ensureLoaded();
    var sourceIdValue = Objects.requireNonNull(sourceId, "sourceId");
    try (var source = JavaCppValues.stringView(sourceIdValue);
        var outInfo = new MaplibreNativeC.mln_style_source_info()) {
      outInfo.size(outInfo.sizeof());
      var outFound = new boolean[1];
      Status.check(
          MaplibreNativeC.mln_map_get_style_source_info(
              JavaCppSupport.map(state.requireLiveAddress()), source.view(), outInfo, outFound));
      if (!outFound[0]) {
        return Optional.empty();
      }
      var attribution = Optional.<String>empty();
      if (outInfo.has_attribution()) {
        var attributionSize = Math.toIntExact(outInfo.attribution_size());
        if (attributionSize == 0) {
          attribution = Optional.of("");
        } else {
          var outAttribution = new byte[attributionSize];
          var outAttributionFound = new boolean[1];
          try (var out = new BytePointer(attributionSize);
              var outSize = new SizeTPointer(1)) {
            Status.check(
                MaplibreNativeC.mln_map_copy_style_source_attribution(
                    JavaCppSupport.map(state.requireLiveAddress()),
                    source.view(),
                    out,
                    attributionSize,
                    outSize,
                    outAttributionFound));
            if (!outAttributionFound[0]) {
              return Optional.empty();
            }
            out.get(outAttribution, 0, Math.toIntExact(outSize.get()));
            attribution =
                Optional.of(
                    new String(
                        outAttribution, 0, Math.toIntExact(outSize.get()), StandardCharsets.UTF_8));
          }
        }
      }
      return Optional.of(
          new SourceInfo(
              SourceType.fromNative(outInfo.type()),
              outInfo.type(),
              outInfo.is_volatile(),
              attribution));
    }
  }

  public List<String> styleSourceIds() {
    NativeLibrary.ensureLoaded();
    var outList = JavaCppSupport.outPointer(MaplibreNativeC.mln_style_id_list.class);
    Status.check(
        MaplibreNativeC.mln_map_list_style_source_ids(
            JavaCppSupport.map(state.requireLiveAddress()), outList));
    var list =
        new MaplibreNativeC.mln_style_id_list(
            JavaCppSupport.pointer(
                JavaCppSupport.outAddress(outList, MaplibreNativeC.mln_style_id_list.class)));
    return List.of(StyleStructs.styleIdList(list));
  }

  public void addGeoJsonSourceUrl(String sourceId, String url) {
    NativeLibrary.ensureLoaded();
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeUrl = JavaCppValues.stringView(Objects.requireNonNull(url, "url"))) {
      Status.check(
          MaplibreNativeC.mln_map_add_geojson_source_url(
              JavaCppSupport.map(state.requireLiveAddress()), source.view(), nativeUrl.view()));
    }
  }

  public void addGeoJsonSourceData(String sourceId, GeoJson data) {
    NativeLibrary.ensureLoaded();
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeData = StyleStructs.geoJson(Objects.requireNonNull(data, "data"))) {
      Status.check(
          MaplibreNativeC.mln_map_add_geojson_source_data(
              JavaCppSupport.map(state.requireLiveAddress()), source.view(), nativeData.value()));
    }
  }

  public void setGeoJsonSourceUrl(String sourceId, String url) {
    NativeLibrary.ensureLoaded();
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeUrl = JavaCppValues.stringView(Objects.requireNonNull(url, "url"))) {
      Status.check(
          MaplibreNativeC.mln_map_set_geojson_source_url(
              JavaCppSupport.map(state.requireLiveAddress()), source.view(), nativeUrl.view()));
    }
  }

  public void setGeoJsonSourceData(String sourceId, GeoJson data) {
    NativeLibrary.ensureLoaded();
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeData = StyleStructs.geoJson(Objects.requireNonNull(data, "data"))) {
      Status.check(
          MaplibreNativeC.mln_map_set_geojson_source_data(
              JavaCppSupport.map(state.requireLiveAddress()), source.view(), nativeData.value()));
    }
  }

  public void addCustomGeometrySource(String sourceId, CustomGeometrySourceOptions options) {
    NativeLibrary.ensureLoaded();
    var copiedSourceId = Objects.requireNonNull(sourceId, "sourceId");
    var copiedOptions = Objects.requireNonNull(options, "options");
    if ((copiedOptions.hasTileSize() && copiedOptions.tileSize() < 0)
        || (copiedOptions.hasBuffer() && copiedOptions.buffer() < 0)) {
      JavaCppSupport.setThreadDiagnostic(
          "custom geometry source unsigned options must be non-negative");
      Status.check(MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT);
    }
    var sourceState = new CustomGeometrySourceState(copiedOptions);
    try (var nativeSourceId = JavaCppValues.stringView(copiedSourceId)) {
      Status.check(
          MaplibreNativeC.mln_map_add_custom_geometry_source(
              JavaCppSupport.map(state.requireLiveAddress()),
              nativeSourceId.view(),
              sourceState.descriptor()));
      closeQuietly(customGeometrySources.put(copiedSourceId, sourceState));
    } catch (RuntimeException | Error error) {
      closeQuietly(sourceState);
      throw error;
    }
  }

  public void setCustomGeometrySourceTileData(
      String sourceId, CanonicalTileId tileId, GeoJson data) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(tileId, "tileId");
    checkCanonicalTile(tileId.z(), tileId.x(), tileId.y());
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeTileId = StyleStructs.canonicalTileId(tileId.z(), tileId.x(), tileId.y());
        var nativeData = StyleStructs.geoJson(Objects.requireNonNull(data, "data"))) {
      Status.check(
          MaplibreNativeC.mln_map_set_custom_geometry_source_tile_data(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              nativeTileId.tileId(),
              nativeData.value()));
    }
  }

  public void invalidateCustomGeometrySourceTile(String sourceId, CanonicalTileId tileId) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(tileId, "tileId");
    checkCanonicalTile(tileId.z(), tileId.x(), tileId.y());
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeTileId = StyleStructs.canonicalTileId(tileId.z(), tileId.x(), tileId.y())) {
      Status.check(
          MaplibreNativeC.mln_map_invalidate_custom_geometry_source_tile(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              nativeTileId.tileId()));
    }
  }

  public void invalidateCustomGeometrySourceRegion(String sourceId, LatLngBounds bounds) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(bounds, "bounds");
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeBounds =
            StyleStructs.latLngBounds(
                bounds.southwest().latitude(),
                bounds.southwest().longitude(),
                bounds.northeast().latitude(),
                bounds.northeast().longitude())) {
      Status.check(
          MaplibreNativeC.mln_map_invalidate_custom_geometry_source_region(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              nativeBounds.bounds()));
    }
  }

  public void addVectorSourceUrl(String sourceId, String url) {
    addVectorSourceUrl(sourceId, url, null);
  }

  public void addVectorSourceUrl(String sourceId, String url, TileSourceOptions options) {
    NativeLibrary.ensureLoaded();
    var nativeOptions = tileSourceOptions(options);
    checkTileSourceOptions(nativeOptions);
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeUrl = JavaCppValues.stringView(Objects.requireNonNull(url, "url"));
        var nativeTileOptions =
            StyleStructs.tileSourceOptions(
                nativeOptions.fields(), nativeOptions.values(), nativeOptions.attribution())) {
      Status.check(
          MaplibreNativeC.mln_map_add_vector_source_url(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              nativeUrl.view(),
              nativeTileOptions.options()));
    }
  }

  public void addVectorSourceTiles(String sourceId, List<String> tiles) {
    addVectorSourceTiles(sourceId, tiles, null);
  }

  public void addVectorSourceTiles(String sourceId, List<String> tiles, TileSourceOptions options) {
    NativeLibrary.ensureLoaded();
    var nativeOptions = tileSourceOptions(options);
    checkTileSourceOptions(nativeOptions);
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeTiles = JavaCppValues.stringViews(stringArray(tiles, "tiles"));
        var nativeTileOptions =
            StyleStructs.tileSourceOptions(
                nativeOptions.fields(), nativeOptions.values(), nativeOptions.attribution())) {
      Status.check(
          MaplibreNativeC.mln_map_add_vector_source_tiles(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              nativeTiles.views(),
              nativeTiles.count(),
              nativeTileOptions.options()));
    }
  }

  public void addRasterSourceUrl(String sourceId, String url) {
    addRasterSourceUrl(sourceId, url, null);
  }

  public void addRasterSourceUrl(String sourceId, String url, TileSourceOptions options) {
    NativeLibrary.ensureLoaded();
    var nativeOptions = tileSourceOptions(options);
    checkTileSourceOptions(nativeOptions);
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeUrl = JavaCppValues.stringView(Objects.requireNonNull(url, "url"));
        var nativeTileOptions =
            StyleStructs.tileSourceOptions(
                nativeOptions.fields(), nativeOptions.values(), nativeOptions.attribution())) {
      Status.check(
          MaplibreNativeC.mln_map_add_raster_source_url(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              nativeUrl.view(),
              nativeTileOptions.options()));
    }
  }

  public void addRasterSourceTiles(String sourceId, List<String> tiles) {
    addRasterSourceTiles(sourceId, tiles, null);
  }

  public void addRasterSourceTiles(String sourceId, List<String> tiles, TileSourceOptions options) {
    NativeLibrary.ensureLoaded();
    var nativeOptions = tileSourceOptions(options);
    checkTileSourceOptions(nativeOptions);
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeTiles = JavaCppValues.stringViews(stringArray(tiles, "tiles"));
        var nativeTileOptions =
            StyleStructs.tileSourceOptions(
                nativeOptions.fields(), nativeOptions.values(), nativeOptions.attribution())) {
      Status.check(
          MaplibreNativeC.mln_map_add_raster_source_tiles(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              nativeTiles.views(),
              nativeTiles.count(),
              nativeTileOptions.options()));
    }
  }

  public void addRasterDemSourceUrl(String sourceId, String url) {
    addRasterDemSourceUrl(sourceId, url, null);
  }

  public void addRasterDemSourceUrl(String sourceId, String url, TileSourceOptions options) {
    NativeLibrary.ensureLoaded();
    var nativeOptions = tileSourceOptions(options);
    checkTileSourceOptions(nativeOptions);
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeUrl = JavaCppValues.stringView(Objects.requireNonNull(url, "url"));
        var nativeTileOptions =
            StyleStructs.tileSourceOptions(
                nativeOptions.fields(), nativeOptions.values(), nativeOptions.attribution())) {
      Status.check(
          MaplibreNativeC.mln_map_add_raster_dem_source_url(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              nativeUrl.view(),
              nativeTileOptions.options()));
    }
  }

  public void addRasterDemSourceTiles(String sourceId, List<String> tiles) {
    addRasterDemSourceTiles(sourceId, tiles, null);
  }

  public void addRasterDemSourceTiles(
      String sourceId, List<String> tiles, TileSourceOptions options) {
    NativeLibrary.ensureLoaded();
    var nativeOptions = tileSourceOptions(options);
    checkTileSourceOptions(nativeOptions);
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeTiles = JavaCppValues.stringViews(stringArray(tiles, "tiles"));
        var nativeTileOptions =
            StyleStructs.tileSourceOptions(
                nativeOptions.fields(), nativeOptions.values(), nativeOptions.attribution())) {
      Status.check(
          MaplibreNativeC.mln_map_add_raster_dem_source_tiles(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              nativeTiles.views(),
              nativeTiles.count(),
              nativeTileOptions.options()));
    }
  }

  public void setStyleImage(String imageId, PremultipliedRgba8Image image) {
    setStyleImage(imageId, image, null);
  }

  public void setStyleImage(
      String imageId, PremultipliedRgba8Image image, StyleImageOptions options) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(image, "image");
    try (var id = JavaCppValues.stringView(Objects.requireNonNull(imageId, "imageId"));
        var nativeImage =
            StyleStructs.premultipliedRgba8Image(
                image.width(), image.height(), image.stride(), image.pixels())) {
      var nativeOptions = MaplibreNativeC.mln_style_image_options_default();
      try {
        var fields = 0;
        if (options != null && options.hasPixelRatio()) {
          fields |= MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
          nativeOptions.pixel_ratio(options.pixelRatio());
        }
        if (options != null && options.hasSdf()) {
          fields |= MaplibreNativeC.MLN_STYLE_IMAGE_OPTION_SDF;
          nativeOptions.sdf(options.sdf());
        }
        nativeOptions.fields(fields);
        Status.check(
            MaplibreNativeC.mln_map_set_style_image(
                JavaCppSupport.map(state.requireLiveAddress()),
                id.view(),
                nativeImage.image(),
                nativeOptions));
      } finally {
        nativeOptions.close();
      }
    }
  }

  public boolean removeStyleImage(String imageId) {
    NativeLibrary.ensureLoaded();
    var outRemoved = new boolean[1];
    try (var id = JavaCppValues.stringView(Objects.requireNonNull(imageId, "imageId"))) {
      Status.check(
          MaplibreNativeC.mln_map_remove_style_image(
              JavaCppSupport.map(state.requireLiveAddress()), id.view(), outRemoved));
    }
    return outRemoved[0];
  }

  public boolean styleImageExists(String imageId) {
    NativeLibrary.ensureLoaded();
    var outExists = new boolean[1];
    try (var id = JavaCppValues.stringView(Objects.requireNonNull(imageId, "imageId"))) {
      Status.check(
          MaplibreNativeC.mln_map_style_image_exists(
              JavaCppSupport.map(state.requireLiveAddress()), id.view(), outExists));
    }
    return outExists[0];
  }

  public Optional<StyleImageInfo> styleImageInfo(String imageId) {
    NativeLibrary.ensureLoaded();
    try (var id = JavaCppValues.stringView(Objects.requireNonNull(imageId, "imageId"))) {
      var info = MaplibreNativeC.mln_style_image_info_default();
      var outFound = new boolean[1];
      Status.check(
          MaplibreNativeC.mln_map_get_style_image_info(
              JavaCppSupport.map(state.requireLiveAddress()), id.view(), info, outFound));
      return outFound[0]
          ? Optional.of(
              new StyleImageInfo(
                  info.width(),
                  info.height(),
                  info.stride(),
                  info.byte_length(),
                  info.pixel_ratio(),
                  info.sdf()))
          : Optional.empty();
    }
  }

  public Optional<StyleImage> copyStyleImagePremultipliedRgba8(String imageId) {
    var info = styleImageInfo(imageId);
    if (info.isEmpty()) {
      return Optional.empty();
    }
    var imageInfo = info.orElseThrow();
    var outPixels = new byte[Math.toIntExact(imageInfo.byteLength())];
    var outFound = new boolean[1];
    try (var id = JavaCppValues.stringView(Objects.requireNonNull(imageId, "imageId"))) {
      try (var outByteLength = new SizeTPointer(1)) {
        Status.check(
            MaplibreNativeC.mln_map_copy_style_image_premultiplied_rgba8(
                JavaCppSupport.map(state.requireLiveAddress()),
                id.view(),
                outPixels,
                outPixels.length,
                outByteLength,
                outFound));
      }
    }
    if (!outFound[0]) {
      return Optional.empty();
    }
    return Optional.of(
        new StyleImage(
            new PremultipliedRgba8Image(
                imageInfo.width(), imageInfo.height(), imageInfo.stride(), outPixels),
            imageInfo.pixelRatio(),
            imageInfo.sdf()));
  }

  public void addImageSourceUrl(String sourceId, List<LatLng> coordinates, String url) {
    NativeLibrary.ensureLoaded();
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeCoordinates =
            StyleStructs.latLngArray(coordinateArray(coordinates, "coordinates"));
        var nativeUrl = JavaCppValues.stringView(Objects.requireNonNull(url, "url"))) {
      Status.check(
          MaplibreNativeC.mln_map_add_image_source_url(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              nativeCoordinates.coordinates(),
              nativeCoordinates.count(),
              nativeUrl.view()));
    }
  }

  public void addImageSourceImage(
      String sourceId, List<LatLng> coordinates, PremultipliedRgba8Image image) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(image, "image");
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeCoordinates =
            StyleStructs.latLngArray(coordinateArray(coordinates, "coordinates"));
        var nativeImage =
            StyleStructs.premultipliedRgba8Image(
                image.width(), image.height(), image.stride(), image.pixels())) {
      Status.check(
          MaplibreNativeC.mln_map_add_image_source_image(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              nativeCoordinates.coordinates(),
              nativeCoordinates.count(),
              nativeImage.image()));
    }
  }

  public void setImageSourceUrl(String sourceId, String url) {
    NativeLibrary.ensureLoaded();
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeUrl = JavaCppValues.stringView(Objects.requireNonNull(url, "url"))) {
      Status.check(
          MaplibreNativeC.mln_map_set_image_source_url(
              JavaCppSupport.map(state.requireLiveAddress()), source.view(), nativeUrl.view()));
    }
  }

  public void setImageSourceImage(String sourceId, PremultipliedRgba8Image image) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(image, "image");
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeImage =
            StyleStructs.premultipliedRgba8Image(
                image.width(), image.height(), image.stride(), image.pixels())) {
      Status.check(
          MaplibreNativeC.mln_map_set_image_source_image(
              JavaCppSupport.map(state.requireLiveAddress()), source.view(), nativeImage.image()));
    }
  }

  public void setImageSourceCoordinates(String sourceId, List<LatLng> coordinates) {
    NativeLibrary.ensureLoaded();
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeCoordinates =
            StyleStructs.latLngArray(coordinateArray(coordinates, "coordinates"))) {
      Status.check(
          MaplibreNativeC.mln_map_set_image_source_coordinates(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              nativeCoordinates.coordinates(),
              nativeCoordinates.count()));
    }
  }

  public Optional<List<LatLng>> imageSourceCoordinates(String sourceId) {
    NativeLibrary.ensureLoaded();
    var outCoordinates = new double[8];
    var outCoordinateCount = new long[1];
    var outFound = new boolean[1];
    try (var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var nativeCoordinates = new StyleStructs.LatLngArrayScope(4);
        var nativeCoordinateCount = new SizeTPointer(1)) {
      Status.check(
          MaplibreNativeC.mln_map_get_image_source_coordinates(
              JavaCppSupport.map(state.requireLiveAddress()),
              source.view(),
              nativeCoordinates.coordinates(),
              nativeCoordinates.count(),
              nativeCoordinateCount,
              outFound));
      outCoordinateCount[0] = nativeCoordinateCount.get();
      nativeCoordinates.copyTo(outCoordinates, outCoordinateCount[0]);
    }
    if (!outFound[0]) {
      return Optional.empty();
    }
    var coordinates = new ArrayList<LatLng>(Math.toIntExact(outCoordinateCount[0]));
    for (var index = 0; index < outCoordinateCount[0]; index++) {
      coordinates.add(new LatLng(outCoordinates[index * 2], outCoordinates[index * 2 + 1]));
    }
    return Optional.of(List.copyOf(coordinates));
  }

  public void addStyleLayerJson(JsonValue layerJson) {
    addStyleLayerJson(layerJson, "");
  }

  public void addStyleLayerJson(JsonValue layerJson, String beforeLayerId) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.json(Objects.requireNonNull(layerJson, "layerJson"));
        var before =
            JavaCppValues.stringView(Objects.requireNonNull(beforeLayerId, "beforeLayerId"))) {
      Status.check(
          MaplibreNativeC.mln_map_add_style_layer_json(
              JavaCppSupport.map(state.requireLiveAddress()), layer.value(), before.view()));
    }
  }

  public void addHillshadeLayer(String layerId, String sourceId) {
    addHillshadeLayer(layerId, sourceId, "");
  }

  public void addHillshadeLayer(String layerId, String sourceId, String beforeLayerId) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"));
        var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var before =
            JavaCppValues.stringView(Objects.requireNonNull(beforeLayerId, "beforeLayerId"))) {
      Status.check(
          MaplibreNativeC.mln_map_add_hillshade_layer(
              JavaCppSupport.map(state.requireLiveAddress()),
              layer.view(),
              source.view(),
              before.view()));
    }
  }

  public void addColorReliefLayer(String layerId, String sourceId) {
    addColorReliefLayer(layerId, sourceId, "");
  }

  public void addColorReliefLayer(String layerId, String sourceId, String beforeLayerId) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"));
        var source = JavaCppValues.stringView(Objects.requireNonNull(sourceId, "sourceId"));
        var before =
            JavaCppValues.stringView(Objects.requireNonNull(beforeLayerId, "beforeLayerId"))) {
      Status.check(
          MaplibreNativeC.mln_map_add_color_relief_layer(
              JavaCppSupport.map(state.requireLiveAddress()),
              layer.view(),
              source.view(),
              before.view()));
    }
  }

  public void addLocationIndicatorLayer(String layerId) {
    addLocationIndicatorLayer(layerId, "");
  }

  public void addLocationIndicatorLayer(String layerId, String beforeLayerId) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"));
        var before =
            JavaCppValues.stringView(Objects.requireNonNull(beforeLayerId, "beforeLayerId"))) {
      Status.check(
          MaplibreNativeC.mln_map_add_location_indicator_layer(
              JavaCppSupport.map(state.requireLiveAddress()), layer.view(), before.view()));
    }
  }

  public void setLocationIndicatorLocation(String layerId, LatLng coordinate, double altitude) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(coordinate, "coordinate");
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"));
        var nativeCoordinate = new MaplibreNativeC.mln_lat_lng()) {
      nativeCoordinate.latitude(coordinate.latitude());
      nativeCoordinate.longitude(coordinate.longitude());
      Status.check(
          MaplibreNativeC.mln_map_set_location_indicator_location(
              JavaCppSupport.map(state.requireLiveAddress()),
              layer.view(),
              nativeCoordinate,
              altitude));
    }
  }

  public void setLocationIndicatorBearing(String layerId, double bearing) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"))) {
      Status.check(
          MaplibreNativeC.mln_map_set_location_indicator_bearing(
              JavaCppSupport.map(state.requireLiveAddress()), layer.view(), bearing));
    }
  }

  public void setLocationIndicatorAccuracyRadius(String layerId, double radius) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"))) {
      Status.check(
          MaplibreNativeC.mln_map_set_location_indicator_accuracy_radius(
              JavaCppSupport.map(state.requireLiveAddress()), layer.view(), radius));
    }
  }

  public void setLocationIndicatorImageName(
      String layerId, LocationIndicatorImageKind imageKind, String imageId) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"));
        var image = JavaCppValues.stringView(Objects.requireNonNull(imageId, "imageId"))) {
      Status.check(
          MaplibreNativeC.mln_map_set_location_indicator_image_name(
              JavaCppSupport.map(state.requireLiveAddress()),
              layer.view(),
              Objects.requireNonNull(imageKind, "imageKind").nativeValue(),
              image.view()));
    }
  }

  public boolean removeStyleLayer(String layerId) {
    NativeLibrary.ensureLoaded();
    var outRemoved = new boolean[1];
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"))) {
      Status.check(
          MaplibreNativeC.mln_map_remove_style_layer(
              JavaCppSupport.map(state.requireLiveAddress()), layer.view(), outRemoved));
    }
    return outRemoved[0];
  }

  public boolean styleLayerExists(String layerId) {
    NativeLibrary.ensureLoaded();
    var outExists = new boolean[1];
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"))) {
      Status.check(
          MaplibreNativeC.mln_map_style_layer_exists(
              JavaCppSupport.map(state.requireLiveAddress()), layer.view(), outExists));
    }
    return outExists[0];
  }

  public Optional<String> styleLayerType(String layerId) {
    NativeLibrary.ensureLoaded();
    var outLayerType = new MaplibreNativeC.mln_string_view();
    var outFound = new boolean[1];
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"))) {
      Status.check(
          MaplibreNativeC.mln_map_get_style_layer_type(
              JavaCppSupport.map(state.requireLiveAddress()),
              layer.view(),
              outLayerType,
              outFound));
      return outFound[0] ? Optional.of(JavaCppValues.string(outLayerType)) : Optional.empty();
    } finally {
      outLayerType.close();
    }
  }

  public List<String> styleLayerIds() {
    NativeLibrary.ensureLoaded();
    var outList = JavaCppSupport.outPointer(MaplibreNativeC.mln_style_id_list.class);
    Status.check(
        MaplibreNativeC.mln_map_list_style_layer_ids(
            JavaCppSupport.map(state.requireLiveAddress()), outList));
    var list =
        new MaplibreNativeC.mln_style_id_list(
            JavaCppSupport.pointer(
                JavaCppSupport.outAddress(outList, MaplibreNativeC.mln_style_id_list.class)));
    return List.of(StyleStructs.styleIdList(list));
  }

  public void moveStyleLayer(String layerId) {
    moveStyleLayer(layerId, "");
  }

  public void moveStyleLayer(String layerId, String beforeLayerId) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"));
        var before =
            JavaCppValues.stringView(Objects.requireNonNull(beforeLayerId, "beforeLayerId"))) {
      Status.check(
          MaplibreNativeC.mln_map_move_style_layer(
              JavaCppSupport.map(state.requireLiveAddress()), layer.view(), before.view()));
    }
  }

  public Optional<JsonValue> styleLayerJson(String layerId) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"));
        var outFound = new BoolPointer(1)) {
      var outSnapshot = JavaCppSupport.outPointer(MaplibreNativeC.mln_json_snapshot.class);
      Status.check(
          MaplibreNativeC.mln_map_get_style_layer_json(
              JavaCppSupport.map(state.requireLiveAddress()), layer.view(), outSnapshot, outFound));
      return outFound.get()
          ? Optional.of(StyleStructs.jsonSnapshot(outSnapshot))
          : Optional.empty();
    }
  }

  public void setStyleLightJson(JsonValue lightJson) {
    NativeLibrary.ensureLoaded();
    try (var light = JavaCppValues.json(Objects.requireNonNull(lightJson, "lightJson"))) {
      Status.check(
          MaplibreNativeC.mln_map_set_style_light_json(
              JavaCppSupport.map(state.requireLiveAddress()), light.value()));
    }
  }

  public void setStyleLightProperty(String propertyName, JsonValue value) {
    NativeLibrary.ensureLoaded();
    try (var property =
            JavaCppValues.stringView(Objects.requireNonNull(propertyName, "propertyName"));
        var nativeValue = JavaCppValues.json(Objects.requireNonNull(value, "value"))) {
      Status.check(
          MaplibreNativeC.mln_map_set_style_light_property(
              JavaCppSupport.map(state.requireLiveAddress()),
              property.view(),
              nativeValue.value()));
    }
  }

  public Optional<JsonValue> styleLightProperty(String propertyName) {
    NativeLibrary.ensureLoaded();
    try (var property =
        JavaCppValues.stringView(Objects.requireNonNull(propertyName, "propertyName"))) {
      var outSnapshot = JavaCppSupport.outPointer(MaplibreNativeC.mln_json_snapshot.class);
      Status.check(
          MaplibreNativeC.mln_map_get_style_light_property(
              JavaCppSupport.map(state.requireLiveAddress()), property.view(), outSnapshot));
      return Optional.ofNullable(StyleStructs.jsonSnapshot(outSnapshot));
    }
  }

  public void setLayerProperty(String layerId, String propertyName, JsonValue value) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"));
        var property =
            JavaCppValues.stringView(Objects.requireNonNull(propertyName, "propertyName"));
        var nativeValue = JavaCppValues.json(Objects.requireNonNull(value, "value"))) {
      Status.check(
          MaplibreNativeC.mln_map_set_layer_property(
              JavaCppSupport.map(state.requireLiveAddress()),
              layer.view(),
              property.view(),
              nativeValue.value()));
    }
  }

  public Optional<JsonValue> layerProperty(String layerId, String propertyName) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"));
        var property =
            JavaCppValues.stringView(Objects.requireNonNull(propertyName, "propertyName"))) {
      var outSnapshot = JavaCppSupport.outPointer(MaplibreNativeC.mln_json_snapshot.class);
      Status.check(
          MaplibreNativeC.mln_map_get_layer_property(
              JavaCppSupport.map(state.requireLiveAddress()),
              layer.view(),
              property.view(),
              outSnapshot));
      return Optional.ofNullable(StyleStructs.jsonSnapshot(outSnapshot));
    }
  }

  public void setLayerFilter(String layerId, JsonValue filter) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"));
        var nativeFilter = JavaCppValues.json(Objects.requireNonNull(filter, "filter"))) {
      Status.check(
          MaplibreNativeC.mln_map_set_layer_filter(
              JavaCppSupport.map(state.requireLiveAddress()), layer.view(), nativeFilter.value()));
    }
  }

  public void clearLayerFilter(String layerId) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"))) {
      Status.check(
          MaplibreNativeC.mln_map_set_layer_filter(
              JavaCppSupport.map(state.requireLiveAddress()), layer.view(), null));
    }
  }

  public Optional<JsonValue> layerFilter(String layerId) {
    NativeLibrary.ensureLoaded();
    try (var layer = JavaCppValues.stringView(Objects.requireNonNull(layerId, "layerId"))) {
      var outSnapshot = JavaCppSupport.outPointer(MaplibreNativeC.mln_json_snapshot.class);
      Status.check(
          MaplibreNativeC.mln_map_get_layer_filter(
              JavaCppSupport.map(state.requireLiveAddress()), layer.view(), outSnapshot));
      return Optional.ofNullable(StyleStructs.jsonSnapshot(outSnapshot));
    }
  }

  public RenderSessionHandle attachMetalOwnedTexture(MetalOwnedTextureDescriptor descriptor) {
    return RenderSessionHandle.attachMetalOwnedTexture(this, descriptor);
  }

  public RenderSessionHandle attachMetalBorrowedTexture(MetalBorrowedTextureDescriptor descriptor) {
    return RenderSessionHandle.attachMetalBorrowedTexture(this, descriptor);
  }

  public RenderSessionHandle attachVulkanOwnedTexture(VulkanOwnedTextureDescriptor descriptor) {
    return RenderSessionHandle.attachVulkanOwnedTexture(this, descriptor);
  }

  public RenderSessionHandle attachVulkanBorrowedTexture(
      VulkanBorrowedTextureDescriptor descriptor) {
    return RenderSessionHandle.attachVulkanBorrowedTexture(this, descriptor);
  }

  public RenderSessionHandle attachOpenGLOwnedTexture(OpenGLOwnedTextureDescriptor descriptor) {
    return RenderSessionHandle.attachOpenGLOwnedTexture(this, descriptor);
  }

  public RenderSessionHandle attachOpenGLBorrowedTexture(
      OpenGLBorrowedTextureDescriptor descriptor) {
    return RenderSessionHandle.attachOpenGLBorrowedTexture(this, descriptor);
  }

  public RenderSessionHandle attachMetalSurface(MetalSurfaceDescriptor descriptor) {
    return RenderSessionHandle.attachMetalSurface(this, descriptor);
  }

  public RenderSessionHandle attachVulkanSurface(VulkanSurfaceDescriptor descriptor) {
    return RenderSessionHandle.attachVulkanSurface(this, descriptor);
  }

  public RenderSessionHandle attachOpenGLSurface(OpenGLSurfaceDescriptor descriptor) {
    return RenderSessionHandle.attachOpenGLSurface(this, descriptor);
  }

  public void requestRepaint() {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_map_request_repaint(JavaCppSupport.map(state.requireLiveAddress())));
  }

  public void requestStillImage() {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_map_request_still_image(
            JavaCppSupport.map(state.requireLiveAddress())));
  }

  public void setDebugOptions(Set<DebugOption> options) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(options, "options");
    var mask = 0;
    for (var option : options) {
      mask |= Objects.requireNonNull(option, "option").nativeMask();
    }
    Status.check(
        MaplibreNativeC.mln_map_set_debug_options(
            JavaCppSupport.map(state.requireLiveAddress()), mask));
  }

  public EnumSet<DebugOption> debugOptions() {
    NativeLibrary.ensureLoaded();
    var outOptions = new int[1];
    Status.check(
        MaplibreNativeC.mln_map_get_debug_options(
            JavaCppSupport.map(state.requireLiveAddress()), outOptions));
    var options = EnumSet.noneOf(DebugOption.class);
    for (var option : DebugOption.values()) {
      if ((outOptions[0] & option.nativeMask()) != 0) {
        options.add(option);
      }
    }
    return options;
  }

  public void setRenderingStatsViewEnabled(boolean enabled) {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_map_set_rendering_stats_view_enabled(
            JavaCppSupport.map(state.requireLiveAddress()), enabled));
  }

  public boolean isRenderingStatsViewEnabled() {
    NativeLibrary.ensureLoaded();
    var outEnabled = new boolean[1];
    Status.check(
        MaplibreNativeC.mln_map_get_rendering_stats_view_enabled(
            JavaCppSupport.map(state.requireLiveAddress()), outEnabled));
    return outEnabled[0];
  }

  public boolean isFullyLoaded() {
    NativeLibrary.ensureLoaded();
    var outLoaded = new boolean[1];
    Status.check(
        MaplibreNativeC.mln_map_is_fully_loaded(
            JavaCppSupport.map(state.requireLiveAddress()), outLoaded));
    return outLoaded[0];
  }

  public void dumpDebugLogs() {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_map_dump_debug_logs(JavaCppSupport.map(state.requireLiveAddress())));
  }

  public ViewportOptions viewportOptions() {
    NativeLibrary.ensureLoaded();
    var nativeOptions = MaplibreNativeC.mln_map_viewport_options_default();
    Status.check(
        MaplibreNativeC.mln_map_get_viewport_options(
            JavaCppSupport.map(state.requireLiveAddress()), nativeOptions));
    var fields = nativeOptions.fields();
    var options = new ViewportOptions();
    if ((fields & MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0) {
      options.northOrientation(NorthOrientation.fromNative(nativeOptions.north_orientation()));
    }
    if ((fields & MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0) {
      options.constrainMode(ConstrainMode.fromNative(nativeOptions.constrain_mode()));
    }
    if ((fields & MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0) {
      options.viewportMode(ViewportMode.fromNative(nativeOptions.viewport_mode()));
    }
    if ((fields & MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0) {
      options.frustumOffset(
          new EdgeInsets(
              nativeOptions.frustum_offset().top(),
              nativeOptions.frustum_offset().left(),
              nativeOptions.frustum_offset().bottom(),
              nativeOptions.frustum_offset().right()));
    }
    return options;
  }

  public void setViewportOptions(ViewportOptions options) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(options, "options");
    var nativeOptions = MaplibreNativeC.mln_map_viewport_options_default();
    var fields = 0;
    if (options.hasNorthOrientation()) {
      fields |= MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
      nativeOptions.north_orientation(options.northOrientation().nativeValue());
    }
    if (options.hasConstrainMode()) {
      fields |= MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
      nativeOptions.constrain_mode(options.constrainMode().nativeValue());
    }
    if (options.hasViewportMode()) {
      fields |= MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
      nativeOptions.viewport_mode(options.viewportMode().nativeValue());
    }
    if (options.hasFrustumOffset()) {
      fields |= MaplibreNativeC.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
      nativeOptions.frustum_offset(
          new MaplibreNativeC.mln_edge_insets()
              .top(options.frustumOffset().top())
              .left(options.frustumOffset().left())
              .bottom(options.frustumOffset().bottom())
              .right(options.frustumOffset().right()));
    }
    nativeOptions.fields(fields);
    Status.check(
        MaplibreNativeC.mln_map_set_viewport_options(
            JavaCppSupport.map(state.requireLiveAddress()), nativeOptions));
  }

  public TileOptions tileOptions() {
    NativeLibrary.ensureLoaded();
    var nativeOptions = MaplibreNativeC.mln_map_tile_options_default();
    Status.check(
        MaplibreNativeC.mln_map_get_tile_options(
            JavaCppSupport.map(state.requireLiveAddress()), nativeOptions));
    var fields = nativeOptions.fields();
    var options = new TileOptions();
    if ((fields & MaplibreNativeC.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0) {
      options.prefetchZoomDelta(nativeOptions.prefetch_zoom_delta());
    }
    if ((fields & MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0) {
      options.lodMinRadius(nativeOptions.lod_min_radius());
    }
    if ((fields & MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_SCALE) != 0) {
      options.lodScale(nativeOptions.lod_scale());
    }
    if ((fields & MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0) {
      options.lodPitchThreshold(nativeOptions.lod_pitch_threshold());
    }
    if ((fields & MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0) {
      options.lodZoomShift(nativeOptions.lod_zoom_shift());
    }
    if ((fields & MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_MODE) != 0) {
      options.lodMode(TileLodMode.fromNative(nativeOptions.lod_mode()));
    }
    return options;
  }

  public void setTileOptions(TileOptions options) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(options, "options");
    var nativeOptions = MaplibreNativeC.mln_map_tile_options_default();
    var fields = 0;
    if (options.hasPrefetchZoomDelta()) {
      fields |= MaplibreNativeC.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
      nativeOptions.prefetch_zoom_delta(options.prefetchZoomDelta());
    }
    if (options.hasLodMinRadius()) {
      fields |= MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
      nativeOptions.lod_min_radius(options.lodMinRadius());
    }
    if (options.hasLodScale()) {
      fields |= MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_SCALE;
      nativeOptions.lod_scale(options.lodScale());
    }
    if (options.hasLodPitchThreshold()) {
      fields |= MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
      nativeOptions.lod_pitch_threshold(options.lodPitchThreshold());
    }
    if (options.hasLodZoomShift()) {
      fields |= MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
      nativeOptions.lod_zoom_shift(options.lodZoomShift());
    }
    if (options.hasLodMode()) {
      fields |= MaplibreNativeC.MLN_MAP_TILE_OPTION_LOD_MODE;
      nativeOptions.lod_mode(options.lodMode().nativeValue());
    }
    nativeOptions.fields(fields);
    Status.check(
        MaplibreNativeC.mln_map_set_tile_options(
            JavaCppSupport.map(state.requireLiveAddress()), nativeOptions));
  }

  public CameraOptions camera() {
    NativeLibrary.ensureLoaded();
    var outCamera = MaplibreNativeC.mln_camera_options_default();
    Status.check(
        MaplibreNativeC.mln_map_get_camera(
            JavaCppSupport.map(state.requireLiveAddress()), outCamera));
    return MapStructs.cameraOptions(outCamera);
  }

  public void jumpTo(CameraOptions camera) {
    NativeLibrary.ensureLoaded();
    var nativeCamera = MapStructs.nativeCameraOptions(camera);
    Status.check(
        MaplibreNativeC.mln_map_jump_to(
            JavaCppSupport.map(state.requireLiveAddress()), nativeCamera));
  }

  public void easeTo(CameraOptions camera) {
    easeToInternal(camera, null, false);
  }

  public void easeTo(CameraOptions camera, AnimationOptions animation) {
    easeToInternal(camera, Objects.requireNonNull(animation, "animation"), true);
  }

  public void flyTo(CameraOptions camera) {
    flyToInternal(camera, null, false);
  }

  public void flyTo(CameraOptions camera, AnimationOptions animation) {
    flyToInternal(camera, Objects.requireNonNull(animation, "animation"), true);
  }

  public void moveBy(double deltaX, double deltaY) {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_map_move_by(
            JavaCppSupport.map(state.requireLiveAddress()), deltaX, deltaY));
  }

  public void moveByAnimated(double deltaX, double deltaY) {
    moveByAnimatedInternal(deltaX, deltaY, null, false);
  }

  public void moveByAnimated(double deltaX, double deltaY, AnimationOptions animation) {
    moveByAnimatedInternal(deltaX, deltaY, Objects.requireNonNull(animation, "animation"), true);
  }

  public void scaleBy(double scale) {
    scaleByInternal(scale, null, false);
  }

  public void scaleBy(double scale, ScreenPoint anchor) {
    scaleByInternal(scale, Objects.requireNonNull(anchor, "anchor"), false);
  }

  public void scaleByAnimated(double scale) {
    scaleByInternal(scale, null, true);
  }

  public void scaleByAnimated(double scale, ScreenPoint anchor) {
    scaleByInternal(scale, Objects.requireNonNull(anchor, "anchor"), true);
  }

  public void scaleByAnimated(double scale, AnimationOptions animation) {
    scaleByAnimatedInternal(
        scale, null, false, Objects.requireNonNull(animation, "animation"), true);
  }

  public void scaleByAnimated(double scale, ScreenPoint anchor, AnimationOptions animation) {
    scaleByAnimatedInternal(
        scale,
        Objects.requireNonNull(anchor, "anchor"),
        true,
        Objects.requireNonNull(animation, "animation"),
        true);
  }

  public void rotateBy(ScreenPoint first, ScreenPoint second) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(first, "first");
    Objects.requireNonNull(second, "second");
    Status.check(
        MaplibreNativeC.mln_map_rotate_by(
            JavaCppSupport.map(state.requireLiveAddress()),
            new MaplibreNativeC.mln_screen_point().x(first.x()).y(first.y()),
            new MaplibreNativeC.mln_screen_point().x(second.x()).y(second.y())));
  }

  public void rotateByAnimated(ScreenPoint first, ScreenPoint second) {
    rotateByAnimatedInternal(first, second, null, false);
  }

  public void rotateByAnimated(ScreenPoint first, ScreenPoint second, AnimationOptions animation) {
    rotateByAnimatedInternal(first, second, Objects.requireNonNull(animation, "animation"), true);
  }

  public void pitchBy(double pitch) {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_map_pitch_by(JavaCppSupport.map(state.requireLiveAddress()), pitch));
  }

  public void pitchByAnimated(double pitch) {
    pitchByAnimatedInternal(pitch, null, false);
  }

  public void pitchByAnimated(double pitch, AnimationOptions animation) {
    pitchByAnimatedInternal(pitch, Objects.requireNonNull(animation, "animation"), true);
  }

  public void cancelTransitions() {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_map_cancel_transitions(JavaCppSupport.map(state.requireLiveAddress())));
  }

  private void easeToInternal(
      CameraOptions camera, AnimationOptions animation, boolean hasAnimation) {
    NativeLibrary.ensureLoaded();
    var nativeCamera = MapStructs.nativeCameraOptions(camera);
    var nativeAnimation = nativeAnimation(animationToNative(animation, hasAnimation), hasAnimation);
    Status.check(
        MaplibreNativeC.mln_map_ease_to(
            JavaCppSupport.map(state.requireLiveAddress()), nativeCamera, nativeAnimation));
  }

  private void flyToInternal(
      CameraOptions camera, AnimationOptions animation, boolean hasAnimation) {
    NativeLibrary.ensureLoaded();
    var nativeCamera = MapStructs.nativeCameraOptions(camera);
    var nativeAnimation = nativeAnimation(animationToNative(animation, hasAnimation), hasAnimation);
    Status.check(
        MaplibreNativeC.mln_map_fly_to(
            JavaCppSupport.map(state.requireLiveAddress()), nativeCamera, nativeAnimation));
  }

  private void moveByAnimatedInternal(
      double deltaX, double deltaY, AnimationOptions animation, boolean hasAnimation) {
    NativeLibrary.ensureLoaded();
    var nativeAnimation = nativeAnimation(animationToNative(animation, hasAnimation), hasAnimation);
    Status.check(
        MaplibreNativeC.mln_map_move_by_animated(
            JavaCppSupport.map(state.requireLiveAddress()), deltaX, deltaY, nativeAnimation));
  }

  private void scaleByInternal(double scale, ScreenPoint anchor, boolean animated) {
    if (animated) {
      scaleByAnimatedInternal(scale, anchor, anchor != null, null, false);
      return;
    }
    NativeLibrary.ensureLoaded();
    var hasAnchor = anchor != null;
    var anchorX = hasAnchor ? anchor.x() : 0;
    var anchorY = hasAnchor ? anchor.y() : 0;
    Status.check(
        MaplibreNativeC.mln_map_scale_by(
            JavaCppSupport.map(state.requireLiveAddress()),
            scale,
            hasAnchor ? new MaplibreNativeC.mln_screen_point().x(anchorX).y(anchorY) : null));
  }

  private void scaleByAnimatedInternal(
      double scale,
      ScreenPoint anchor,
      boolean hasAnchor,
      AnimationOptions animation,
      boolean hasAnimation) {
    NativeLibrary.ensureLoaded();
    var anchorX = hasAnchor ? anchor.x() : 0;
    var anchorY = hasAnchor ? anchor.y() : 0;
    var nativeAnimation = nativeAnimation(animationToNative(animation, hasAnimation), hasAnimation);
    Status.check(
        MaplibreNativeC.mln_map_scale_by_animated(
            JavaCppSupport.map(state.requireLiveAddress()),
            scale,
            hasAnchor ? new MaplibreNativeC.mln_screen_point().x(anchorX).y(anchorY) : null,
            nativeAnimation));
  }

  private void rotateByAnimatedInternal(
      ScreenPoint first, ScreenPoint second, AnimationOptions animation, boolean hasAnimation) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(first, "first");
    Objects.requireNonNull(second, "second");
    var nativeAnimation = nativeAnimation(animationToNative(animation, hasAnimation), hasAnimation);
    Status.check(
        MaplibreNativeC.mln_map_rotate_by_animated(
            JavaCppSupport.map(state.requireLiveAddress()),
            new MaplibreNativeC.mln_screen_point().x(first.x()).y(first.y()),
            new MaplibreNativeC.mln_screen_point().x(second.x()).y(second.y()),
            nativeAnimation));
  }

  private void pitchByAnimatedInternal(
      double pitch, AnimationOptions animation, boolean hasAnimation) {
    NativeLibrary.ensureLoaded();
    var nativeAnimation = nativeAnimation(animationToNative(animation, hasAnimation), hasAnimation);
    Status.check(
        MaplibreNativeC.mln_map_pitch_by_animated(
            JavaCppSupport.map(state.requireLiveAddress()), pitch, nativeAnimation));
  }

  public CameraOptions cameraForLatLngBounds(LatLngBounds bounds) {
    return cameraForLatLngBoundsInternal(bounds, null, false);
  }

  public CameraOptions cameraForLatLngBounds(LatLngBounds bounds, CameraFitOptions fitOptions) {
    return cameraForLatLngBoundsInternal(
        bounds, Objects.requireNonNull(fitOptions, "fitOptions"), true);
  }

  public CameraOptions cameraForLatLngs(List<LatLng> coordinates) {
    return cameraForLatLngsInternal(coordinates, null, false);
  }

  public CameraOptions cameraForLatLngs(List<LatLng> coordinates, CameraFitOptions fitOptions) {
    return cameraForLatLngsInternal(
        coordinates, Objects.requireNonNull(fitOptions, "fitOptions"), true);
  }

  public CameraOptions cameraForGeometry(Geometry geometry) {
    return cameraForGeometryInternal(geometry, null, false);
  }

  public CameraOptions cameraForGeometry(Geometry geometry, CameraFitOptions fitOptions) {
    return cameraForGeometryInternal(
        geometry, Objects.requireNonNull(fitOptions, "fitOptions"), true);
  }

  public LatLngBounds latLngBoundsForCamera(CameraOptions camera) {
    return latLngBoundsForCameraInternal(camera, false);
  }

  public LatLngBounds latLngBoundsForCameraUnwrapped(CameraOptions camera) {
    return latLngBoundsForCameraInternal(camera, true);
  }

  private CameraOptions cameraForLatLngBoundsInternal(
      LatLngBounds bounds, CameraFitOptions fitOptions, boolean hasFitOptions) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(bounds, "bounds");
    var nativeFit = nativeFit(fitToNative(fitOptions, hasFitOptions), hasFitOptions);
    var outCamera = MaplibreNativeC.mln_camera_options_default();
    Status.check(
        MaplibreNativeC.mln_map_camera_for_lat_lng_bounds(
            JavaCppSupport.map(state.requireLiveAddress()),
            nativeLatLngBounds(
                bounds.southwest().latitude(),
                bounds.southwest().longitude(),
                bounds.northeast().latitude(),
                bounds.northeast().longitude()),
            nativeFit,
            outCamera));
    return MapStructs.cameraOptions(outCamera);
  }

  private CameraOptions cameraForLatLngsInternal(
      List<LatLng> coordinates, CameraFitOptions fitOptions, boolean hasFitOptions) {
    NativeLibrary.ensureLoaded();
    var copiedCoordinates = List.copyOf(Objects.requireNonNull(coordinates, "coordinates"));
    var nativeFit = nativeFit(fitToNative(fitOptions, hasFitOptions), hasFitOptions);
    try (var nativeCoordinates =
        org.maplibre.nativejni.internal.struct.CoreStructs.latLngArray(copiedCoordinates)) {
      var outCamera = MaplibreNativeC.mln_camera_options_default();
      Status.check(
          MaplibreNativeC.mln_map_camera_for_lat_lngs(
              JavaCppSupport.map(state.requireLiveAddress()),
              nativeCoordinates.coordinates(),
              nativeCoordinates.count(),
              nativeFit,
              outCamera));
      return MapStructs.cameraOptions(outCamera);
    }
  }

  private CameraOptions cameraForGeometryInternal(
      Geometry geometry, CameraFitOptions fitOptions, boolean hasFitOptions) {
    NativeLibrary.ensureLoaded();
    var nativeFit = nativeFit(fitToNative(fitOptions, hasFitOptions), hasFitOptions);
    try (var nativeGeometry =
        JavaCppValues.geometry(Objects.requireNonNull(geometry, "geometry"))) {
      var outCamera = MaplibreNativeC.mln_camera_options_default();
      Status.check(
          MaplibreNativeC.mln_map_camera_for_geometry(
              JavaCppSupport.map(state.requireLiveAddress()),
              nativeGeometry.value(),
              nativeFit,
              outCamera));
      return MapStructs.cameraOptions(outCamera);
    }
  }

  private LatLngBounds latLngBoundsForCameraInternal(CameraOptions camera, boolean unwrapped) {
    NativeLibrary.ensureLoaded();
    var nativeCamera = MapStructs.nativeCameraOptions(camera);
    var outBounds = new MaplibreNativeC.mln_lat_lng_bounds();
    var status =
        unwrapped
            ? MaplibreNativeC.mln_map_lat_lng_bounds_for_camera_unwrapped(
                JavaCppSupport.map(state.requireLiveAddress()), nativeCamera, outBounds)
            : MaplibreNativeC.mln_map_lat_lng_bounds_for_camera(
                JavaCppSupport.map(state.requireLiveAddress()), nativeCamera, outBounds);
    Status.check(status);
    return latLngBounds(outBounds);
  }

  public BoundOptions bounds() {
    NativeLibrary.ensureLoaded();
    var outBounds = MaplibreNativeC.mln_bound_options_default();
    Status.check(
        MaplibreNativeC.mln_map_get_bounds(
            JavaCppSupport.map(state.requireLiveAddress()), outBounds));
    var fields = outBounds.fields();
    var nativeBounds = outBounds.bounds();
    return boundsFromNative(
        new boolean[] {
          (fields & MaplibreNativeC.MLN_BOUND_OPTION_BOUNDS) != 0,
          (fields & MaplibreNativeC.MLN_BOUND_OPTION_MIN_ZOOM) != 0,
          (fields & MaplibreNativeC.MLN_BOUND_OPTION_MAX_ZOOM) != 0,
          (fields & MaplibreNativeC.MLN_BOUND_OPTION_MIN_PITCH) != 0,
          (fields & MaplibreNativeC.MLN_BOUND_OPTION_MAX_PITCH) != 0
        },
        new double[] {
          nativeBounds.southwest().latitude(),
          nativeBounds.southwest().longitude(),
          nativeBounds.northeast().latitude(),
          nativeBounds.northeast().longitude(),
          outBounds.min_zoom(),
          outBounds.max_zoom(),
          outBounds.min_pitch(),
          outBounds.max_pitch()
        });
  }

  public void setBounds(BoundOptions options) {
    NativeLibrary.ensureLoaded();
    var nativeBounds = nativeBounds(boundsToNative(options));
    Status.check(
        MaplibreNativeC.mln_map_set_bounds(
            JavaCppSupport.map(state.requireLiveAddress()), nativeBounds));
  }

  public FreeCameraOptions freeCameraOptions() {
    NativeLibrary.ensureLoaded();
    var outFreeCamera = MaplibreNativeC.mln_free_camera_options_default();
    Status.check(
        MaplibreNativeC.mln_map_get_free_camera_options(
            JavaCppSupport.map(state.requireLiveAddress()), outFreeCamera));
    var fields = outFreeCamera.fields();
    return freeCameraFromNative(
        new boolean[] {
          (fields & MaplibreNativeC.MLN_FREE_CAMERA_OPTION_POSITION) != 0,
          (fields & MaplibreNativeC.MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0
        },
        new double[] {
          outFreeCamera._position().x(),
          outFreeCamera._position().y(),
          outFreeCamera._position().z(),
          outFreeCamera.orientation().x(),
          outFreeCamera.orientation().y(),
          outFreeCamera.orientation().z(),
          outFreeCamera.orientation().w()
        });
  }

  public void setFreeCameraOptions(FreeCameraOptions options) {
    NativeLibrary.ensureLoaded();
    var nativeFreeCamera = nativeFreeCamera(freeCameraToNative(options));
    Status.check(
        MaplibreNativeC.mln_map_set_free_camera_options(
            JavaCppSupport.map(state.requireLiveAddress()), nativeFreeCamera));
  }

  public ProjectionModeOptions projectionMode() {
    NativeLibrary.ensureLoaded();
    var outMode = MaplibreNativeC.mln_projection_mode_default();
    Status.check(
        MaplibreNativeC.mln_map_get_projection_mode(
            JavaCppSupport.map(state.requireLiveAddress()), outMode));
    var fields = outMode.fields();
    return projectionModeFromNative(
        new boolean[] {
          (fields & MaplibreNativeC.MLN_PROJECTION_MODE_AXONOMETRIC) != 0,
          (fields & MaplibreNativeC.MLN_PROJECTION_MODE_X_SKEW) != 0,
          (fields & MaplibreNativeC.MLN_PROJECTION_MODE_Y_SKEW) != 0
        },
        new boolean[] {outMode.axonometric()},
        new double[] {outMode.x_skew(), outMode.y_skew()});
  }

  public void setProjectionMode(ProjectionModeOptions mode) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(mode, "mode");
    var fields = new boolean[PROJECTION_MODE_FIELD_COUNT];
    var booleans = new boolean[PROJECTION_MODE_BOOLEAN_COUNT];
    var values = new double[PROJECTION_MODE_VALUE_COUNT];
    fields[0] = mode.hasAxonometric();
    booleans[0] = mode.hasAxonometric() && mode.axonometric();
    fields[1] = mode.hasXSkew();
    values[0] = mode.hasXSkew() ? mode.xSkew() : 0;
    fields[2] = mode.hasYSkew();
    values[1] = mode.hasYSkew() ? mode.ySkew() : 0;
    var nativeMode = MaplibreNativeC.mln_projection_mode_default();
    var nativeFields = 0;
    if (fields[0]) {
      nativeFields |= MaplibreNativeC.MLN_PROJECTION_MODE_AXONOMETRIC;
      nativeMode.axonometric(booleans[0]);
    }
    if (fields[1]) {
      nativeFields |= MaplibreNativeC.MLN_PROJECTION_MODE_X_SKEW;
      nativeMode.x_skew(values[0]);
    }
    if (fields[2]) {
      nativeFields |= MaplibreNativeC.MLN_PROJECTION_MODE_Y_SKEW;
      nativeMode.y_skew(values[1]);
    }
    nativeMode.fields(nativeFields);
    Status.check(
        MaplibreNativeC.mln_map_set_projection_mode(
            JavaCppSupport.map(state.requireLiveAddress()), nativeMode));
  }

  public ScreenPoint pixelForLatLng(LatLng coordinate) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(coordinate, "coordinate");
    var outPoint = new MaplibreNativeC.mln_screen_point();
    Status.check(
        MaplibreNativeC.mln_map_pixel_for_lat_lng(
            JavaCppSupport.map(state.requireLiveAddress()),
            new MaplibreNativeC.mln_lat_lng()
                .latitude(coordinate.latitude())
                .longitude(coordinate.longitude()),
            outPoint));
    return new ScreenPoint(outPoint.x(), outPoint.y());
  }

  public LatLng latLngForPixel(ScreenPoint point) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(point, "point");
    var outCoordinate = new MaplibreNativeC.mln_lat_lng();
    Status.check(
        MaplibreNativeC.mln_map_lat_lng_for_pixel(
            JavaCppSupport.map(state.requireLiveAddress()),
            new MaplibreNativeC.mln_screen_point().x(point.x()).y(point.y()),
            outCoordinate));
    return new LatLng(outCoordinate.latitude(), outCoordinate.longitude());
  }

  public List<ScreenPoint> pixelsForLatLngs(List<LatLng> coordinates) {
    NativeLibrary.ensureLoaded();
    var copiedCoordinates = List.copyOf(Objects.requireNonNull(coordinates, "coordinates"));
    try (var nativeCoordinates =
            org.maplibre.nativejni.internal.struct.CoreStructs.latLngArray(copiedCoordinates);
        var outPoints =
            copiedCoordinates.isEmpty()
                ? null
                : new MaplibreNativeC.mln_screen_point(copiedCoordinates.size())) {
      Status.check(
          MaplibreNativeC.mln_map_pixels_for_lat_lngs(
              JavaCppSupport.map(state.requireLiveAddress()),
              nativeCoordinates.coordinates(),
              nativeCoordinates.count(),
              outPoints));
      var points = new java.util.ArrayList<ScreenPoint>(copiedCoordinates.size());
      for (var index = 0; index < copiedCoordinates.size(); index++) {
        var point = outPoints.getPointer(index);
        points.add(new ScreenPoint(point.x(), point.y()));
      }
      return List.copyOf(points);
    }
  }

  public List<LatLng> latLngsForPixels(List<ScreenPoint> points) {
    NativeLibrary.ensureLoaded();
    var copiedPoints = List.copyOf(Objects.requireNonNull(points, "points"));
    try (var nativePoints =
            copiedPoints.isEmpty()
                ? null
                : new MaplibreNativeC.mln_screen_point(copiedPoints.size());
        var outCoordinates =
            copiedPoints.isEmpty() ? null : new MaplibreNativeC.mln_lat_lng(copiedPoints.size())) {
      for (var index = 0; index < copiedPoints.size(); index++) {
        var point = Objects.requireNonNull(copiedPoints.get(index), "point");
        nativePoints.position(index).x(point.x()).y(point.y());
      }
      if (nativePoints != null) {
        nativePoints.position(0);
      }
      Status.check(
          MaplibreNativeC.mln_map_lat_lngs_for_pixels(
              JavaCppSupport.map(state.requireLiveAddress()),
              nativePoints,
              copiedPoints.size(),
              outCoordinates));
      var coordinates = new java.util.ArrayList<LatLng>(copiedPoints.size());
      for (var index = 0; index < copiedPoints.size(); index++) {
        var coordinate = outCoordinates.getPointer(index);
        coordinates.add(new LatLng(coordinate.latitude(), coordinate.longitude()));
      }
      return List.copyOf(coordinates);
    }
  }

  static NativeOptions cameraToNative(CameraOptions camera) {
    var cameraValue = MapStructs.cameraOptions(camera);
    var fields = new boolean[CAMERA_FIELD_COUNT];
    var values = new double[CAMERA_VALUE_COUNT];
    fields[0] = cameraValue.hasCenter();
    if (fields[0]) {
      values[0] = cameraValue.center().latitude();
      values[1] = cameraValue.center().longitude();
    }
    fields[1] = cameraValue.hasCenterAltitude();
    values[2] = cameraValue.centerAltitude();
    fields[2] = cameraValue.hasPadding();
    if (fields[2]) {
      values[3] = cameraValue.padding().top();
      values[4] = cameraValue.padding().left();
      values[5] = cameraValue.padding().bottom();
      values[6] = cameraValue.padding().right();
    }
    fields[3] = cameraValue.hasAnchor();
    if (fields[3]) {
      values[7] = cameraValue.anchor().x();
      values[8] = cameraValue.anchor().y();
    }
    fields[4] = cameraValue.hasZoom();
    values[9] = cameraValue.zoom();
    fields[5] = cameraValue.hasBearing();
    values[10] = cameraValue.bearing();
    fields[6] = cameraValue.hasPitch();
    values[11] = cameraValue.pitch();
    fields[7] = cameraValue.hasRoll();
    values[12] = cameraValue.roll();
    fields[8] = cameraValue.hasFieldOfView();
    values[13] = cameraValue.fieldOfView();
    return new NativeOptions(fields, values);
  }

  static CameraOptions cameraFromNative(boolean[] fields, double[] values) {
    var camera = new CameraOptions();
    if (fields[0]) {
      camera.center(values[0], values[1]);
    }
    if (fields[1]) {
      camera.centerAltitude(values[2]);
    }
    if (fields[2]) {
      camera.padding(new EdgeInsets(values[3], values[4], values[5], values[6]));
    }
    if (fields[3]) {
      camera.anchor(new ScreenPoint(values[7], values[8]));
    }
    if (fields[4]) {
      camera.zoom(values[9]);
    }
    if (fields[5]) {
      camera.bearing(values[10]);
    }
    if (fields[6]) {
      camera.pitch(values[11]);
    }
    if (fields[7]) {
      camera.roll(values[12]);
    }
    if (fields[8]) {
      camera.fieldOfView(values[13]);
    }
    return camera;
  }

  private static NativeOptions fitToNative(CameraFitOptions options, boolean hasOptions) {
    var fields = new boolean[FIT_FIELD_COUNT];
    var values = new double[FIT_VALUE_COUNT];
    if (hasOptions) {
      var fitOptions = MapStructs.cameraFitOptions(options);
      fields[0] = fitOptions.hasPadding();
      if (fields[0]) {
        values[0] = fitOptions.padding().top();
        values[1] = fitOptions.padding().left();
        values[2] = fitOptions.padding().bottom();
        values[3] = fitOptions.padding().right();
      }
      fields[1] = fitOptions.hasBearing();
      values[4] = fitOptions.bearing();
      fields[2] = fitOptions.hasPitch();
      values[5] = fitOptions.pitch();
    }
    return new NativeOptions(fields, values);
  }

  private static NativeOptions boundsToNative(BoundOptions options) {
    var boundsValue = MapStructs.boundOptions(options);
    var fields = new boolean[BOUND_FIELD_COUNT];
    var values = new double[BOUND_VALUE_COUNT];
    fields[0] = boundsValue.hasBounds();
    if (fields[0]) {
      values[0] = boundsValue.bounds().southwest().latitude();
      values[1] = boundsValue.bounds().southwest().longitude();
      values[2] = boundsValue.bounds().northeast().latitude();
      values[3] = boundsValue.bounds().northeast().longitude();
    }
    fields[1] = boundsValue.hasMinZoom();
    values[4] = boundsValue.minZoom();
    fields[2] = boundsValue.hasMaxZoom();
    values[5] = boundsValue.maxZoom();
    fields[3] = boundsValue.hasMinPitch();
    values[6] = boundsValue.minPitch();
    fields[4] = boundsValue.hasMaxPitch();
    values[7] = boundsValue.maxPitch();
    return new NativeOptions(fields, values);
  }

  private static BoundOptions boundsFromNative(boolean[] fields, double[] values) {
    var options = new BoundOptions();
    if (fields[0]) {
      options.bounds(
          new LatLngBounds(new LatLng(values[0], values[1]), new LatLng(values[2], values[3])));
    }
    if (fields[1]) {
      options.minZoom(values[4]);
    }
    if (fields[2]) {
      options.maxZoom(values[5]);
    }
    if (fields[3]) {
      options.minPitch(values[6]);
    }
    if (fields[4]) {
      options.maxPitch(values[7]);
    }
    return options;
  }

  private static NativeOptions freeCameraToNative(FreeCameraOptions options) {
    var freeCamera = MapStructs.freeCameraOptions(options);
    var fields = new boolean[FREE_CAMERA_FIELD_COUNT];
    var values = new double[FREE_CAMERA_VALUE_COUNT];
    fields[0] = freeCamera.hasPosition();
    if (fields[0]) {
      values[0] = freeCamera.position().x();
      values[1] = freeCamera.position().y();
      values[2] = freeCamera.position().z();
    }
    fields[1] = freeCamera.hasOrientation();
    if (fields[1]) {
      values[3] = freeCamera.orientation().x();
      values[4] = freeCamera.orientation().y();
      values[5] = freeCamera.orientation().z();
      values[6] = freeCamera.orientation().w();
    }
    return new NativeOptions(fields, values);
  }

  private static FreeCameraOptions freeCameraFromNative(boolean[] fields, double[] values) {
    var options = new FreeCameraOptions();
    if (fields[0]) {
      options.position(new Vec3(values[0], values[1], values[2]));
    }
    if (fields[1]) {
      options.orientation(new Quaternion(values[3], values[4], values[5], values[6]));
    }
    return options;
  }

  private static ProjectionModeOptions projectionModeFromNative(
      boolean[] fields, boolean[] booleans, double[] values) {
    var mode = new ProjectionModeOptions();
    if (fields[0]) {
      mode.axonometric(booleans[0]);
    }
    if (fields[1]) {
      mode.xSkew(values[0]);
    }
    if (fields[2]) {
      mode.ySkew(values[1]);
    }
    return mode;
  }

  private static NativeOptions animationToNative(AnimationOptions animation, boolean hasAnimation) {
    var fields = new boolean[ANIMATION_FIELD_COUNT];
    var values = new double[ANIMATION_VALUE_COUNT];
    if (hasAnimation) {
      var animationValue = MapStructs.animationOptions(animation);
      fields[0] = animationValue.hasDurationMs();
      values[0] = animationValue.durationMs();
      fields[1] = animationValue.hasVelocity();
      values[1] = animationValue.velocity();
      fields[2] = animationValue.hasMinZoom();
      values[2] = animationValue.minZoom();
      fields[3] = animationValue.hasEasing();
      if (fields[3]) {
        values[3] = animationValue.easing().x1();
        values[4] = animationValue.easing().y1();
        values[5] = animationValue.easing().x2();
        values[6] = animationValue.easing().y2();
      }
    }
    return new NativeOptions(fields, values);
  }

  private static MaplibreNativeC.mln_animation_options nativeAnimation(
      NativeOptions animation, boolean hasAnimation) {
    if (!hasAnimation) {
      return null;
    }
    var out = MaplibreNativeC.mln_animation_options_default();
    var fields = 0;
    if (animation.fields()[0]) {
      fields |= MaplibreNativeC.MLN_ANIMATION_OPTION_DURATION;
      out.duration_ms(animation.values()[0]);
    }
    if (animation.fields()[1]) {
      fields |= MaplibreNativeC.MLN_ANIMATION_OPTION_VELOCITY;
      out.velocity(animation.values()[1]);
    }
    if (animation.fields()[2]) {
      fields |= MaplibreNativeC.MLN_ANIMATION_OPTION_MIN_ZOOM;
      out.min_zoom(animation.values()[2]);
    }
    if (animation.fields()[3]) {
      fields |= MaplibreNativeC.MLN_ANIMATION_OPTION_EASING;
      out.easing(
          new MaplibreNativeC.mln_unit_bezier()
              .x1(animation.values()[3])
              .y1(animation.values()[4])
              .x2(animation.values()[5])
              .y2(animation.values()[6]));
    }
    out.fields(fields);
    return out;
  }

  private static MaplibreNativeC.mln_camera_fit_options nativeFit(
      NativeOptions fit, boolean hasFit) {
    if (!hasFit) {
      return null;
    }
    var out = MaplibreNativeC.mln_camera_fit_options_default();
    var fields = 0;
    if (fit.fields()[0]) {
      fields |= MaplibreNativeC.MLN_CAMERA_FIT_OPTION_PADDING;
      out.padding(
          new MaplibreNativeC.mln_edge_insets()
              .top(fit.values()[0])
              .left(fit.values()[1])
              .bottom(fit.values()[2])
              .right(fit.values()[3]));
    }
    if (fit.fields()[1]) {
      fields |= MaplibreNativeC.MLN_CAMERA_FIT_OPTION_BEARING;
      out.bearing(fit.values()[4]);
    }
    if (fit.fields()[2]) {
      fields |= MaplibreNativeC.MLN_CAMERA_FIT_OPTION_PITCH;
      out.pitch(fit.values()[5]);
    }
    out.fields(fields);
    return out;
  }

  private static MaplibreNativeC.mln_bound_options nativeBounds(NativeOptions bounds) {
    var out = MaplibreNativeC.mln_bound_options_default();
    var fields = 0;
    if (bounds.fields()[0]) {
      fields |= MaplibreNativeC.MLN_BOUND_OPTION_BOUNDS;
      out.bounds(
          nativeLatLngBounds(
              bounds.values()[0], bounds.values()[1], bounds.values()[2], bounds.values()[3]));
    }
    if (bounds.fields()[1]) {
      fields |= MaplibreNativeC.MLN_BOUND_OPTION_MIN_ZOOM;
      out.min_zoom(bounds.values()[4]);
    }
    if (bounds.fields()[2]) {
      fields |= MaplibreNativeC.MLN_BOUND_OPTION_MAX_ZOOM;
      out.max_zoom(bounds.values()[5]);
    }
    if (bounds.fields()[3]) {
      fields |= MaplibreNativeC.MLN_BOUND_OPTION_MIN_PITCH;
      out.min_pitch(bounds.values()[6]);
    }
    if (bounds.fields()[4]) {
      fields |= MaplibreNativeC.MLN_BOUND_OPTION_MAX_PITCH;
      out.max_pitch(bounds.values()[7]);
    }
    out.fields(fields);
    return out;
  }

  private static MaplibreNativeC.mln_lat_lng_bounds nativeLatLngBounds(
      double swLat, double swLon, double neLat, double neLon) {
    var out = new MaplibreNativeC.mln_lat_lng_bounds();
    out.southwest().latitude(swLat).longitude(swLon);
    out.northeast().latitude(neLat).longitude(neLon);
    return out;
  }

  private static LatLngBounds latLngBounds(MaplibreNativeC.mln_lat_lng_bounds bounds) {
    return new LatLngBounds(
        new LatLng(bounds.southwest().latitude(), bounds.southwest().longitude()),
        new LatLng(bounds.northeast().latitude(), bounds.northeast().longitude()));
  }

  private static MaplibreNativeC.mln_free_camera_options nativeFreeCamera(
      NativeOptions freeCamera) {
    var out = MaplibreNativeC.mln_free_camera_options_default();
    var fields = 0;
    if (freeCamera.fields()[0]) {
      fields |= MaplibreNativeC.MLN_FREE_CAMERA_OPTION_POSITION;
      out._position(
          new MaplibreNativeC.mln_vec3()
              .x(freeCamera.values()[0])
              .y(freeCamera.values()[1])
              .z(freeCamera.values()[2]));
    }
    if (freeCamera.fields()[1]) {
      fields |= MaplibreNativeC.MLN_FREE_CAMERA_OPTION_ORIENTATION;
      out.orientation(
          new MaplibreNativeC.mln_quaternion()
              .x(freeCamera.values()[3])
              .y(freeCamera.values()[4])
              .z(freeCamera.values()[5])
              .w(freeCamera.values()[6]));
    }
    out.fields(fields);
    return out;
  }

  private static String[] stringArray(List<String> values, String name) {
    Objects.requireNonNull(values, name);
    return values.stream()
        .map(value -> Objects.requireNonNull(value, name + " element"))
        .toArray(String[]::new);
  }

  private static double[] coordinateArray(List<LatLng> coordinates, String name) {
    Objects.requireNonNull(coordinates, name);
    var values = new double[coordinates.size() * 2];
    for (var index = 0; index < coordinates.size(); index++) {
      var coordinate = Objects.requireNonNull(coordinates.get(index), name + " element");
      values[index * 2] = coordinate.latitude();
      values[index * 2 + 1] = coordinate.longitude();
    }
    return values;
  }

  private static void checkCanonicalTile(int z, long x, long y) {
    if (z < 0 || x < 0 || y < 0 || x > 0xffff_ffffL || y > 0xffff_ffffL) {
      JavaCppSupport.setThreadDiagnostic("canonical tile z, x, and y must fit uint32");
      Status.check(MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT);
    }
  }

  private static void checkTileSourceOptions(NativeTileSourceOptions options) {
    if (options.fields().length > 5 && options.fields()[5] && options.values()[7] < 0) {
      JavaCppSupport.setThreadDiagnostic("tile size must be non-negative");
      Status.check(MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT);
    }
  }

  private static NativeTileSourceOptions tileSourceOptions(TileSourceOptions options) {
    var fields = new boolean[8];
    var values = new double[10];
    var attribution = "";
    if (options != null) {
      fields[0] = options.hasMinZoom();
      values[0] = fields[0] ? options.minZoom() : 0.0;
      fields[1] = options.hasMaxZoom();
      values[1] = fields[1] ? options.maxZoom() : 0.0;
      fields[2] = options.hasAttribution();
      attribution = fields[2] ? options.attribution() : "";
      fields[3] = options.hasScheme();
      values[6] = fields[3] ? options.scheme().nativeValue() : 0.0;
      fields[4] = options.hasBounds();
      if (fields[4]) {
        values[2] = options.bounds().southwest().latitude();
        values[3] = options.bounds().southwest().longitude();
        values[4] = options.bounds().northeast().latitude();
        values[5] = options.bounds().northeast().longitude();
      }
      fields[5] = options.hasTileSize();
      values[7] = fields[5] ? options.tileSize() : 0.0;
      fields[6] = options.hasVectorEncoding();
      values[8] = fields[6] ? options.vectorEncoding().nativeValue() : 0.0;
      fields[7] = options.hasRasterDemEncoding();
      values[9] = fields[7] ? options.rasterDemEncoding().nativeValue() : 0.0;
    }
    return new NativeTileSourceOptions(fields, values, attribution);
  }

  record NativeOptions(boolean[] fields, double[] values) {}

  record NativeTileSourceOptions(boolean[] fields, double[] values, String attribution) {}

  public MapProjectionHandle createProjection() {
    return MapProjectionHandle.create(this);
  }

  public void close() {
    state.closeOnce(
        address -> MaplibreNativeC.mln_map_destroy(JavaCppSupport.map(address)),
        () -> {
          clearCustomGeometrySources();
          runtime.unregisterMap(InternalAccess.INSTANCE, this);
        });
  }

  public boolean isClosed() {
    return state.isReleased();
  }

  public RuntimeHandle runtime() {
    return runtime;
  }

  public long nativeAddress(InternalAccess access) {
    Objects.requireNonNull(access, "access");
    return nativeAddress();
  }

  long nativeAddress() {
    return state.requireLiveAddress();
  }

  public void releaseDetachedCustomGeometrySources(InternalAccess access) {
    Objects.requireNonNull(access, "access");
    releaseDetachedCustomGeometrySources();
  }

  void releaseDetachedCustomGeometrySources() {
    var iterator = customGeometrySources.entrySet().iterator();
    while (iterator.hasNext()) {
      var entry = iterator.next();
      var type = styleSourceType(entry.getKey());
      if (type.isEmpty() || type.orElseThrow() != SourceType.CUSTOM_VECTOR) {
        closeQuietly(entry.getValue());
        iterator.remove();
      }
    }
  }

  int customGeometrySourceCountForTesting() {
    return customGeometrySources.size();
  }

  private void closeCustomGeometrySource(String sourceId) {
    closeQuietly(customGeometrySources.remove(sourceId));
  }

  private void clearCustomGeometrySources() {
    customGeometrySources.values().forEach(MapHandle::closeQuietly);
    customGeometrySources.clear();
  }

  private static void closeQuietly(AutoCloseable closeable) {
    if (closeable == null) {
      return;
    }
    try {
      closeable.close();
    } catch (Exception ignored) {
      // Closing callback state is best-effort after native ownership ends.
    }
  }
}
