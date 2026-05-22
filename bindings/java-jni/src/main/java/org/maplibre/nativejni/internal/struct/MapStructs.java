package org.maplibre.nativejni.internal.struct;

import java.util.Objects;
import org.maplibre.nativejni.camera.AnimationOptions;
import org.maplibre.nativejni.camera.BoundOptions;
import org.maplibre.nativejni.camera.CameraFitOptions;
import org.maplibre.nativejni.camera.CameraOptions;
import org.maplibre.nativejni.camera.FreeCameraOptions;
import org.maplibre.nativejni.map.MapMode;
import org.maplibre.nativejni.map.MapOptions;

/** Internal materializers for map, camera, bounds, and projection descriptors. */
public final class MapStructs {
  private MapStructs() {}

  public record MapOptionsValue(int width, int height, double scaleFactor, int mapMode) {}

  public record CameraOptionsValue(
      boolean hasCenter,
      CoreStructs.LatLngValue center,
      boolean hasCenterAltitude,
      double centerAltitude,
      boolean hasPadding,
      CoreStructs.EdgeInsetsValue padding,
      boolean hasAnchor,
      CoreStructs.ScreenPointValue anchor,
      boolean hasZoom,
      double zoom,
      boolean hasBearing,
      double bearing,
      boolean hasPitch,
      double pitch,
      boolean hasRoll,
      double roll,
      boolean hasFieldOfView,
      double fieldOfView) {}

  public record AnimationOptionsValue(
      boolean hasDurationMs,
      double durationMs,
      boolean hasVelocity,
      double velocity,
      boolean hasMinZoom,
      double minZoom,
      boolean hasEasing,
      CoreStructs.UnitBezierValue easing) {}

  public record CameraFitOptionsValue(
      boolean hasPadding,
      CoreStructs.EdgeInsetsValue padding,
      boolean hasBearing,
      double bearing,
      boolean hasPitch,
      double pitch) {}

  public record BoundOptionsValue(
      boolean hasBounds,
      CoreStructs.LatLngBoundsValue bounds,
      boolean hasMinZoom,
      double minZoom,
      boolean hasMaxZoom,
      double maxZoom,
      boolean hasMinPitch,
      double minPitch,
      boolean hasMaxPitch,
      double maxPitch) {}

  public record FreeCameraOptionsValue(
      boolean hasPosition,
      CoreStructs.Vec3Value position,
      boolean hasOrientation,
      CoreStructs.QuaternionValue orientation) {}

  public static MapOptionsValue mapOptions(MapOptions options) {
    Objects.requireNonNull(options, "options");
    return new MapOptionsValue(
        options.width() == null ? 512 : options.width(),
        options.height() == null ? 512 : options.height(),
        options.scaleFactor() == null ? 1.0 : options.scaleFactor(),
        options.mapMode() == null
            ? MapMode.CONTINUOUS.nativeValue()
            : options.mapMode().nativeValue());
  }

  public static CameraOptionsValue cameraOptions(CameraOptions options) {
    Objects.requireNonNull(options, "options");
    return new CameraOptionsValue(
        options.hasCenter(),
        options.hasCenter() ? CoreStructs.latLng(options.center()) : null,
        options.hasCenterAltitude(),
        options.hasCenterAltitude() ? options.centerAltitude() : 0,
        options.hasPadding(),
        options.hasPadding() ? CoreStructs.edgeInsets(options.padding()) : null,
        options.hasAnchor(),
        options.hasAnchor() ? CoreStructs.screenPoint(options.anchor()) : null,
        options.hasZoom(),
        options.hasZoom() ? options.zoom() : 0,
        options.hasBearing(),
        options.hasBearing() ? options.bearing() : 0,
        options.hasPitch(),
        options.hasPitch() ? options.pitch() : 0,
        options.hasRoll(),
        options.hasRoll() ? options.roll() : 0,
        options.hasFieldOfView(),
        options.hasFieldOfView() ? options.fieldOfView() : 0);
  }

  public static AnimationOptionsValue animationOptions(AnimationOptions options) {
    Objects.requireNonNull(options, "options");
    return new AnimationOptionsValue(
        options.hasDurationMs(),
        options.hasDurationMs() ? options.durationMs() : 0,
        options.hasVelocity(),
        options.hasVelocity() ? options.velocity() : 0,
        options.hasMinZoom(),
        options.hasMinZoom() ? options.minZoom() : 0,
        options.hasEasing(),
        options.hasEasing() ? CoreStructs.unitBezier(options.easing()) : null);
  }

  public static CameraFitOptionsValue cameraFitOptions(CameraFitOptions options) {
    Objects.requireNonNull(options, "options");
    return new CameraFitOptionsValue(
        options.hasPadding(),
        options.hasPadding() ? CoreStructs.edgeInsets(options.padding()) : null,
        options.hasBearing(),
        options.hasBearing() ? options.bearing() : 0,
        options.hasPitch(),
        options.hasPitch() ? options.pitch() : 0);
  }

  public static BoundOptionsValue boundOptions(BoundOptions options) {
    Objects.requireNonNull(options, "options");
    return new BoundOptionsValue(
        options.hasBounds(),
        options.hasBounds() ? CoreStructs.latLngBounds(options.bounds()) : null,
        options.hasMinZoom(),
        options.hasMinZoom() ? options.minZoom() : 0,
        options.hasMaxZoom(),
        options.hasMaxZoom() ? options.maxZoom() : 0,
        options.hasMinPitch(),
        options.hasMinPitch() ? options.minPitch() : 0,
        options.hasMaxPitch(),
        options.hasMaxPitch() ? options.maxPitch() : 0);
  }

  public static FreeCameraOptionsValue freeCameraOptions(FreeCameraOptions options) {
    Objects.requireNonNull(options, "options");
    return new FreeCameraOptionsValue(
        options.hasPosition(),
        options.hasPosition() ? CoreStructs.vec3(options.position()) : null,
        options.hasOrientation(),
        options.hasOrientation() ? CoreStructs.quaternion(options.orientation()) : null);
  }
}
