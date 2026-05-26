package org.maplibre.nativejni.internal.bridge;

import org.bytedeco.javacpp.PointerPointer;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.JavaCppValues;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;

/** JavaCPP-backed declarations for the RenderSessionNative C API coverage group. */
public final class RenderSessionNative {
  private RenderSessionNative() {}

  public static int mln_render_session_resize(
      long session, int width, int height, double scaleFactor) {
    if (width < 0 || height < 0) {
      BaseNative.setThreadDiagnostic("width and height must be non-negative");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    if (!Double.isFinite(scaleFactor)
        || scaleFactor <= 0
        || width * scaleFactor > Integer.MAX_VALUE
        || height * scaleFactor > Integer.MAX_VALUE) {
      BaseNative.setThreadDiagnostic("scaled width and height must fit Java int range");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    return MaplibreNativeC.mln_render_session_resize(
        JavaCppSupport.renderSession(session), width, height, scaleFactor);
  }

  public static int mln_render_session_render_update(long session) {
    return MaplibreNativeC.mln_render_session_render_update(JavaCppSupport.renderSession(session));
  }

  public static int mln_render_session_detach(long session) {
    return MaplibreNativeC.mln_render_session_detach(JavaCppSupport.renderSession(session));
  }

  public static int mln_render_session_destroy(long session) {
    return MaplibreNativeC.mln_render_session_destroy(JavaCppSupport.renderSession(session));
  }

  public static int mln_render_session_reduce_memory_use(long session) {
    return MaplibreNativeC.mln_render_session_reduce_memory_use(
        JavaCppSupport.renderSession(session));
  }

  public static int mln_render_session_clear_data(long session) {
    return MaplibreNativeC.mln_render_session_clear_data(JavaCppSupport.renderSession(session));
  }

  public static int mln_render_session_dump_debug_logs(long session) {
    return MaplibreNativeC.mln_render_session_dump_debug_logs(
        JavaCppSupport.renderSession(session));
  }

  public static int mln_render_session_set_feature_state(
      long session,
      org.maplibre.nativejni.query.FeatureStateSelector selector,
      org.maplibre.nativejni.json.JsonValue value) {
    try (var nativeSelector = selector(selector);
        var nativeValue = JavaCppValues.json(value)) {
      return MaplibreNativeC.mln_render_session_set_feature_state(
          JavaCppSupport.renderSession(session), nativeSelector.selector(), nativeValue.value());
    }
  }

  public static int mln_render_session_get_feature_state(
      long session, org.maplibre.nativejni.query.FeatureStateSelector selector, Object[] outState) {
    try (var nativeSelector = selector(selector)) {
      var outSnapshot = new PointerPointer<MaplibreNativeC.mln_json_snapshot>(1);
      var status =
          MaplibreNativeC.mln_render_session_get_feature_state(
              JavaCppSupport.renderSession(session), nativeSelector.selector(), outSnapshot);
      if (status != MaplibreNativeC.MLN_STATUS_OK) {
        return status;
      }
      var snapshotAddress =
          JavaCppSupport.outAddress(outSnapshot, MaplibreNativeC.mln_json_snapshot.class);
      var snapshot = new MaplibreNativeC.mln_json_snapshot(JavaCppSupport.pointer(snapshotAddress));
      try {
        var outValue = new PointerPointer<MaplibreNativeC.mln_json_value>(1);
        status = MaplibreNativeC.mln_json_snapshot_get(snapshot, outValue);
        if (status == MaplibreNativeC.MLN_STATUS_OK) {
          var valueAddress =
              JavaCppSupport.outAddress(outValue, MaplibreNativeC.mln_json_value.class);
          outState[0] =
              JavaCppValues.jsonValue(
                  new MaplibreNativeC.mln_json_value(JavaCppSupport.pointer(valueAddress)));
        }
        return status;
      } finally {
        MaplibreNativeC.mln_json_snapshot_destroy(snapshot);
      }
    }
  }

  public static int mln_render_session_remove_feature_state(
      long session, org.maplibre.nativejni.query.FeatureStateSelector selector) {
    try (var nativeSelector = selector(selector)) {
      return MaplibreNativeC.mln_render_session_remove_feature_state(
          JavaCppSupport.renderSession(session), nativeSelector.selector());
    }
  }

  private static SelectorScope selector(org.maplibre.nativejni.query.FeatureStateSelector value) {
    return new SelectorScope(value);
  }

  private static final class SelectorScope implements AutoCloseable {
    private final JavaCppValues.StringViewScope sourceId;
    private final JavaCppValues.StringViewScope sourceLayerId;
    private final JavaCppValues.StringViewScope featureId;
    private final JavaCppValues.StringViewScope stateKey;
    private final MaplibreNativeC.mln_feature_state_selector selector;

    SelectorScope(org.maplibre.nativejni.query.FeatureStateSelector value) {
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

    MaplibreNativeC.mln_feature_state_selector selector() {
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
}
