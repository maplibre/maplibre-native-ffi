package org.maplibre.nativejni;

import java.nio.file.Path;
import java.util.Objects;
import java.util.Set;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.ProjectedMeters;
import org.maplibre.nativejni.internal.callback.LogCallbackState;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.log.LogCallback;
import org.maplibre.nativejni.log.LogSeverity;
import org.maplibre.nativejni.render.OpenGLContextProviderSupport;
import org.maplibre.nativejni.render.RenderBackendSupport;
import org.maplibre.nativejni.runtime.NetworkStatus;

/** Process-global entry points for the Java JNI binding. */
public final class Maplibre {
  private Maplibre() {}

  /** Loads the native library using the binding's standard lookup order. */
  public static void loadNativeLibrary() {
    NativeLibrary.ensureLoaded();
  }

  /** Loads the native library from an exact file path. */
  public static void loadNativeLibrary(Path libraryPath) {
    NativeLibrary.load(Objects.requireNonNull(libraryPath, "libraryPath"));
  }

  /** Returns the native C ABI contract version. */
  public static long cVersion() {
    NativeLibrary.ensureLoaded();
    return MaplibreNativeC.mln_c_version();
  }

  /** Returns the render backends compiled into the loaded native library. */
  public static RenderBackendSupport supportedRenderBackends() {
    NativeLibrary.ensureLoaded();
    return RenderBackendSupport.fromMask(MaplibreNativeC.mln_supported_render_backend_mask());
  }

  /** Returns the OpenGL context providers compiled into the loaded native library. */
  public static OpenGLContextProviderSupport supportedOpenGLContextProviders() {
    NativeLibrary.ensureLoaded();
    return OpenGLContextProviderSupport.fromMask(
        MaplibreNativeC.mln_opengl_supported_context_provider_mask());
  }

  /** Reads Maplibre Native's process-global network status. */
  public static NetworkStatus networkStatus() {
    NativeLibrary.ensureLoaded();
    var out = new int[1];
    Status.check(MaplibreNativeC.mln_network_status_get(out));
    return NetworkStatus.fromNative(out[0]);
  }

  /** Sets Maplibre Native's process-global network status. */
  public static void setNetworkStatus(NetworkStatus status) {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_network_status_set(
            Objects.requireNonNull(status, "status").nativeValue()));
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
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(severities, "severities");
    var mask = 0;
    for (var severity : severities) {
      mask |= Objects.requireNonNull(severity, "severity").nativeMask();
    }
    Status.check(MaplibreNativeC.mln_log_set_async_severity_mask(mask));
  }

  /** Restores the native default async log severity mask. */
  public static void restoreDefaultAsyncLogSeverities() {
    NativeLibrary.ensureLoaded();
    Status.check(
        MaplibreNativeC.mln_log_set_async_severity_mask(
            LogSeverity.INFO.nativeMask() | LogSeverity.WARNING.nativeMask()));
  }

  /** Converts a geographic coordinate to spherical Mercator projected meters. */
  public static ProjectedMeters projectedMetersForLatLng(LatLng coordinate) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(coordinate, "coordinate");
    var nativeCoordinate =
        new MaplibreNativeC.mln_lat_lng()
            .latitude(coordinate.latitude())
            .longitude(coordinate.longitude());
    var outMeters = new MaplibreNativeC.mln_projected_meters();
    Status.check(MaplibreNativeC.mln_projected_meters_for_lat_lng(nativeCoordinate, outMeters));
    return new ProjectedMeters(outMeters.northing(), outMeters.easting());
  }

  /** Converts spherical Mercator projected meters to a geographic coordinate. */
  public static LatLng latLngForProjectedMeters(ProjectedMeters meters) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(meters, "meters");
    var nativeMeters =
        new MaplibreNativeC.mln_projected_meters()
            .northing(meters.northing())
            .easting(meters.easting());
    var outCoordinate = new MaplibreNativeC.mln_lat_lng();
    Status.check(MaplibreNativeC.mln_lat_lng_for_projected_meters(nativeMeters, outCoordinate));
    return new LatLng(outCoordinate.latitude(), outCoordinate.longitude());
  }
}
