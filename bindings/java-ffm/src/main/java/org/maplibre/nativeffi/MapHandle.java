package org.maplibre.nativeffi;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.util.Objects;
import org.maplibre.nativeffi.internal.HandleState;
import org.maplibre.nativeffi.internal.MemoryUtil;
import org.maplibre.nativeffi.internal.NativeAccess;
import org.maplibre.nativeffi.internal.Status;
import org.maplibre.nativeffi.internal.Structs;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;

/** Owned native map handle. Close it on the map owner thread. */
public final class MapHandle implements AutoCloseable {
  private final RuntimeHandle runtime;
  private final HandleState state;

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
              runtime.nativeHandle(), Structs.mapOptions(options, arena), outMap));
      var map = new MapHandle(runtime, outMap.get(ValueLayout.ADDRESS, 0));
      runtime.registerMap(map);
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
    }
  }

  public void requestRepaint() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_map_request_repaint(state.requireLive()));
  }

  public void requestStillImage() {
    NativeAccess.ensureLoaded();
    Status.check(MapLibreNativeC.mln_map_request_still_image(state.requireLive()));
  }

  public MapProjectionHandle createProjection() {
    return MapProjectionHandle.create(this);
  }

  @Override
  public void close() {
    NativeAccess.ensureLoaded();
    state.closeOnce(MapLibreNativeC::mln_map_destroy, () -> runtime.unregisterMap(this));
  }

  public boolean isClosed() {
    return state.isReleased();
  }

  public RuntimeHandle runtime() {
    return runtime;
  }

  MemorySegment nativeHandle() {
    return state.requireLive();
  }

  long nativeAddress() {
    return state.address();
  }
}
