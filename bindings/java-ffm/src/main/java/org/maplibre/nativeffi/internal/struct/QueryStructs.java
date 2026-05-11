package org.maplibre.nativeffi.internal.struct;

import static org.maplibre.nativeffi.internal.struct.CoreStructs.screenPoint;
import static org.maplibre.nativeffi.internal.struct.CoreStructs.screenPointArray;
import static org.maplibre.nativeffi.internal.struct.CoreStructs.sizedArray;
import static org.maplibre.nativeffi.internal.struct.CoreStructs.stringView;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import org.maplibre.nativeffi.geo.Feature;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_feature;
import org.maplibre.nativeffi.internal.c.mln_feature_collection;
import org.maplibre.nativeffi.internal.c.mln_feature_extension_result_info;
import org.maplibre.nativeffi.internal.c.mln_feature_state_selector;
import org.maplibre.nativeffi.internal.c.mln_queried_feature;
import org.maplibre.nativeffi.internal.c.mln_rendered_feature_query_options;
import org.maplibre.nativeffi.internal.c.mln_screen_box;
import org.maplibre.nativeffi.internal.c.mln_source_feature_query_options;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.internal.status.Status;
import org.maplibre.nativeffi.json.JsonValue;
import org.maplibre.nativeffi.query.FeatureExtensionResult;
import org.maplibre.nativeffi.query.FeatureStateSelector;
import org.maplibre.nativeffi.query.QueriedFeature;
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions;
import org.maplibre.nativeffi.query.RenderedQueryGeometry;
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions;

/** Internal materializers and readers for feature state and query structs. */
public final class QueryStructs {
  private QueryStructs() {}

  public static MemorySegment featureStateSelector(FeatureStateSelector selector, Arena arena) {
    var segment = mln_feature_state_selector.allocate(arena);
    mln_feature_state_selector.size(segment, (int) mln_feature_state_selector.sizeof());
    mln_feature_state_selector.source_id(segment, stringView(selector.sourceId(), arena));
    var fields = 0;
    if (selector.hasSourceLayerId()) {
      fields |= MapLibreNativeC.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID();
      mln_feature_state_selector.source_layer_id(
          segment, stringView(selector.sourceLayerId(), arena));
    }
    if (selector.hasFeatureId()) {
      fields |= MapLibreNativeC.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID();
      mln_feature_state_selector.feature_id(segment, stringView(selector.featureId(), arena));
    }
    if (selector.hasStateKey()) {
      fields |= MapLibreNativeC.MLN_FEATURE_STATE_SELECTOR_STATE_KEY();
      mln_feature_state_selector.state_key(segment, stringView(selector.stateKey(), arena));
    }
    mln_feature_state_selector.fields(segment, fields);
    return segment;
  }

  public static MemorySegment renderedQueryGeometry(RenderedQueryGeometry geometry, Arena arena) {
    if (geometry instanceof RenderedQueryGeometry.Point point) {
      return MapLibreNativeC.mln_rendered_query_geometry_point(
          arena, screenPoint(point.point(), arena));
    }
    if (geometry instanceof RenderedQueryGeometry.Box box) {
      var nativeBox = mln_screen_box.allocate(arena);
      mln_screen_box.min(nativeBox, screenPoint(box.box().min(), arena));
      mln_screen_box.max(nativeBox, screenPoint(box.box().max(), arena));
      return MapLibreNativeC.mln_rendered_query_geometry_box(arena, nativeBox);
    }
    if (geometry instanceof RenderedQueryGeometry.LineString lineString) {
      var points = lineString.points();
      return MapLibreNativeC.mln_rendered_query_geometry_line_string(
          arena,
          points.isEmpty() ? MemorySegment.NULL : screenPointArray(points, arena),
          points.size());
    }
    throw new IllegalArgumentException(
        "Unsupported rendered query geometry type: " + geometry.getClass().getName());
  }

  public static MemorySegment renderedFeatureQueryOptions(
      RenderedFeatureQueryOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_rendered_feature_query_options_default(arena);
    var fields = 0;
    if (options.hasLayerIds()) {
      fields |= MapLibreNativeC.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS();
      var ids = options.layerIds();
      mln_rendered_feature_query_options.layer_ids(
          segment, ids.isEmpty() ? MemorySegment.NULL : StyleStructs.stringViewArray(ids, arena));
      mln_rendered_feature_query_options.layer_id_count(segment, ids.size());
    }
    if (options.hasFilter()) {
      mln_rendered_feature_query_options.filter(
          segment, ValueStructs.jsonValue(options.filter(), arena));
    }
    mln_rendered_feature_query_options.fields(segment, fields);
    return segment;
  }

