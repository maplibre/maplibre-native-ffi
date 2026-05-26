package org.maplibre.nativejni.internal.struct;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import org.bytedeco.javacpp.Pointer;
import org.bytedeco.javacpp.PointerPointer;
import org.bytedeco.javacpp.SizeTPointer;
import org.maplibre.nativejni.geo.Feature;
import org.maplibre.nativejni.geo.FeatureIdentifier;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.ScreenBox;
import org.maplibre.nativejni.geo.ScreenPoint;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.JavaCppValues;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.json.JsonValue;
import org.maplibre.nativejni.query.FeatureExtensionResult;
import org.maplibre.nativejni.query.FeatureStateSelector;
import org.maplibre.nativejni.query.QueriedFeature;
import org.maplibre.nativejni.query.RenderedFeatureQueryOptions;
import org.maplibre.nativejni.query.RenderedQueryGeometry;
import org.maplibre.nativejni.query.SourceFeatureQueryOptions;

/** JavaCPP-backed materializers and readers for query-related JNI calls. */
public final class QueryStructs {
  private QueryStructs() {}

  public static SelectorScope featureStateSelector(FeatureStateSelector value) {
    return new SelectorScope(value);
  }

  public static JsonValue jsonSnapshot(long snapshotAddress) {
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

  public static List<QueriedFeature> featureQueryResult(
      PointerPointer<MaplibreNativeC.mln_feature_query_result> outResult) {
    var resultAddress =
        JavaCppSupport.outAddress(outResult, MaplibreNativeC.mln_feature_query_result.class);
    var result =
        new MaplibreNativeC.mln_feature_query_result(JavaCppSupport.pointer(resultAddress));
    try (var count = new SizeTPointer(1)) {
      var status = MaplibreNativeC.mln_feature_query_result_count(result, count);
      org.maplibre.nativejni.internal.status.Status.check(status);
      var features = new QueriedFeature[Math.toIntExact(count.get())];
      for (var i = 0; i < features.length; i++) {
        var feature = new MaplibreNativeC.mln_queried_feature();
        feature.size(feature.sizeof());
        status = MaplibreNativeC.mln_feature_query_result_get(result, i, feature);
        if (status != MaplibreNativeC.MLN_STATUS_OK) {
          feature.close();
          org.maplibre.nativejni.internal.status.Status.check(status);
        }
        features[i] = queriedFeature(feature);
        feature.close();
      }
      return List.of(features);
    } finally {
      MaplibreNativeC.mln_feature_query_result_destroy(result);
    }
  }

  public static FeatureExtensionResult featureExtensionResult(
      PointerPointer<MaplibreNativeC.mln_feature_extension_result> outResult) {
    var resultAddress =
        JavaCppSupport.outAddress(outResult, MaplibreNativeC.mln_feature_extension_result.class);
    var result =
        new MaplibreNativeC.mln_feature_extension_result(JavaCppSupport.pointer(resultAddress));
    try {
      var info = new MaplibreNativeC.mln_feature_extension_result_info();
      info.size(info.sizeof());
      var status = MaplibreNativeC.mln_feature_extension_result_get(result, info);
      org.maplibre.nativejni.internal.status.Status.check(status);
      var javaResult = extensionResult(info);
      info.close();
      return javaResult;
    } finally {
      MaplibreNativeC.mln_feature_extension_result_destroy(result);
    }
  }

  private static QueriedFeature queriedFeature(MaplibreNativeC.mln_queried_feature feature) {
    var fields = feature.fields();
    return new QueriedFeature(
        feature(feature.feature()),
        (fields & MaplibreNativeC.MLN_QUERIED_FEATURE_SOURCE_ID) != 0
            ? Optional.of(JavaCppValues.string(feature.source_id()))
            : Optional.empty(),
        (fields & MaplibreNativeC.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID) != 0
            ? Optional.of(JavaCppValues.string(feature.source_layer_id()))
            : Optional.empty(),
        (fields & MaplibreNativeC.MLN_QUERIED_FEATURE_STATE) != 0
            ? Optional.of(JavaCppValues.jsonValue(feature.state()))
            : Optional.empty());
  }

  private static FeatureExtensionResult extensionResult(
      MaplibreNativeC.mln_feature_extension_result_info info) {
    return switch (info.type()) {
      case MaplibreNativeC.MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE ->
          new FeatureExtensionResult.Value(JavaCppValues.jsonValue(info.data_value()));
      case MaplibreNativeC.MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION ->
          new FeatureExtensionResult.FeatureCollection(features(info.data_feature_collection()));
      default -> new FeatureExtensionResult.Unknown(info.type());
    };
  }

  private static Feature feature(MaplibreNativeC.mln_feature feature) {
    var properties = new ArrayList<JsonValue.Member>(Math.toIntExact(feature.property_count()));
    var nativeProperties = feature.properties();
    for (var i = 0; i < feature.property_count(); i++) {
      var property = nativeProperties.getPointer(i);
      properties.add(
          new JsonValue.Member(
              JavaCppValues.string(property.key()), JavaCppValues.jsonValue(property.value())));
    }
    return new Feature(geometry(feature.geometry()), properties, identifier(feature));
  }

  private static List<Feature> features(MaplibreNativeC.mln_feature_collection collection) {
    var features = new ArrayList<Feature>(Math.toIntExact(collection.feature_count()));
    var nativeFeatures = collection.features();
    for (var i = 0; i < collection.feature_count(); i++) {
      features.add(feature(nativeFeatures.getPointer(i)));
    }
    return features;
  }

  private static FeatureIdentifier identifier(MaplibreNativeC.mln_feature feature) {
    return switch (feature.identifier_type()) {
      case MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_NULL -> FeatureIdentifier.nullValue();
      case MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_UINT ->
          FeatureIdentifier.unsigned(feature.identifier_uint_value());
      case MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_INT ->
          FeatureIdentifier.of(feature.identifier_int_value());
      case MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE ->
          FeatureIdentifier.of(feature.identifier_double_value());
      case MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_STRING ->
          FeatureIdentifier.of(JavaCppValues.string(feature.identifier_string_value()));
      default -> FeatureIdentifier.nullValue();
    };
  }

  private static Geometry geometry(MaplibreNativeC.mln_geometry geometry) {
    return switch (geometry.type()) {
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_EMPTY -> Geometry.empty();
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_POINT ->
          Geometry.point(coordinate(geometry.data_point()));
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_LINE_STRING ->
          Geometry.lineString(coordinates(geometry.data_line_string()));
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_POLYGON ->
          Geometry.polygon(polygon(geometry.data_polygon()));
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POINT ->
          Geometry.multiPoint(coordinates(geometry.data_multi_point()));
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING ->
          Geometry.multiLineString(multiLine(geometry.data_multi_line_string()));
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POLYGON ->
          Geometry.multiPolygon(multiPolygon(geometry.data_multi_polygon()));
      case MaplibreNativeC.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION ->
          Geometry.collection(collection(geometry.data_geometry_collection()));
      default -> throw new IllegalArgumentException("unknown geometry type: " + geometry.type());
    };
  }

  private static LatLng coordinate(MaplibreNativeC.mln_lat_lng coordinate) {
    return new LatLng(coordinate.latitude(), coordinate.longitude());
  }

  private static List<LatLng> coordinates(MaplibreNativeC.mln_coordinate_span span) {
    var values = new ArrayList<LatLng>(Math.toIntExact(span.coordinate_count()));
    var nativeValues = span.coordinates();
    for (var i = 0; i < span.coordinate_count(); i++)
      values.add(coordinate(nativeValues.getPointer(i)));
    return values;
  }

  private static List<List<LatLng>> polygon(MaplibreNativeC.mln_polygon_geometry polygon) {
    var rings = new ArrayList<List<LatLng>>(Math.toIntExact(polygon.ring_count()));
    var nativeRings = polygon.rings();
    for (var i = 0; i < polygon.ring_count(); i++)
      rings.add(coordinates(nativeRings.getPointer(i)));
    return rings;
  }

  private static List<List<LatLng>> multiLine(MaplibreNativeC.mln_multi_line_geometry multiLine) {
    var lines = new ArrayList<List<LatLng>>(Math.toIntExact(multiLine.line_count()));
    var nativeLines = multiLine.lines();
    for (var i = 0; i < multiLine.line_count(); i++)
      lines.add(coordinates(nativeLines.getPointer(i)));
    return lines;
  }

  private static List<List<List<LatLng>>> multiPolygon(
      MaplibreNativeC.mln_multi_polygon_geometry multiPolygon) {
    var polygons = new ArrayList<List<List<LatLng>>>(Math.toIntExact(multiPolygon.polygon_count()));
    var nativePolygons = multiPolygon.polygons();
    for (var i = 0; i < multiPolygon.polygon_count(); i++)
      polygons.add(polygon(nativePolygons.getPointer(i)));
    return polygons;
  }

  private static List<Geometry> collection(MaplibreNativeC.mln_geometry_collection collection) {
    var geometries = new ArrayList<Geometry>(Math.toIntExact(collection.geometry_count()));
    var nativeGeometries = collection.geometries();
    for (var i = 0; i < collection.geometry_count(); i++)
      geometries.add(geometry(nativeGeometries.getPointer(i)));
    return geometries;
  }

  private static MaplibreNativeC.mln_screen_point point(ScreenPoint point) {
    var out = new MaplibreNativeC.mln_screen_point();
    out.x(point.x());
    out.y(point.y());
    return out;
  }

  public static final class RenderedGeometryScope implements AutoCloseable {
    private final ArrayList<Pointer> owned = new ArrayList<>();
    private final MaplibreNativeC.mln_rendered_query_geometry geometry;

    public RenderedGeometryScope(RenderedQueryGeometry value) {
      if (value instanceof RenderedQueryGeometry.Point node) {
        geometry = MaplibreNativeC.mln_rendered_query_geometry_point(own(point(node.point())));
      } else if (value instanceof RenderedQueryGeometry.Box node) {
        geometry = MaplibreNativeC.mln_rendered_query_geometry_box(box(node.box()));
      } else if (value instanceof RenderedQueryGeometry.LineString node) {
        geometry =
            MaplibreNativeC.mln_rendered_query_geometry_line_string(
                points(node.points()), node.points().size());
      } else {
        throw new IllegalArgumentException("unknown rendered query geometry");
      }
    }

    public MaplibreNativeC.mln_rendered_query_geometry geometry() {
      return geometry;
    }

    @Override
    public void close() {
      geometry.close();
      for (var i = owned.size() - 1; i >= 0; i--) owned.get(i).close();
    }

    private MaplibreNativeC.mln_screen_box box(ScreenBox box) {
      var out = own(new MaplibreNativeC.mln_screen_box());
      out.min(point(box.min()));
      out.max(point(box.max()));
      return out;
    }

    private MaplibreNativeC.mln_screen_point points(List<ScreenPoint> points) {
      var nativePoints = own(new MaplibreNativeC.mln_screen_point(points.size()));
      for (var i = 0; i < points.size(); i++) {
        nativePoints.position(i).x(points.get(i).x()).y(points.get(i).y());
      }
      nativePoints.position(0);
      return nativePoints;
    }

    private <T extends Pointer> T own(T pointer) {
      owned.add(pointer);
      return pointer;
    }
  }

  public static final class RenderedOptionsScope implements AutoCloseable {
    private final JavaCppValues.StringViewsScope layerIds;
    private final JavaCppValues.JsonScope filter;
    private final MaplibreNativeC.mln_rendered_feature_query_options options;

    public RenderedOptionsScope(RenderedFeatureQueryOptions value) {
      options = MaplibreNativeC.mln_rendered_feature_query_options_default();
      int fields = 0;
      layerIds =
          value != null && value.hasLayerIds()
              ? JavaCppValues.stringViews(value.layerIds().toArray(String[]::new))
              : null;
      filter = value != null && value.hasFilter() ? JavaCppValues.json(value.filter()) : null;
      if (layerIds != null) {
        fields |= MaplibreNativeC.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
        options.layer_ids(layerIds.views());
        options.layer_id_count(layerIds.count());
      }
      if (filter != null) options.filter(filter.value());
      options.fields(fields);
    }

    public MaplibreNativeC.mln_rendered_feature_query_options options() {
      return options;
    }

    @Override
    public void close() {
      options.close();
      if (filter != null) filter.close();
      if (layerIds != null) layerIds.close();
    }
  }

  public static final class SelectorScope implements AutoCloseable {
    private final JavaCppValues.StringViewScope sourceId;
    private final JavaCppValues.StringViewScope sourceLayerId;
    private final JavaCppValues.StringViewScope featureId;
    private final JavaCppValues.StringViewScope stateKey;
    private final MaplibreNativeC.mln_feature_state_selector selector;

    private SelectorScope(FeatureStateSelector value) {
      this.sourceId = JavaCppValues.stringView(value.sourceId());
      this.sourceLayerId =
          value.hasSourceLayerId() ? JavaCppValues.stringView(value.sourceLayerId()) : null;
      this.featureId = value.hasFeatureId() ? JavaCppValues.stringView(value.featureId()) : null;
      this.stateKey = value.hasStateKey() ? JavaCppValues.stringView(value.stateKey()) : null;
      this.selector = new MaplibreNativeC.mln_feature_state_selector();
      selector.size(selector.sizeof());
      selector.source_id(sourceId.view());
      int fields = 0;
      if (sourceLayerId != null) {
        fields |= MaplibreNativeC.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
        selector.source_layer_id(sourceLayerId.view());
      }
      if (featureId != null) {
        fields |= MaplibreNativeC.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
        selector.feature_id(featureId.view());
      }
      if (stateKey != null) {
        fields |= MaplibreNativeC.MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
        selector.state_key(stateKey.view());
      }
      selector.fields(fields);
    }

    public MaplibreNativeC.mln_feature_state_selector selector() {
      return selector;
    }

    @Override
    public void close() {
      selector.close();
      if (stateKey != null) stateKey.close();
      if (featureId != null) featureId.close();
      if (sourceLayerId != null) sourceLayerId.close();
      sourceId.close();
    }
  }

  public static final class SourceOptionsScope implements AutoCloseable {
    private final JavaCppValues.StringViewsScope sourceLayerIds;
    private final JavaCppValues.JsonScope filter;
    private final MaplibreNativeC.mln_source_feature_query_options options;

    public SourceOptionsScope(SourceFeatureQueryOptions value) {
      options = MaplibreNativeC.mln_source_feature_query_options_default();
      int fields = 0;
      sourceLayerIds =
          value != null && value.hasSourceLayerIds()
              ? JavaCppValues.stringViews(value.sourceLayerIds().toArray(String[]::new))
              : null;
      filter = value != null && value.hasFilter() ? JavaCppValues.json(value.filter()) : null;
      if (sourceLayerIds != null) {
        fields |= MaplibreNativeC.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
        options.source_layer_ids(sourceLayerIds.views());
        options.source_layer_id_count(sourceLayerIds.count());
      }
      if (filter != null) options.filter(filter.value());
      options.fields(fields);
    }

    public MaplibreNativeC.mln_source_feature_query_options options() {
      return options;
    }

    @Override
    public void close() {
      options.close();
      if (filter != null) filter.close();
      if (sourceLayerIds != null) sourceLayerIds.close();
    }
  }

  public static final class FeatureScope implements AutoCloseable {
    private final ArrayList<Pointer> owned = new ArrayList<>();
    private final ArrayList<JavaCppValues.StringViewScope> strings = new ArrayList<>();
    private final ArrayList<JavaCppValues.JsonScope> values = new ArrayList<>();
    private final MaplibreNativeC.mln_feature feature;

    public FeatureScope(Feature value) {
      feature = feature(value);
    }

    public MaplibreNativeC.mln_feature feature() {
      return feature;
    }

    @Override
    public void close() {
      for (var i = values.size() - 1; i >= 0; i--) values.get(i).close();
      for (var i = owned.size() - 1; i >= 0; i--) owned.get(i).close();
      for (var i = strings.size() - 1; i >= 0; i--) strings.get(i).close();
    }

    private MaplibreNativeC.mln_feature feature(Feature value) {
      var out = own(new MaplibreNativeC.mln_feature());
      out.size(out.sizeof());
      out.geometry(geometry(value.geometry()));
      if (!value.properties().isEmpty()) {
        var properties = own(new MaplibreNativeC.mln_json_member(value.properties().size()));
        for (var i = 0; i < value.properties().size(); i++) {
          var property = value.properties().get(i);
          properties.position(i).key(string(property.key())).value(json(property.value()));
        }
        properties.position(0);
        out.properties(properties);
      }
      out.property_count(value.properties().size());
      setIdentifier(out, value.identifier());
      return out;
    }

    private void setIdentifier(MaplibreNativeC.mln_feature out, FeatureIdentifier identifier) {
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
          var collection = own(new MaplibreNativeC.mln_geometry_collection());
          if (!node.geometries().isEmpty()) {
            var geometries = own(new MaplibreNativeC.mln_geometry(node.geometries().size()));
            for (var i = 0; i < node.geometries().size(); i++)
              geometries.position(i).put(geometry(node.geometries().get(i)));
            geometries.position(0);
            collection.geometries(geometries);
          }
          collection.geometry_count(node.geometries().size());
          out.data_geometry_collection(collection);
        }
      }
      return out;
    }

