package org.maplibre.nativeffi;

import java.lang.foreign.Arena;
import java.lang.foreign.ValueLayout;
import java.nio.file.Path;
import java.util.EnumSet;
import java.util.Objects;
import java.util.Set;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.geo.ProjectedMeters;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_lat_lng;
import org.maplibre.nativeffi.internal.c.mln_projected_meters;
import org.maplibre.nativeffi.internal.callback.LogCallbackState;
import org.maplibre.nativeffi.internal.loader.NativeAccess;
import org.maplibre.nativeffi.internal.status.Status;
import org.maplibre.nativeffi.internal.struct.CoreStructs;
import org.maplibre.nativeffi.log.LogCallback;
import org.maplibre.nativeffi.log.LogSeverity;
import org.maplibre.nativeffi.render.OpenGLContextProvider;
import org.maplibre.nativeffi.render.RenderBackend;
import org.maplibre.nativeffi.runtime.NetworkStatus;

/** Process-global entry points for the Java FFM binding. */
public final class Maplibre {
  private Maplibre() {}

  /** Loads the native library using the binding's standard lookup order. */
  public static void loadNativeLibrary() {
    NativeAccess.ensureLoaded();
  }

  /** Loads the native library from an exact file path. */
  public static void loadNativeLibrary(Path libraryPath) {
    NativeAccess.load(Objects.requireNonNull(libraryPath, "libraryPath"));
  }

  /** Returns the native C ABI contract version. */
  public static long cVersion() {
    NativeAccess.ensureLoaded();
    return Integer.toUnsignedLong(MapLibreNativeC.mln_c_version());
  }

  /** Returns the render backends compiled into the loaded native library. */
  public static EnumSet<RenderBackend> supportedRenderBackends() {
    NativeAccess.ensureLoaded();
    return RenderBackend.fromMask(MapLibreNativeC.mln_supported_render_backend_mask());
  }

  /** Returns the OpenGL context providers compiled into the loaded native library. */
  public static EnumSet<OpenGLContextProvider> supportedOpenGLContextProviders() {
    NativeAccess.ensureLoaded();
    return OpenGLContextProvider.fromMask(
        MapLibreNativeC.mln_opengl_supported_context_provider_mask());
  }

  /** Reads Maplibre Native's process-global network status. */
  public static NetworkStatus networkStatus() {
    NativeAccess.ensureLoaded();
    try (var arena = Arena.ofConfined()) {
      var out = arena.allocate(ValueLayout.JAVA_INT);
      Status.check(MapLibreNativeC.mln_network_status_get(out));
      return NetworkStatus.fromNative(out.get(ValueLayout.JAVA_INT, 0));
    }
  }

  /** Sets Maplibre Native's process-global network status. */
  public static void setNetworkStatus(NetworkStatus status) {
    NativeAccess.ensureLoaded();
    Status.check(
        MapLibreNativeC.mln_network_status_set(Objects.requireNonNull(status).nativeValue()));
  }

  /**
   * Installs or replaces the process-global native log callback.
   *
   * <p>See {@link LogCallback} for callback threading and exception-containment rules.
   */
  public static void setLogCallback(LogCallback callback) {
    LogCallbackState.set(Objects.requireNonNull(callback, "callback"));
  }

  /** Clears the process-global native log callback. */
  public static void clearLogCallback() {
    LogCallbackState.clear();
  }

  /** Configures severities that native logging may dispatch asynchronously. */
  public static void setAsyncLogSeverities(Set<LogSeverity> severities) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(severities, "severities");
    var mask = 0;
    for (var severity : severities) {
      mask |= Objects.requireNonNull(severity, "severity").nativeMask();
    }
    Status.check(MapLibreNativeC.mln_log_set_async_severity_mask(mask));
  }

  /** Restores the native default async log severity mask. */
  public static void restoreDefaultAsyncLogSeverities() {
    NativeAccess.ensureLoaded();
    Status.check(
        MapLibreNativeC.mln_log_set_async_severity_mask(
            MapLibreNativeC.MLN_LOG_SEVERITY_MASK_DEFAULT()));
  }

  /** Converts a geographic coordinate to spherical Mercator projected meters. */
  public static ProjectedMeters projectedMetersForLatLng(LatLng coordinate) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(coordinate, "coordinate");
    try (var arena = Arena.ofConfined()) {
      var out = mln_projected_meters.allocate(arena);
      Status.check(
          MapLibreNativeC.mln_projected_meters_for_lat_lng(
              CoreStructs.latLng(coordinate, arena), out));
      return CoreStructs.projectedMeters(out);
    }
  }

  /** Converts spherical Mercator projected meters to a geographic coordinate. */
  public static LatLng latLngForProjectedMeters(ProjectedMeters meters) {
    NativeAccess.ensureLoaded();
    Objects.requireNonNull(meters, "meters");
    try (var arena = Arena.ofConfined()) {
      var out = mln_lat_lng.allocate(arena);
      Status.check(
          MapLibreNativeC.mln_lat_lng_for_projected_meters(
              CoreStructs.projectedMeters(meters, arena), out));
      return CoreStructs.latLng(out);
    }
  }
}
