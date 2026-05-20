package org.maplibre.nativeffi.map;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import org.maplibre.nativeffi.camera.AnimationOptions;
import org.maplibre.nativeffi.camera.BoundOptions;
import org.maplibre.nativeffi.camera.CameraFitOptions;
import org.maplibre.nativeffi.camera.CameraOptions;
import org.maplibre.nativeffi.camera.FreeCameraOptions;
import org.maplibre.nativeffi.geo.CanonicalTileId;
import org.maplibre.nativeffi.geo.GeoJson;
import org.maplibre.nativeffi.geo.Geometry;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.geo.LatLngBounds;
import org.maplibre.nativeffi.geo.ScreenPoint;
import org.maplibre.nativeffi.internal.access.InternalAccess;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_lat_lng;
import org.maplibre.nativeffi.internal.c.mln_lat_lng_bounds;
import org.maplibre.nativeffi.internal.c.mln_screen_point;
import org.maplibre.nativeffi.internal.c.mln_style_source_info;
import org.maplibre.nativeffi.internal.lifecycle.HandleState;
import org.maplibre.nativeffi.internal.loader.NativeAccess;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.internal.status.Status;
import org.maplibre.nativeffi.internal.struct.CoreStructs;
import org.maplibre.nativeffi.internal.struct.MapStructs;
import org.maplibre.nativeffi.internal.struct.StyleStructs;
import org.maplibre.nativeffi.internal.struct.ValueStructs;
import org.maplibre.nativeffi.json.JsonValue;
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor;
import org.maplibre.nativeffi.render.PremultipliedRgba8Image;
import org.maplibre.nativeffi.render.RenderSessionHandle;
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor;
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor;
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor;
import org.maplibre.nativeffi.runtime.RuntimeHandle;
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions;
import org.maplibre.nativeffi.style.LocationIndicatorImageKind;
import org.maplibre.nativeffi.style.SourceInfo;
import org.maplibre.nativeffi.style.SourceType;
import org.maplibre.nativeffi.style.StyleImage;
import org.maplibre.nativeffi.style.StyleImageInfo;
import org.maplibre.nativeffi.style.StyleImageOptions;
import org.maplibre.nativeffi.style.TileSourceOptions;

/** Owned native map handle. Close it on the map owner thread. */
public final class MapHandle implements AutoCloseable {
  private final RuntimeHandle runtime;
  private final HandleState state;
  private final Map<String, CustomGeometrySourceState> customGeometrySources = new HashMap<>();

  private MapHandle(RuntimeHandle runtime, MemorySegment handle) {
    this.runtime = runtime;
    this.state = new HandleState("MapHandle", handle, runtime);
  }

