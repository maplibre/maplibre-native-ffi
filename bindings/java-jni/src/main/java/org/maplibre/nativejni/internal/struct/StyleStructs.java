package org.maplibre.nativejni.internal.struct;

import java.util.ArrayList;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.bytedeco.javacpp.PointerPointer;
import org.bytedeco.javacpp.SizeTPointer;
import org.maplibre.nativejni.geo.Feature;
import org.maplibre.nativejni.geo.FeatureIdentifier;
import org.maplibre.nativejni.geo.GeoJson;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.JavaCppValues;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.json.JsonValue;

/** JavaCPP-backed materializers and readers for style JNI calls. */
public final class StyleStructs {

  private StyleStructs() {}

  public static String[] styleIdList(MaplibreNativeC.mln_style_id_list list) {
    try (var count = new SizeTPointer(1)) {
      var status = MaplibreNativeC.mln_style_id_list_count(list, count);
      org.maplibre.nativejni.internal.status.Status.check(status);
      var ids = new String[Math.toIntExact(count.get())];
      for (var i = 0; i < ids.length; i++) {
        var view = new MaplibreNativeC.mln_string_view();
        status = MaplibreNativeC.mln_style_id_list_get(list, i, view);
        org.maplibre.nativejni.internal.status.Status.check(status);
        ids[i] = JavaCppValues.string(view);
        view.close();
      }
      return ids;
    } finally {
      MaplibreNativeC.mln_style_id_list_destroy(list);
    }
  }

  public static JsonValue jsonSnapshot(
      PointerPointer<MaplibreNativeC.mln_json_snapshot> outSnapshot) {
    var snapshotAddress =
        JavaCppSupport.outAddress(outSnapshot, MaplibreNativeC.mln_json_snapshot.class);
    if (snapshotAddress == 0) {
      return null;
    }
    var snapshot = new MaplibreNativeC.mln_json_snapshot(JavaCppSupport.pointer(snapshotAddress));
    try {
      var outValue = JavaCppSupport.outPointer(MaplibreNativeC.mln_json_value.class);
      var status = MaplibreNativeC.mln_json_snapshot_get(snapshot, outValue);
      org.maplibre.nativejni.internal.status.Status.check(status);
      var valueAddress = JavaCppSupport.outAddress(outValue, MaplibreNativeC.mln_json_value.class);
      return valueAddress == 0
          ? null
          : JavaCppValues.jsonValue(
              new MaplibreNativeC.mln_json_value(JavaCppSupport.pointer(valueAddress)));
    } finally {
      MaplibreNativeC.mln_json_snapshot_destroy(snapshot);
    }
  }

  public static GeoJsonScope geoJson(GeoJson value) {
    return new GeoJsonScope(value);
  }

  public static TileIdScope canonicalTileId(int z, long x, long y) {
    return new TileIdScope(z, x, y);
  }

  public static BoundsScope latLngBounds(
      double southwestLatitude,
      double southwestLongitude,
      double northeastLatitude,
      double northeastLongitude) {
    return new BoundsScope(
        southwestLatitude, southwestLongitude, northeastLatitude, northeastLongitude);
  }

  public static LatLngArrayScope latLngArray(double[] coordinates) {
    return new LatLngArrayScope(coordinates);
  }

  public static PremultipliedImageScope premultipliedRgba8Image(
      int width, int height, int stride, byte[] pixels) {
    return new PremultipliedImageScope(width, height, stride, pixels);
  }

  public static TileOptionsScope tileSourceOptions(
      boolean[] fields, double[] values, String attribution) {
    return new TileOptionsScope(fields, values, attribution);
  }

  public static final class TileIdScope implements AutoCloseable {
    private final MaplibreNativeC.mln_canonical_tile_id tileId;

    public TileIdScope(int z, long x, long y) {
      tileId = new MaplibreNativeC.mln_canonical_tile_id();
      tileId.z(z);
      tileId.x((int) x);
      tileId.y((int) y);
    }

    public MaplibreNativeC.mln_canonical_tile_id tileId() {
      return tileId;
    }

    @Override
    public void close() {
      tileId.close();
    }
  }

  public static final class BoundsScope implements AutoCloseable {
    private final MaplibreNativeC.mln_lat_lng_bounds bounds;

    public BoundsScope(
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

    public MaplibreNativeC.mln_lat_lng_bounds bounds() {
      return bounds;
    }

    @Override
    public void close() {
      bounds.close();
    }
  }

  public static final class GeoJsonScope implements AutoCloseable {
    private final ArrayList<Pointer> owned = new ArrayList<>();
    private final ArrayList<JavaCppValues.StringViewScope> strings = new ArrayList<>();
    private final ArrayList<JavaCppValues.JsonScope> jsonValues = new ArrayList<>();
    private final MaplibreNativeC.mln_geojson value;

    public GeoJsonScope(GeoJson value) {
      this.value = geoJson(value);
    }

    public MaplibreNativeC.mln_geojson value() {
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

  public static final class LatLngArrayScope implements AutoCloseable {
    private final MaplibreNativeC.mln_lat_lng coordinates;
    private final long count;

    public LatLngArrayScope(double[] values) {
      this(values.length / 2);
      for (var i = 0; i < count; i++) {
        coordinates.position(i).latitude(values[i * 2]).longitude(values[i * 2 + 1]);
      }
      coordinates.position(0);
    }

    public LatLngArrayScope(long count) {
      this.count = count;
      this.coordinates = new MaplibreNativeC.mln_lat_lng(count);
    }

    public MaplibreNativeC.mln_lat_lng coordinates() {
      return coordinates;
    }

    public long count() {
      return count;
    }

    public void copyTo(double[] out, long coordinateCount) {
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

  public static final class PremultipliedImageScope implements AutoCloseable {
    private final BytePointer pixels;
    private final MaplibreNativeC.mln_premultiplied_rgba8_image image;

    public PremultipliedImageScope(int width, int height, int stride, byte[] pixels) {
      this.pixels = new BytePointer(pixels.length);
      this.pixels.put(pixels);
      this.image = MaplibreNativeC.mln_premultiplied_rgba8_image_default();
      image.width(width);
      image.height(height);
      image.stride(stride);
      image.pixels(this.pixels);
      image.byte_length(pixels.length);
    }

    public MaplibreNativeC.mln_premultiplied_rgba8_image image() {
      return image;
    }

    @Override
    public void close() {
      image.close();
      pixels.close();
    }
  }

  public static final class TileOptionsScope implements AutoCloseable {
    private final JavaCppValues.StringViewScope attribution;
    private final MaplibreNativeC.mln_style_tile_source_options options;

    public TileOptionsScope(boolean[] fields, double[] values, String attribution) {
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

    public MaplibreNativeC.mln_style_tile_source_options options() {
      return options;
    }

    @Override
    public void close() {
      options.close();
      attribution.close();
    }
  }
}
