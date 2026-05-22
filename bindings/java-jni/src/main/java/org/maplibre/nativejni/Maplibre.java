package org.maplibre.nativejni;

import java.nio.file.Path;
import java.util.EnumSet;
import java.util.Objects;
import java.util.Set;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.ProjectedMeters;
import org.maplibre.nativejni.internal.bridge.NativeBridge;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.log.LogCallback;
import org.maplibre.nativejni.log.LogSeverity;
import org.maplibre.nativejni.render.RenderBackend;
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
    return NativeBridge.cVersion();
  }

  /** Returns the render backends compiled into the loaded native library. */
  public static EnumSet<RenderBackend> supportedRenderBackends() {
    NativeLibrary.ensureLoaded();
    return RenderBackend.fromMask(NativeBridge.supportedRenderBackendMask());
  }

  /** Reads Maplibre Native's process-global network status. */
  public static NetworkStatus networkStatus() {
    NativeLibrary.ensureLoaded();
    var out = new int[1];
    Status.check(NativeBridge.networkStatusGet(out));
    return NetworkStatus.fromNative(out[0]);
  }

  /** Sets Maplibre Native's process-global network status. */
  public static void setNetworkStatus(NetworkStatus status) {
    NativeLibrary.ensureLoaded();
    Status.check(
        NativeBridge.networkStatusSet(Objects.requireNonNull(status, "status").nativeValue()));
  }

  /**
   * Installs or replaces the process-global native log callback.
   *
   * <p>See {@link LogCallback} for callback threading and exception-containment rules.
   */
  public static void setLogCallback(LogCallback callback) {
    Objects.requireNonNull(callback, "callback");
    throw new UnsupportedOperationException(
        "setLogCallback is not implemented by the JNI bridge yet");
  }

  /** Clears the process-global native log callback. */
  public static void clearLogCallback() {
    throw new UnsupportedOperationException(
        "clearLogCallback is not implemented by the JNI bridge yet");
  }

  /** Configures severities that native logging may dispatch asynchronously. */
  public static void setAsyncLogSeverities(Set<LogSeverity> severities) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(severities, "severities");
    for (var severity : severities) {
      Objects.requireNonNull(severity, "severity");
    }
    throw new UnsupportedOperationException(
        "setAsyncLogSeverities is not implemented by the JNI bridge yet");
  }

  /** Restores the native default async log severity mask. */
  public static void restoreDefaultAsyncLogSeverities() {
    NativeLibrary.ensureLoaded();
    throw new UnsupportedOperationException(
        "restoreDefaultAsyncLogSeverities is not implemented by the JNI bridge yet");
  }

  /** Converts a geographic coordinate to spherical Mercator projected meters. */
  public static ProjectedMeters projectedMetersForLatLng(LatLng coordinate) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(coordinate, "coordinate");
    throw new UnsupportedOperationException(
        "projectedMetersForLatLng is not implemented by the JNI bridge yet");
  }

  /** Converts spherical Mercator projected meters to a geographic coordinate. */
  public static LatLng latLngForProjectedMeters(ProjectedMeters meters) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(meters, "meters");
    throw new UnsupportedOperationException(
        "latLngForProjectedMeters is not implemented by the JNI bridge yet");
  }
}