    private MaplibreNativeC.mln_coordinate_span coordinateSpan(List<LatLng> values) {
      var span = own(new MaplibreNativeC.mln_coordinate_span());
      if (!values.isEmpty()) {
        var coordinates = own(new MaplibreNativeC.mln_lat_lng(values.size()));
        for (var i = 0; i < values.size(); i++)
          coordinates
              .position(i)
              .latitude(values.get(i).latitude())
              .longitude(values.get(i).longitude());
        coordinates.position(0);
        span.coordinates(coordinates);
      }
      span.coordinate_count(values.size());
      return span;
    }

    private MaplibreNativeC.mln_polygon_geometry polygon(List<List<LatLng>> rings) {
      var out = own(new MaplibreNativeC.mln_polygon_geometry());
      if (!rings.isEmpty()) {
        var nativeRings = own(new MaplibreNativeC.mln_coordinate_span(rings.size()));
        for (var i = 0; i < rings.size(); i++)
          nativeRings.position(i).put(coordinateSpan(rings.get(i)));
        nativeRings.position(0);
        out.rings(nativeRings);
      }
      out.ring_count(rings.size());
      return out;
    }

    private MaplibreNativeC.mln_multi_line_geometry multiLine(List<List<LatLng>> lines) {
      var out = own(new MaplibreNativeC.mln_multi_line_geometry());
      if (!lines.isEmpty()) {
        var nativeLines = own(new MaplibreNativeC.mln_coordinate_span(lines.size()));
        for (var i = 0; i < lines.size(); i++)
          nativeLines.position(i).put(coordinateSpan(lines.get(i)));
        nativeLines.position(0);
        out.lines(nativeLines);
      }
      out.line_count(lines.size());
      return out;
    }

    private MaplibreNativeC.mln_multi_polygon_geometry multiPolygon(
        List<List<List<LatLng>>> polygons) {
      var out = own(new MaplibreNativeC.mln_multi_polygon_geometry());
      if (!polygons.isEmpty()) {
        var nativePolygons = own(new MaplibreNativeC.mln_polygon_geometry(polygons.size()));
        for (var i = 0; i < polygons.size(); i++)
          nativePolygons.position(i).put(polygon(polygons.get(i)));
        nativePolygons.position(0);
        out.polygons(nativePolygons);
      }
      out.polygon_count(polygons.size());
      return out;
    }

    private MaplibreNativeC.mln_lat_lng coordinate(LatLng value) {
      var out = own(new MaplibreNativeC.mln_lat_lng());
      out.latitude(value.latitude()).longitude(value.longitude());
      return out;
    }

    private MaplibreNativeC.mln_json_value json(JsonValue value) {
      var json = JavaCppValues.json(value);
      values.add(json);
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
}
