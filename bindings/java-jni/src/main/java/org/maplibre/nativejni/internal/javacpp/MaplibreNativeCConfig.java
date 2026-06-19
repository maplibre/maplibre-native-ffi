package org.maplibre.nativejni.internal.javacpp;

import org.bytedeco.javacpp.annotation.Platform;
import org.bytedeco.javacpp.annotation.Properties;
import org.bytedeco.javacpp.tools.Info;
import org.bytedeco.javacpp.tools.InfoMap;
import org.bytedeco.javacpp.tools.InfoMapper;

@Properties(
    value =
        @Platform(
            includepath = {"../../include"},
            cinclude = {
              "maplibre_native_c/base.h",
              "maplibre_native_c/diagnostics.h",
              "maplibre_native_c/logging.h",
              "maplibre_native_c/android.h",
              "maplibre_native_c/runtime.h",
              "maplibre_native_c/camera.h",
              "maplibre_native_c/render_target.h",
              "maplibre_native_c/texture.h",
              "maplibre_native_c/surface.h",
              "maplibre_native_c/render_session.h",
              "maplibre_native_c/projection.h",
              "maplibre_native_c/query.h",
              "maplibre_native_c/style.h",
              "maplibre_native_c/map.h"
            },
            link = {"maplibre-native-c"}),
    target = "org.maplibre.nativejni.internal.javacpp.MaplibreNativeC")
public class MaplibreNativeCConfig implements InfoMapper {
  @Override
  public void map(InfoMap infoMap) {
    infoMap.put(new Info("MLN_API", "MLN_NOEXCEPT").cppTypes().annotations());
    infoMap.put(
        new Info("__cplusplus", "MAPLIBRE_NATIVE_C_BASE_H", "MAPLIBRE_NATIVE_C_H").define(false));
  }
}