  public static MemorySegment sourceFeatureQueryOptions(
      SourceFeatureQueryOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_source_feature_query_options_default(arena);
    var fields = 0;
    if (options.hasSourceLayerIds()) {
      fields |= MapLibreNativeC.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS();
      var ids = options.sourceLayerIds();
      mln_source_feature_query_options.source_layer_ids(
          segment, ids.isEmpty() ? MemorySegment.NULL : StyleStructs.stringViewArray(ids, arena));
      mln_source_feature_query_options.source_layer_id_count(segment, ids.size());
    }
    if (options.hasFilter()) {
      mln_source_feature_query_options.filter(
          segment, ValueStructs.jsonValue(options.filter(), arena));
    }
    mln_source_feature_query_options.fields(segment, fields);
    return segment;
  }

  public static List<QueriedFeature> featureQueryResult(MemorySegment result) {
    try (var arena = Arena.ofConfined()) {
      var outCount = arena.allocate(ValueLayout.JAVA_LONG);
      Status.check(MapLibreNativeC.mln_feature_query_result_count(result, outCount));
      var count = Math.toIntExact(outCount.get(ValueLayout.JAVA_LONG, 0));
      var features = new ArrayList<QueriedFeature>(count);
      for (var index = 0; index < count; index++) {
        var outFeature = mln_queried_feature.allocate(arena);
        mln_queried_feature.size(outFeature, (int) mln_queried_feature.sizeof());
        Status.check(MapLibreNativeC.mln_feature_query_result_get(result, index, outFeature));
        features.add(queriedFeature(outFeature));
      }
      return List.copyOf(features);
    } finally {
      MapLibreNativeC.mln_feature_query_result_destroy(result);
    }
  }

  public static FeatureExtensionResult featureExtensionResult(MemorySegment result) {
    try (var arena = Arena.ofConfined()) {
      var info = mln_feature_extension_result_info.allocate(arena);
      mln_feature_extension_result_info.size(
          info, (int) mln_feature_extension_result_info.sizeof());
      Status.check(MapLibreNativeC.mln_feature_extension_result_get(result, info));
      var type = mln_feature_extension_result_info.type(info);
      var data = mln_feature_extension_result_info.data(info);
      if (type == MapLibreNativeC.MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE()) {
        return new FeatureExtensionResult.Value(
            ValueStructs.jsonValue(mln_feature_extension_result_info.data.value(data)));
      }
      if (type == MapLibreNativeC.MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION()) {
        return new FeatureExtensionResult.FeatureCollection(
            featureCollection(mln_feature_extension_result_info.data.feature_collection(data)));
      }
      return new FeatureExtensionResult.Unknown(type);
    } finally {
      MapLibreNativeC.mln_feature_extension_result_destroy(result);
    }
  }

  private static QueriedFeature queriedFeature(MemorySegment segment) {
    var fields = mln_queried_feature.fields(segment);
    var sourceId = Optional.<String>empty();
    if ((fields & MapLibreNativeC.MLN_QUERIED_FEATURE_SOURCE_ID()) != 0) {
      sourceId = Optional.of(stringView(mln_queried_feature.source_id(segment)));
    }
    var sourceLayerId = Optional.<String>empty();
    if ((fields & MapLibreNativeC.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID()) != 0) {
      sourceLayerId = Optional.of(stringView(mln_queried_feature.source_layer_id(segment)));
    }
    var state = Optional.<JsonValue>empty();
    if ((fields & MapLibreNativeC.MLN_QUERIED_FEATURE_STATE()) != 0) {
      var stateValue = mln_queried_feature.state(segment);
      if (!MemoryUtil.isNull(stateValue)) {
        state = Optional.of(ValueStructs.jsonValue(stateValue));
      }
    }
    return new QueriedFeature(
        ValueStructs.feature(mln_queried_feature.feature(segment)), sourceId, sourceLayerId, state);
  }

  private static List<Feature> featureCollection(MemorySegment collection) {
    var count = Math.toIntExact(mln_feature_collection.feature_count(collection));
    var features =
        sizedArray(
            mln_feature_collection.features(collection), count, mln_feature.sizeof(), "features");
    var copied = new ArrayList<Feature>(count);
    for (var index = 0; index < count; index++) {
      copied.add(ValueStructs.feature(mln_feature.asSlice(features, index)));
    }
    return List.copyOf(copied);
  }
}
