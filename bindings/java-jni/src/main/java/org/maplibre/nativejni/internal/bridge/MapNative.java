package org.maplibre.nativejni.internal.bridge;

import org.bytedeco.javacpp.PointerPointer;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;

/** JavaCPP-backed declarations for the MapNative C API coverage group. */
public final class MapNative {
  private MapNative() {}

  public static int mln_map_create(
      long runtime, int width, int height, double scaleFactor, int mapMode, long[] outMap) {
    if (outMap == null || outMap.length == 0) {
      BaseNative.setThreadDiagnostic("out map must not be null");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    if (width < 0 || height < 0) {
      BaseNative.setThreadDiagnostic("width and height must be non-negative");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    var options = MaplibreNativeC.mln_map_options_default();
    options.width(width);
    options.height(height);
    options.scale_factor(scaleFactor);
    options.map_mode(mapMode);
    var out = new PointerPointer<MaplibreNativeC.mln_map>(1);
    var status = MaplibreNativeC.mln_map_create(JavaCppSupport.runtime(runtime), options, out);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outMap[0] = JavaCppSupport.outAddress(out, MaplibreNativeC.mln_map.class);
    }
    return status;
  }

  public static int mln_map_request_repaint(long map) {
    return MaplibreNativeC.mln_map_request_repaint(JavaCppSupport.map(map));
  }

  public static int mln_map_request_still_image(long map) {
    return MaplibreNativeC.mln_map_request_still_image(JavaCppSupport.map(map));
  }

  public static int mln_map_destroy(long map) {
    return MaplibreNativeC.mln_map_destroy(JavaCppSupport.map(map));
  }

  public static int mln_map_set_style_url(long map, String url) {
    if (url != null && url.indexOf('\0') >= 0) {
      BaseNative.setThreadDiagnostic("style URL contains embedded NUL");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    return MaplibreNativeC.mln_map_set_style_url(JavaCppSupport.map(map), url);
  }

  public static int mln_map_set_style_json(long map, String json) {
    if (json != null && json.indexOf('\0') >= 0) {
      BaseNative.setThreadDiagnostic("style JSON contains embedded NUL");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    return MaplibreNativeC.mln_map_set_style_json(JavaCppSupport.map(map), json);
  }
}