  public static MapHandle create(RuntimeHandle runtime, MapOptions options) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(runtime, "runtime");
    Objects.requireNonNull(options, "options");
    try (var arena = Arena.ofConfined()) {
      var outMap = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_map_create(
              runtime.nativeHandle(InternalAccess.INSTANCE),
              MapStructs.mapOptions(options, arena),
              outMap));
      var map = new MapHandle(runtime, outMap.get(ValueLayout.ADDRESS, 0));
      runtime.registerMap(InternalAccess.INSTANCE, map);
      return map;
    }
  }

  public void setStyleUrl(String url) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_style_url(
              state.requireLive(), MemoryUtil.allocateCString(arena, Objects.requireNonNull(url))));
    }
  }

  public void setStyleJson(String json) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_style_json(
              state.requireLive(),
              MemoryUtil.allocateCString(arena, Objects.requireNonNull(json))));
      clearCustomGeometrySources();
    }
  }

  public void addStyleSourceJson(String sourceId, JsonValue sourceJson) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(sourceJson, "sourceJson");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_style_source_json(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              ValueStructs.jsonValue(sourceJson, arena)));
    }
  }

  public boolean removeStyleSource(String sourceId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outRemoved = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_remove_style_source(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              outRemoved));
      var removed = outRemoved.get(ValueLayout.JAVA_BOOLEAN, 0);
      if (removed) {
        closeCustomGeometrySource(sourceId);
      }
      return removed;
    }
  }

  public boolean styleSourceExists(String sourceId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outExists = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_style_source_exists(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              outExists));
      return outExists.get(ValueLayout.JAVA_BOOLEAN, 0);
    }
  }

  public Optional<SourceType> styleSourceType(String sourceId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outType = arena.allocate(ValueLayout.JAVA_INT);
      var outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_get_style_source_type(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              outType,
              outFound));
      return outFound.get(ValueLayout.JAVA_BOOLEAN, 0)
          ? Optional.of(SourceType.fromNative(outType.get(ValueLayout.JAVA_INT, 0)))
          : Optional.empty();
    }
  }

  public Optional<SourceInfo> styleSourceInfo(String sourceId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var sourceIdView =
          CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena);
      var outInfo = mln_style_source_info.allocate(arena);
      mln_style_source_info.size(outInfo, (int) mln_style_source_info.sizeof());
      var outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_get_style_source_info(
              state.requireLive(), sourceIdView, outInfo, outFound));
      if (!outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        return Optional.empty();
      }
      var attribution = Optional.<String>empty();
      if (mln_style_source_info.has_attribution(outInfo)) {
        var attributionSize = Math.toIntExact(mln_style_source_info.attribution_size(outInfo));
        if (attributionSize == 0) {
          attribution = Optional.of("");
        } else {
          var outAttribution = arena.allocate(attributionSize);
          var outAttributionSize = arena.allocate(ValueLayout.JAVA_LONG);
          var outAttributionFound = arena.allocate(ValueLayout.JAVA_BOOLEAN);
          Status.check(
              MapLibreNativeC.mln_map_copy_style_source_attribution(
                  state.requireLive(),
                  sourceIdView,
                  outAttribution,
                  attributionSize,
                  outAttributionSize,
                  outAttributionFound));
          if (!outAttributionFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
            return Optional.empty();
          }
          attribution =
              Optional.of(
                  MemoryUtil.copyStringView(
                      outAttribution, outAttributionSize.get(ValueLayout.JAVA_LONG, 0)));
        }
      }
      return Optional.of(StyleStructs.sourceInfo(outInfo, attribution));
    }
  }

  public List<String> styleSourceIds() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outList = MemoryUtil.allocatePointer(arena);
      Status.check(MapLibreNativeC.mln_map_list_style_source_ids(state.requireLive(), outList));
      return StyleStructs.styleIdList(outList.get(ValueLayout.ADDRESS, 0));
    }
  }

  public void addGeoJsonSourceUrl(String sourceId, String url) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_geojson_source_url(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              CoreStructs.stringView(Objects.requireNonNull(url, "url"), arena)));
    }
  }

  public void addGeoJsonSourceData(String sourceId, GeoJson data) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(data, "data");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_geojson_source_data(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              ValueStructs.geoJson(data, arena)));
    }
  }

  public void setGeoJsonSourceUrl(String sourceId, String url) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_geojson_source_url(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              CoreStructs.stringView(Objects.requireNonNull(url, "url"), arena)));
    }
  }

  public void setGeoJsonSourceData(String sourceId, GeoJson data) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(data, "data");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_geojson_source_data(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              ValueStructs.geoJson(data, arena)));
    }
  }

  public void addCustomGeometrySource(String sourceId, CustomGeometrySourceOptions options) {
    NativeAccess.ensureLoaded();
    var copiedSourceId = Objects.requireNonNull(sourceId, "sourceId");
    var state = new CustomGeometrySourceState(Objects.requireNonNull(options, "options"));
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_custom_geometry_source(
              this.state.requireLive(),
              CoreStructs.stringView(copiedSourceId, arena),
              state.descriptor()));
      closeQuietly(customGeometrySources.put(copiedSourceId, state));
    } catch (RuntimeException | Error error) {
      closeQuietly(state);
      throw error;
    }
  }

  public void setCustomGeometrySourceTileData(
      String sourceId, CanonicalTileId tileId, GeoJson data) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(tileId, "tileId");
    Objects.requireNonNull(data, "data");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_custom_geometry_source_tile_data(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              StyleStructs.canonicalTileId(tileId, arena),
              ValueStructs.geoJson(data, arena)));
    }
  }

  public void invalidateCustomGeometrySourceTile(String sourceId, CanonicalTileId tileId) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(tileId, "tileId");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_invalidate_custom_geometry_source_tile(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              StyleStructs.canonicalTileId(tileId, arena)));
    }
  }

  public void invalidateCustomGeometrySourceRegion(String sourceId, LatLngBounds bounds) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(bounds, "bounds");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_invalidate_custom_geometry_source_region(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              CoreStructs.latLngBounds(bounds, arena)));
    }
  }

  public void addVectorSourceUrl(String sourceId, String url) {
    addVectorSourceUrlInternal(sourceId, url, null, false);
  }

  public void addVectorSourceUrl(String sourceId, String url, TileSourceOptions options) {
    addVectorSourceUrlInternal(sourceId, url, Objects.requireNonNull(options, "options"), true);
  }

  public void addVectorSourceTiles(String sourceId, List<String> tiles) {
    addVectorSourceTilesInternal(sourceId, tiles, null, false);
  }

  public void addVectorSourceTiles(String sourceId, List<String> tiles, TileSourceOptions options) {
    addVectorSourceTilesInternal(sourceId, tiles, Objects.requireNonNull(options, "options"), true);
  }

  public void addRasterSourceUrl(String sourceId, String url) {
    addRasterSourceUrlInternal(sourceId, url, null, false);
  }

  public void addRasterSourceUrl(String sourceId, String url, TileSourceOptions options) {
    addRasterSourceUrlInternal(sourceId, url, Objects.requireNonNull(options, "options"), true);
  }

  public void addRasterSourceTiles(String sourceId, List<String> tiles) {
    addRasterSourceTilesInternal(sourceId, tiles, null, false);
  }

  public void addRasterSourceTiles(String sourceId, List<String> tiles, TileSourceOptions options) {
    addRasterSourceTilesInternal(sourceId, tiles, Objects.requireNonNull(options, "options"), true);
  }

  public void addRasterDemSourceUrl(String sourceId, String url) {
    addRasterDemSourceUrlInternal(sourceId, url, null, false);
  }

  public void addRasterDemSourceUrl(String sourceId, String url, TileSourceOptions options) {
    addRasterDemSourceUrlInternal(sourceId, url, Objects.requireNonNull(options, "options"), true);
  }

  public void addRasterDemSourceTiles(String sourceId, List<String> tiles) {
    addRasterDemSourceTilesInternal(sourceId, tiles, null, false);
  }

  public void addRasterDemSourceTiles(
      String sourceId, List<String> tiles, TileSourceOptions options) {
    addRasterDemSourceTilesInternal(
        sourceId, tiles, Objects.requireNonNull(options, "options"), true);
  }

  public void setStyleImage(String imageId, PremultipliedRgba8Image image) {
    setStyleImageInternal(imageId, image, null, false);
  }

  public void setStyleImage(
      String imageId, PremultipliedRgba8Image image, StyleImageOptions options) {
    setStyleImageInternal(imageId, image, Objects.requireNonNull(options, "options"), true);
  }

  public boolean removeStyleImage(String imageId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outRemoved = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_remove_style_image(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(imageId, "imageId"), arena),
              outRemoved));
      return outRemoved.get(ValueLayout.JAVA_BOOLEAN, 0);
    }
  }

  public boolean styleImageExists(String imageId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outExists = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_style_image_exists(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(imageId, "imageId"), arena),
              outExists));
      return outExists.get(ValueLayout.JAVA_BOOLEAN, 0);
    }
  }

  public Optional<StyleImageInfo> styleImageInfo(String imageId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outInfo = MapLibreNativeC.mln_style_image_info_default(arena);
      var outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_get_style_image_info(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(imageId, "imageId"), arena),
              outInfo,
              outFound));
      return outFound.get(ValueLayout.JAVA_BOOLEAN, 0)
          ? Optional.of(StyleStructs.styleImageInfo(outInfo))
          : Optional.empty();
    }
  }

  public Optional<StyleImage> copyStyleImagePremultipliedRgba8(String imageId) {
    var info = styleImageInfo(imageId);
    if (info.isEmpty()) {
      return Optional.empty();
    }
    var imageInfo = info.get();
    try (var arena = Arena.ofConfined()) {
      var byteLength = Math.toIntExact(imageInfo.byteLength());
      var outPixels = byteLength == 0 ? MemorySegment.NULL : arena.allocate(byteLength);
      var outByteLength = arena.allocate(ValueLayout.JAVA_LONG);
      var outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_copy_style_image_premultiplied_rgba8(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(imageId, "imageId"), arena),
              outPixels,
              byteLength,
              outByteLength,
              outFound));
      if (!outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        return Optional.empty();
      }
      var pixels = byteLength == 0 ? new byte[0] : outPixels.toArray(ValueLayout.JAVA_BYTE);
      return Optional.of(
          new StyleImage(
              new PremultipliedRgba8Image(
                  imageInfo.width(), imageInfo.height(), imageInfo.stride(), pixels),
              imageInfo.pixelRatio(),
              imageInfo.sdf()));
    }
  }

  public void addImageSourceUrl(String sourceId, List<LatLng> coordinates, String url) {
    NativeAccess.ensureLoaded();
    var copiedCoordinates = List.copyOf(Objects.requireNonNull(coordinates, "coordinates"));
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_image_source_url(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              StyleStructs.imageSourceCoordinates(copiedCoordinates, arena),
              copiedCoordinates.size(),
              CoreStructs.stringView(Objects.requireNonNull(url, "url"), arena)));
    }
  }

  public void addImageSourceImage(
      String sourceId, List<LatLng> coordinates, PremultipliedRgba8Image image) {
    NativeAccess.ensureLoaded();
    var copiedCoordinates = List.copyOf(Objects.requireNonNull(coordinates, "coordinates"));
    Objects.requireNonNull(image, "image");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_image_source_image(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              StyleStructs.imageSourceCoordinates(copiedCoordinates, arena),
              copiedCoordinates.size(),
              StyleStructs.premultipliedRgba8Image(image, arena)));
    }
  }

  public void setImageSourceUrl(String sourceId, String url) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_image_source_url(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              CoreStructs.stringView(Objects.requireNonNull(url, "url"), arena)));
    }
  }

  public void setImageSourceImage(String sourceId, PremultipliedRgba8Image image) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(image, "image");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_image_source_image(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              StyleStructs.premultipliedRgba8Image(image, arena)));
    }
  }

  public void setImageSourceCoordinates(String sourceId, List<LatLng> coordinates) {
    NativeAccess.ensureLoaded();
    var copiedCoordinates = List.copyOf(Objects.requireNonNull(coordinates, "coordinates"));
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_image_source_coordinates(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              StyleStructs.imageSourceCoordinates(copiedCoordinates, arena),
              copiedCoordinates.size()));
    }
  }

  public Optional<List<LatLng>> imageSourceCoordinates(String sourceId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outCoordinates = mln_lat_lng.allocateArray(4, arena);
      var outCoordinateCount = arena.allocate(ValueLayout.JAVA_LONG);
      var outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_get_image_source_coordinates(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              outCoordinates,
              4,
              outCoordinateCount,
              outFound));
      return outFound.get(ValueLayout.JAVA_BOOLEAN, 0)
          ? Optional.of(
              CoreStructs.latLngArray(
                  outCoordinates,
                  Math.toIntExact(outCoordinateCount.get(ValueLayout.JAVA_LONG, 0))))
          : Optional.empty();
    }
  }

  public void addStyleLayerJson(JsonValue layerJson) {
    addStyleLayerJson(layerJson, "");
  }

  public void addStyleLayerJson(JsonValue layerJson, String beforeLayerId) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(layerJson, "layerJson");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_style_layer_json(
              state.requireLive(),
              ValueStructs.jsonValue(layerJson, arena),
              CoreStructs.stringView(
                  Objects.requireNonNull(beforeLayerId, "beforeLayerId"), arena)));
    }
  }

  public void addHillshadeLayer(String layerId, String sourceId) {
    addHillshadeLayer(layerId, sourceId, "");
  }

  public void addHillshadeLayer(String layerId, String sourceId, String beforeLayerId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_hillshade_layer(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              CoreStructs.stringView(
                  Objects.requireNonNull(beforeLayerId, "beforeLayerId"), arena)));
    }
  }

  public void addColorReliefLayer(String layerId, String sourceId) {
    addColorReliefLayer(layerId, sourceId, "");
  }

  public void addColorReliefLayer(String layerId, String sourceId, String beforeLayerId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_color_relief_layer(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              CoreStructs.stringView(
                  Objects.requireNonNull(beforeLayerId, "beforeLayerId"), arena)));
    }
  }

  public void addLocationIndicatorLayer(String layerId) {
    addLocationIndicatorLayer(layerId, "");
  }

  public void addLocationIndicatorLayer(String layerId, String beforeLayerId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_location_indicator_layer(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              CoreStructs.stringView(
                  Objects.requireNonNull(beforeLayerId, "beforeLayerId"), arena)));
    }
  }

  public void setLocationIndicatorLocation(String layerId, LatLng coordinate, double altitude) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(coordinate, "coordinate");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_location_indicator_location(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              CoreStructs.latLng(coordinate, arena),
              altitude));
    }
  }

  public void setLocationIndicatorBearing(String layerId, double bearing) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_location_indicator_bearing(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              bearing));
    }
  }

  public void setLocationIndicatorAccuracyRadius(String layerId, double radius) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_location_indicator_accuracy_radius(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              radius));
    }
  }

  public void setLocationIndicatorImageName(
      String layerId, LocationIndicatorImageKind imageKind, String imageId) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(imageKind, "imageKind");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_location_indicator_image_name(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              imageKind.nativeValue(),
              CoreStructs.stringView(Objects.requireNonNull(imageId, "imageId"), arena)));
    }
  }

  public boolean removeStyleLayer(String layerId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outRemoved = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_remove_style_layer(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              outRemoved));
      return outRemoved.get(ValueLayout.JAVA_BOOLEAN, 0);
    }
  }

  public boolean styleLayerExists(String layerId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outExists = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_style_layer_exists(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              outExists));
      return outExists.get(ValueLayout.JAVA_BOOLEAN, 0);
    }
  }

  public Optional<String> styleLayerType(String layerId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outType = org.maplibre.nativeffi.internal.c.mln_string_view.allocate(arena);
      var outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_get_style_layer_type(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              outType,
              outFound));
      return outFound.get(ValueLayout.JAVA_BOOLEAN, 0)
          ? Optional.of(CoreStructs.stringView(outType))
          : Optional.empty();
    }
  }

  public List<String> styleLayerIds() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outList = MemoryUtil.allocatePointer(arena);
      Status.check(MapLibreNativeC.mln_map_list_style_layer_ids(state.requireLive(), outList));
      return StyleStructs.styleIdList(outList.get(ValueLayout.ADDRESS, 0));
    }
  }

  public void moveStyleLayer(String layerId) {
    moveStyleLayer(layerId, "");
  }

  public void moveStyleLayer(String layerId, String beforeLayerId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_move_style_layer(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              CoreStructs.stringView(
                  Objects.requireNonNull(beforeLayerId, "beforeLayerId"), arena)));
    }
  }

  public Optional<JsonValue> styleLayerJson(String layerId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outLayer = MemoryUtil.allocatePointer(arena);
      var outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_get_style_layer_json(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              outLayer,
              outFound));
      if (!outFound.get(ValueLayout.JAVA_BOOLEAN, 0)) {
        return Optional.empty();
      }
      return ValueStructs.jsonSnapshot(outLayer.get(ValueLayout.ADDRESS, 0));
    }
  }

  public void setStyleLightJson(JsonValue lightJson) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(lightJson, "lightJson");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_style_light_json(
              state.requireLive(), ValueStructs.jsonValue(lightJson, arena)));
    }
  }

  public void setStyleLightProperty(String propertyName, JsonValue value) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(value, "value");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_style_light_property(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(propertyName, "propertyName"), arena),
              ValueStructs.jsonValue(value, arena)));
    }
  }

  public Optional<JsonValue> styleLightProperty(String propertyName) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outValue = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_map_get_style_light_property(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(propertyName, "propertyName"), arena),
              outValue));
      return ValueStructs.jsonSnapshot(outValue.get(ValueLayout.ADDRESS, 0));
    }
  }

  public void setLayerProperty(String layerId, String propertyName, JsonValue value) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(value, "value");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_layer_property(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              CoreStructs.stringView(Objects.requireNonNull(propertyName, "propertyName"), arena),
              ValueStructs.jsonValue(value, arena)));
    }
  }

  public Optional<JsonValue> layerProperty(String layerId, String propertyName) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outValue = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_map_get_layer_property(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              CoreStructs.stringView(Objects.requireNonNull(propertyName, "propertyName"), arena),
              outValue));
      return ValueStructs.jsonSnapshot(outValue.get(ValueLayout.ADDRESS, 0));
    }
  }

  public void setLayerFilter(String layerId, JsonValue filter) {
    setLayerFilterInternal(layerId, Objects.requireNonNull(filter, "filter"), true);
  }

  public void clearLayerFilter(String layerId) {
    setLayerFilterInternal(layerId, null, false);
  }

  public Optional<JsonValue> layerFilter(String layerId) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outFilter = MemoryUtil.allocatePointer(arena);
      Status.check(
          MapLibreNativeC.mln_map_get_layer_filter(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              outFilter));
      return ValueStructs.jsonSnapshot(outFilter.get(ValueLayout.ADDRESS, 0));
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

  public RenderSessionHandle attachMetalSurface(MetalSurfaceDescriptor descriptor) {
    return RenderSessionHandle.attachMetalSurface(this, descriptor);
  }

  public RenderSessionHandle attachVulkanSurface(VulkanSurfaceDescriptor descriptor) {
    return RenderSessionHandle.attachVulkanSurface(this, descriptor);
  }

  public void requestRepaint() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_map_request_repaint(state.requireLive()));
  }

  public void requestStillImage() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_map_request_still_image(state.requireLive()));
  }

  public void setDebugOptions(Set<DebugOption> options) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(options, "options");
    var mask = 0;
    for (var option : options) {
      mask |= Objects.requireNonNull(option, "option").nativeMask();
    }
    Status.check(MapLibreNativeC.mln_map_set_debug_options(state.requireLive(), mask));
  }

  public EnumSet<DebugOption> debugOptions() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outOptions = arena.allocate(ValueLayout.JAVA_INT);
      Status.check(MapLibreNativeC.mln_map_get_debug_options(state.requireLive(), outOptions));
      var mask = outOptions.get(ValueLayout.JAVA_INT, 0);
      var options = EnumSet.noneOf(DebugOption.class);
      for (var option : DebugOption.values()) {
        if ((mask & option.nativeMask()) != 0) {
          options.add(option);
        }
      }
      return options;
    }
  }

  public void setRenderingStatsViewEnabled(boolean enabled) {
    NativeAccess.ensureLoaded();
    Status.check(
        MapLibreNativeC.mln_map_set_rendering_stats_view_enabled(state.requireLive(), enabled));
  }

  public boolean isRenderingStatsViewEnabled() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outEnabled = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(
          MapLibreNativeC.mln_map_get_rendering_stats_view_enabled(
              state.requireLive(), outEnabled));
      return outEnabled.get(ValueLayout.JAVA_BOOLEAN, 0);
    }
  }

  public boolean isFullyLoaded() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outLoaded = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(MapLibreNativeC.mln_map_is_fully_loaded(state.requireLive(), outLoaded));
      return outLoaded.get(ValueLayout.JAVA_BOOLEAN, 0);
    }
  }

  public void dumpDebugLogs() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_map_dump_debug_logs(state.requireLive()));
  }

  public ViewportOptions viewportOptions() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outOptions = MapLibreNativeC.mln_map_viewport_options_default(arena);
      Status.check(MapLibreNativeC.mln_map_get_viewport_options(state.requireLive(), outOptions));
      return MapStructs.viewportOptions(outOptions);
    }
  }

  public void setViewportOptions(ViewportOptions options) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(options, "options");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_viewport_options(
              state.requireLive(), MapStructs.viewportOptions(options, arena)));
    }
  }

  public TileOptions tileOptions() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outOptions = MapLibreNativeC.mln_map_tile_options_default(arena);
      Status.check(MapLibreNativeC.mln_map_get_tile_options(state.requireLive(), outOptions));
      return MapStructs.tileOptions(outOptions);
    }
  }

  public void setTileOptions(TileOptions options) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(options, "options");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_tile_options(
              state.requireLive(), MapStructs.tileOptions(options, arena)));
    }
  }

  public CameraOptions camera() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outCamera = MapLibreNativeC.mln_camera_options_default(arena);
      Status.check(MapLibreNativeC.mln_map_get_camera(state.requireLive(), outCamera));
      return MapStructs.cameraOptions(outCamera);
    }
  }

  public void jumpTo(CameraOptions camera) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(camera, "camera");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_jump_to(
              state.requireLive(), MapStructs.cameraOptions(camera, arena)));
    }
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
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_map_move_by(state.requireLive(), deltaX, deltaY));
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
    scaleByInternal(scale, Objects.requireNonNull(anchor, "anchor"), true);
  }

  public void scaleByAnimated(double scale) {
    scaleByAnimatedInternal(scale, null, false, null, false);
  }

  public void scaleByAnimated(double scale, ScreenPoint anchor) {
    scaleByAnimatedInternal(scale, Objects.requireNonNull(anchor, "anchor"), true, null, false);
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
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(first, "first");
    Objects.requireNonNull(second, "second");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_rotate_by(
              state.requireLive(),
              CoreStructs.screenPoint(first, arena),
              CoreStructs.screenPoint(second, arena)));
    }
  }

  public void rotateByAnimated(ScreenPoint first, ScreenPoint second) {
    rotateByAnimatedInternal(first, second, null, false);
  }

  public void rotateByAnimated(ScreenPoint first, ScreenPoint second, AnimationOptions animation) {
    rotateByAnimatedInternal(first, second, Objects.requireNonNull(animation, "animation"), true);
  }

  public void pitchBy(double pitch) {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_map_pitch_by(state.requireLive(), pitch));
  }

  public void pitchByAnimated(double pitch) {
    pitchByAnimatedInternal(pitch, null, false);
  }

  public void pitchByAnimated(double pitch, AnimationOptions animation) {
    pitchByAnimatedInternal(pitch, Objects.requireNonNull(animation, "animation"), true);
  }

  public void cancelTransitions() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_map_cancel_transitions(state.requireLive()));
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
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(camera, "camera");
    try (var arena = Arena.ofConfined()) {
      var outBounds = mln_lat_lng_bounds.allocate(arena);
      Status.check(
          MapLibreNativeC.mln_map_lat_lng_bounds_for_camera(
              state.requireLive(), MapStructs.cameraOptions(camera, arena), outBounds));
      return CoreStructs.latLngBounds(outBounds);
    }
  }

  public LatLngBounds latLngBoundsForCameraUnwrapped(CameraOptions camera) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(camera, "camera");
    try (var arena = Arena.ofConfined()) {
      var outBounds = mln_lat_lng_bounds.allocate(arena);
      Status.check(
          MapLibreNativeC.mln_map_lat_lng_bounds_for_camera_unwrapped(
              state.requireLive(), MapStructs.cameraOptions(camera, arena), outBounds));
      return CoreStructs.latLngBounds(outBounds);
    }
  }

  public BoundOptions bounds() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outOptions = MapLibreNativeC.mln_bound_options_default(arena);
      Status.check(MapLibreNativeC.mln_map_get_bounds(state.requireLive(), outOptions));
      return MapStructs.boundOptions(outOptions);
    }
  }

  public void setBounds(BoundOptions options) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(options, "options");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_bounds(
              state.requireLive(), MapStructs.boundOptions(options, arena)));
    }
  }

  public FreeCameraOptions freeCameraOptions() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outOptions = MapLibreNativeC.mln_free_camera_options_default(arena);
      Status.check(
          MapLibreNativeC.mln_map_get_free_camera_options(state.requireLive(), outOptions));
      return MapStructs.freeCameraOptions(outOptions);
    }
  }

  public void setFreeCameraOptions(FreeCameraOptions options) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(options, "options");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_free_camera_options(
              state.requireLive(), MapStructs.freeCameraOptions(options, arena)));
    }
  }

  public ProjectionModeOptions projectionMode() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var outMode = MapLibreNativeC.mln_projection_mode_default(arena);
      Status.check(MapLibreNativeC.mln_map_get_projection_mode(state.requireLive(), outMode));
      return MapStructs.projectionModeOptions(outMode);
    }
  }

  public void setProjectionMode(ProjectionModeOptions mode) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(mode, "mode");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_projection_mode(
              state.requireLive(), MapStructs.projectionModeOptions(mode, arena)));
    }
  }

  public ScreenPoint pixelForLatLng(LatLng coordinate) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(coordinate, "coordinate");
    try (var arena = Arena.ofConfined()) {
      var outPoint = mln_screen_point.allocate(arena);
      Status.check(
          MapLibreNativeC.mln_map_pixel_for_lat_lng(
              state.requireLive(), CoreStructs.latLng(coordinate, arena), outPoint));
      return CoreStructs.screenPoint(outPoint);
    }
  }

  public LatLng latLngForPixel(ScreenPoint point) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(point, "point");
    try (var arena = Arena.ofConfined()) {
      var outCoordinate = mln_lat_lng.allocate(arena);
      Status.check(
          MapLibreNativeC.mln_map_lat_lng_for_pixel(
              state.requireLive(), CoreStructs.screenPoint(point, arena), outCoordinate));
      return CoreStructs.latLng(outCoordinate);
    }
  }

  public List<ScreenPoint> pixelsForLatLngs(List<LatLng> coordinates) {
    NativeAccess.ensureLoaded();
    var copiedCoordinates = List.copyOf(Objects.requireNonNull(coordinates, "coordinates"));
    try (var arena = Arena.ofConfined()) {
      var outPoints =
          copiedCoordinates.isEmpty()
              ? MemorySegment.NULL
              : mln_screen_point.allocateArray(copiedCoordinates.size(), arena);
      Status.check(
          MapLibreNativeC.mln_map_pixels_for_lat_lngs(
              state.requireLive(),
              copiedCoordinates.isEmpty()
                  ? MemorySegment.NULL
                  : CoreStructs.latLngArray(copiedCoordinates, arena),
              copiedCoordinates.size(),
              outPoints));
      return copiedCoordinates.isEmpty()
          ? List.of()
          : CoreStructs.screenPointArray(outPoints, copiedCoordinates.size());
    }
  }

  public List<LatLng> latLngsForPixels(List<ScreenPoint> points) {
    NativeAccess.ensureLoaded();
    var copiedPoints = List.copyOf(Objects.requireNonNull(points, "points"));
    try (var arena = Arena.ofConfined()) {
      var outCoordinates =
          copiedPoints.isEmpty()
              ? MemorySegment.NULL
              : mln_lat_lng.allocateArray(copiedPoints.size(), arena);
      Status.check(
          MapLibreNativeC.mln_map_lat_lngs_for_pixels(
              state.requireLive(),
              copiedPoints.isEmpty()
                  ? MemorySegment.NULL
                  : CoreStructs.screenPointArray(copiedPoints, arena),
              copiedPoints.size(),
              outCoordinates));
      return copiedPoints.isEmpty()
          ? List.of()
          : CoreStructs.latLngArray(outCoordinates, copiedPoints.size());
    }
  }

  public MapProjectionHandle createProjection() {
    return MapProjectionHandle.create(this);
  }

  private void addVectorSourceUrlInternal(
      String sourceId, String url, TileSourceOptions options, boolean hasOptions) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_vector_source_url(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              CoreStructs.stringView(Objects.requireNonNull(url, "url"), arena),
              hasOptions ? StyleStructs.tileSourceOptions(options, arena) : MemorySegment.NULL));
    }
  }

  private void addVectorSourceTilesInternal(
      String sourceId, List<String> tiles, TileSourceOptions options, boolean hasOptions) {
    NativeAccess.ensureLoaded();
    var copiedTiles = List.copyOf(Objects.requireNonNull(tiles, "tiles"));
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_vector_source_tiles(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              copiedTiles.isEmpty()
                  ? MemorySegment.NULL
                  : StyleStructs.stringViewArray(copiedTiles, arena),
              copiedTiles.size(),
              hasOptions ? StyleStructs.tileSourceOptions(options, arena) : MemorySegment.NULL));
    }
  }

  private void addRasterSourceUrlInternal(
      String sourceId, String url, TileSourceOptions options, boolean hasOptions) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_raster_source_url(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              CoreStructs.stringView(Objects.requireNonNull(url, "url"), arena),
              hasOptions ? StyleStructs.tileSourceOptions(options, arena) : MemorySegment.NULL));
    }
  }

  private void addRasterSourceTilesInternal(
      String sourceId, List<String> tiles, TileSourceOptions options, boolean hasOptions) {
    NativeAccess.ensureLoaded();
    var copiedTiles = List.copyOf(Objects.requireNonNull(tiles, "tiles"));
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_raster_source_tiles(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              copiedTiles.isEmpty()
                  ? MemorySegment.NULL
                  : StyleStructs.stringViewArray(copiedTiles, arena),
              copiedTiles.size(),
              hasOptions ? StyleStructs.tileSourceOptions(options, arena) : MemorySegment.NULL));
    }
  }

  private void addRasterDemSourceUrlInternal(
      String sourceId, String url, TileSourceOptions options, boolean hasOptions) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_raster_dem_source_url(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              CoreStructs.stringView(Objects.requireNonNull(url, "url"), arena),
              hasOptions ? StyleStructs.tileSourceOptions(options, arena) : MemorySegment.NULL));
    }
  }

  private void addRasterDemSourceTilesInternal(
      String sourceId, List<String> tiles, TileSourceOptions options, boolean hasOptions) {
    NativeAccess.ensureLoaded();
    var copiedTiles = List.copyOf(Objects.requireNonNull(tiles, "tiles"));
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_add_raster_dem_source_tiles(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(sourceId, "sourceId"), arena),
              copiedTiles.isEmpty()
                  ? MemorySegment.NULL
                  : StyleStructs.stringViewArray(copiedTiles, arena),
              copiedTiles.size(),
              hasOptions ? StyleStructs.tileSourceOptions(options, arena) : MemorySegment.NULL));
    }
  }

  private void setStyleImageInternal(
      String imageId,
      PremultipliedRgba8Image image,
      StyleImageOptions options,
      boolean hasOptions) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(image, "image");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_style_image(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(imageId, "imageId"), arena),
              StyleStructs.premultipliedRgba8Image(image, arena),
              hasOptions ? StyleStructs.styleImageOptions(options, arena) : MemorySegment.NULL));
    }
  }

  private void setLayerFilterInternal(String layerId, JsonValue filter, boolean hasFilter) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_set_layer_filter(
              state.requireLive(),
              CoreStructs.stringView(Objects.requireNonNull(layerId, "layerId"), arena),
              hasFilter ? ValueStructs.jsonValue(filter, arena) : MemorySegment.NULL));
    }
  }

  private void easeToInternal(
      CameraOptions camera, AnimationOptions animation, boolean hasAnimation) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(camera, "camera");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_ease_to(
              state.requireLive(),
              MapStructs.cameraOptions(camera, arena),
              hasAnimation ? MapStructs.animationOptions(animation, arena) : MemorySegment.NULL));
    }
  }

  private void flyToInternal(
      CameraOptions camera, AnimationOptions animation, boolean hasAnimation) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(camera, "camera");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_fly_to(
              state.requireLive(),
              MapStructs.cameraOptions(camera, arena),
              hasAnimation ? MapStructs.animationOptions(animation, arena) : MemorySegment.NULL));
    }
  }

  private void moveByAnimatedInternal(
      double deltaX, double deltaY, AnimationOptions animation, boolean hasAnimation) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_move_by_animated(
              state.requireLive(),
              deltaX,
              deltaY,
              hasAnimation ? MapStructs.animationOptions(animation, arena) : MemorySegment.NULL));
    }
  }

  private void scaleByInternal(double scale, ScreenPoint anchor, boolean hasAnchor) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_scale_by(
              state.requireLive(),
              scale,
              hasAnchor ? CoreStructs.screenPoint(anchor, arena) : MemorySegment.NULL));
    }
  }

  private void scaleByAnimatedInternal(
      double scale,
      ScreenPoint anchor,
      boolean hasAnchor,
      AnimationOptions animation,
      boolean hasAnimation) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_scale_by_animated(
              state.requireLive(),
              scale,
              hasAnchor ? CoreStructs.screenPoint(anchor, arena) : MemorySegment.NULL,
              hasAnimation ? MapStructs.animationOptions(animation, arena) : MemorySegment.NULL));
    }
  }

  private void rotateByAnimatedInternal(
      ScreenPoint first, ScreenPoint second, AnimationOptions animation, boolean hasAnimation) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(first, "first");
    Objects.requireNonNull(second, "second");
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_rotate_by_animated(
              state.requireLive(),
              CoreStructs.screenPoint(first, arena),
              CoreStructs.screenPoint(second, arena),
              hasAnimation ? MapStructs.animationOptions(animation, arena) : MemorySegment.NULL));
    }
  }

  private void pitchByAnimatedInternal(
      double pitch, AnimationOptions animation, boolean hasAnimation) {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_map_pitch_by_animated(
              state.requireLive(),
              pitch,
              hasAnimation ? MapStructs.animationOptions(animation, arena) : MemorySegment.NULL));
    }
  }

  private CameraOptions cameraForLatLngBoundsInternal(
      LatLngBounds bounds, CameraFitOptions fitOptions, boolean hasFitOptions) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(bounds, "bounds");
    try (var arena = Arena.ofConfined()) {
      var outCamera = MapLibreNativeC.mln_camera_options_default(arena);
      Status.check(
          MapLibreNativeC.mln_map_camera_for_lat_lng_bounds(
              state.requireLive(),
              CoreStructs.latLngBounds(bounds, arena),
              hasFitOptions ? MapStructs.cameraFitOptions(fitOptions, arena) : MemorySegment.NULL,
              outCamera));
      return MapStructs.cameraOptions(outCamera);
    }
  }

  private CameraOptions cameraForLatLngsInternal(
      List<LatLng> coordinates, CameraFitOptions fitOptions, boolean hasFitOptions) {
    NativeAccess.ensureLoaded();
    var copiedCoordinates = List.copyOf(Objects.requireNonNull(coordinates, "coordinates"));
    try (var arena = Arena.ofConfined()) {
      var outCamera = MapLibreNativeC.mln_camera_options_default(arena);
      Status.check(
          MapLibreNativeC.mln_map_camera_for_lat_lngs(
              state.requireLive(),
              copiedCoordinates.isEmpty()
                  ? MemorySegment.NULL
                  : CoreStructs.latLngArray(copiedCoordinates, arena),
              copiedCoordinates.size(),
              hasFitOptions ? MapStructs.cameraFitOptions(fitOptions, arena) : MemorySegment.NULL,
              outCamera));
      return MapStructs.cameraOptions(outCamera);
    }
  }

  private CameraOptions cameraForGeometryInternal(
      Geometry geometry, CameraFitOptions fitOptions, boolean hasFitOptions) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(geometry, "geometry");
    try (var arena = Arena.ofConfined()) {
      var outCamera = MapLibreNativeC.mln_camera_options_default(arena);
      Status.check(
          MapLibreNativeC.mln_map_camera_for_geometry(
              state.requireLive(),
              ValueStructs.geometry(geometry, arena),
              hasFitOptions ? MapStructs.cameraFitOptions(fitOptions, arena) : MemorySegment.NULL,
              outCamera));
      return MapStructs.cameraOptions(outCamera);
    }
  }

  @Override
  public void close() {
    NativeAccess.ensureLoaded();
    state.closeOnce(
        MapLibreNativeC::mln_map_destroy,
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

  public MemorySegment nativeHandle(InternalAccess access) {
    Objects.requireNonNull(access, "access");
    return nativeHandle();
  }

  MemorySegment nativeHandle() {
    return state.requireLive();
  }

  public long nativeAddress(InternalAccess access) {
    Objects.requireNonNull(access, "access");
    return nativeAddress();
  }

  long nativeAddress() {
    return state.address();
  }

  public void releaseDetachedCustomGeometrySources(InternalAccess access) {
    Objects.requireNonNull(access, "access");
    releaseDetachedCustomGeometrySources();
  }

  void releaseDetachedCustomGeometrySources() {
    try (var arena = Arena.ofConfined()) {
      var map = state.requireLive();
      var iterator = customGeometrySources.entrySet().iterator();
      while (iterator.hasNext()) {
        var entry = iterator.next();
        var outType = arena.allocate(ValueLayout.JAVA_INT);
        var outFound = arena.allocate(ValueLayout.JAVA_BOOLEAN);
        var status =
            MapLibreNativeC.mln_map_get_style_source_type(
                map, CoreStructs.stringView(entry.getKey(), arena), outType, outFound);
        if (status != MapLibreNativeC.MLN_STATUS_OK()) {
          continue;
        }
        var found = outFound.get(ValueLayout.JAVA_BOOLEAN, 0);
        var sourceType = SourceType.fromNative(outType.get(ValueLayout.JAVA_INT, 0));
        if (!found || sourceType != SourceType.CUSTOM_VECTOR) {
          closeQuietly(entry.getValue());
          iterator.remove();
        }
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
