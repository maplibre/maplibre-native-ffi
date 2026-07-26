/// Camera descriptor and map camera operation types.
library;

import '../geo/geo.dart';

export '../geo/geo.dart'
    show EdgeInsets, LatLng, LatLngBounds, Quaternion, ScreenPoint, Vec3;

/// Cubic easing curve for animated camera transitions.
final class UnitBezier {
  /// Creates a cubic unit bezier.
  const UnitBezier(this.x1, this.y1, this.x2, this.y2);

  /// First control point x.
  final double x1;

  /// First control point y.
  final double y1;

  /// Second control point x.
  final double x2;

  /// Second control point y.
  final double y2;

  @override
  bool operator ==(Object other) =>
      other is UnitBezier &&
      other.x1 == x1 &&
      other.y1 == y1 &&
      other.x2 == x2 &&
      other.y2 == y2;

  @override
  int get hashCode => Object.hash(x1, y1, x2, y2);
}

/// Camera fields used for snapshots and camera commands.
final class CameraOptions {
  /// Creates camera options.
  const CameraOptions({
    this.center,
    this.zoom,
    this.bearing,
    this.pitch,
    this.centerAltitude,
    this.padding,
    this.anchor,
    this.roll,
    this.fieldOfView,
  });

  /// Optional center coordinate.
  final LatLng? center;

  /// Optional zoom level.
  final double? zoom;

  /// Optional bearing in degrees.
  final double? bearing;

  /// Optional pitch in degrees.
  final double? pitch;

  /// Optional center altitude.
  final double? centerAltitude;

  /// Optional viewport padding.
  final EdgeInsets? padding;

  /// Optional screen anchor.
  final ScreenPoint? anchor;

  /// Optional roll in degrees.
  final double? roll;

  /// Optional field of view.
  final double? fieldOfView;

  @override
  bool operator ==(Object other) =>
      other is CameraOptions &&
      other.center == center &&
      other.zoom == zoom &&
      other.bearing == bearing &&
      other.pitch == pitch &&
      other.centerAltitude == centerAltitude &&
      other.padding == padding &&
      other.anchor == anchor &&
      other.roll == roll &&
      other.fieldOfView == fieldOfView;

  @override
  int get hashCode => Object.hashAll([
    center,
    zoom,
    bearing,
    pitch,
    centerAltitude,
    padding,
    anchor,
    roll,
    fieldOfView,
  ]);
}

/// Optional animation controls for camera transitions.
final class AnimationOptions {
  /// Creates animation options.
  const AnimationOptions({
    this.durationMs,
    this.velocity,
    this.minZoom,
    this.easing,
  });

  /// Optional duration in milliseconds.
  final double? durationMs;

  /// Optional average fly-to velocity in screenfuls per second.
  final double? velocity;

  /// Optional peak zoom for fly-to transitions.
  final double? minZoom;

  /// Optional easing curve.
  final UnitBezier? easing;

  @override
  bool operator ==(Object other) =>
      other is AnimationOptions &&
      other.durationMs == durationMs &&
      other.velocity == velocity &&
      other.minZoom == minZoom &&
      other.easing == easing;

  @override
  int get hashCode => Object.hash(durationMs, velocity, minZoom, easing);
}

/// Optional fitting controls for camera-for-viewport queries.
final class CameraFitOptions {
  /// Creates camera fit options.
  const CameraFitOptions({this.padding, this.bearing, this.pitch});

  /// Optional viewport padding.
  final EdgeInsets? padding;

  /// Optional bearing in degrees.
  final double? bearing;

  /// Optional pitch in degrees.
  final double? pitch;

  @override
  bool operator ==(Object other) =>
      other is CameraFitOptions &&
      other.padding == padding &&
      other.bearing == bearing &&
      other.pitch == pitch;

  @override
  int get hashCode => Object.hash(padding, bearing, pitch);
}

/// Map north orientation values.
final class NorthOrientation {
  const NorthOrientation._(this.rawValue, this.name);

  /// North points up.
  static const up = NorthOrientation._(0, 'up');

  /// North points right.
  static const right = NorthOrientation._(1, 'right');

  /// North points down.
  static const down = NorthOrientation._(2, 'down');

  /// North points left.
  static const left = NorthOrientation._(3, 'left');

  /// Creates a north orientation from a raw native value.
  factory NorthOrientation.fromRawValue(int rawValue) => switch (rawValue) {
    0 => up,
    1 => right,
    2 => down,
    3 => left,
    _ => NorthOrientation._(rawValue, 'unknown($rawValue)'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;

  @override
  bool operator ==(Object other) =>
      other is NorthOrientation && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// Map camera constraint mode.
final class ConstrainMode {
  const ConstrainMode._(this.rawValue, this.name);

  /// No constraints.
  static const none = ConstrainMode._(0, 'none');

  /// Height-only constraints.
  static const heightOnly = ConstrainMode._(1, 'heightOnly');

  /// Width-and-height constraints.
  static const widthAndHeight = ConstrainMode._(2, 'widthAndHeight');

  /// Screen constraints.
  static const screen = ConstrainMode._(3, 'screen');

  /// Creates a constrain mode from a raw native value.
  factory ConstrainMode.fromRawValue(int rawValue) => switch (rawValue) {
    0 => none,
    1 => heightOnly,
    2 => widthAndHeight,
    3 => screen,
    _ => ConstrainMode._(rawValue, 'unknown($rawValue)'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;

  @override
  bool operator ==(Object other) =>
      other is ConstrainMode && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// Viewport orientation mode.
final class ViewportMode {
  const ViewportMode._(this.rawValue, this.name);

  /// Default viewport orientation.
  static const defaultMode = ViewportMode._(0, 'default');

  /// Flipped-Y viewport orientation.
  static const flippedY = ViewportMode._(1, 'flippedY');

  /// Creates a viewport mode from a raw native value.
  factory ViewportMode.fromRawValue(int rawValue) => switch (rawValue) {
    0 => defaultMode,
    1 => flippedY,
    _ => ViewportMode._(rawValue, 'unknown($rawValue)'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;

  @override
  bool operator ==(Object other) =>
      other is ViewportMode && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// Tile LOD algorithm.
final class TileLodMode {
  const TileLodMode._(this.rawValue, this.name);

  /// Native default algorithm.
  static const defaultMode = TileLodMode._(0, 'default');

  /// Distance-based LOD algorithm.
  static const distance = TileLodMode._(1, 'distance');

  /// Creates a tile LOD mode from a raw native value.
  factory TileLodMode.fromRawValue(int rawValue) => switch (rawValue) {
    0 => defaultMode,
    1 => distance,
    _ => TileLodMode._(rawValue, 'unknown($rawValue)'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;

  @override
  bool operator ==(Object other) =>
      other is TileLodMode && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// Optional live map viewport and render-transform controls.
final class MapViewportOptions {
  /// Creates viewport options.
  const MapViewportOptions({
    this.northOrientation,
    this.constrainMode,
    this.viewportMode,
    this.frustumOffset,
  });

  /// Optional north orientation.
  final NorthOrientation? northOrientation;

  /// Optional constraint mode.
  final ConstrainMode? constrainMode;

  /// Optional viewport mode.
  final ViewportMode? viewportMode;

  /// Optional frustum offset.
  final EdgeInsets? frustumOffset;

  @override
  bool operator ==(Object other) =>
      other is MapViewportOptions &&
      other.northOrientation == northOrientation &&
      other.constrainMode == constrainMode &&
      other.viewportMode == viewportMode &&
      other.frustumOffset == frustumOffset;

  @override
  int get hashCode =>
      Object.hash(northOrientation, constrainMode, viewportMode, frustumOffset);
}

/// Optional tile prefetch and LOD tuning controls.
final class MapTileOptions {
  /// Creates tile options.
  const MapTileOptions({
    this.prefetchZoomDelta,
    this.lodMinRadius,
    this.lodScale,
    this.lodPitchThreshold,
    this.lodZoomShift,
    this.lodMode,
  });

  /// Optional prefetch zoom delta.
  final int? prefetchZoomDelta;

  /// Optional LOD minimum radius.
  final double? lodMinRadius;

  /// Optional LOD scale.
  final double? lodScale;

  /// Optional LOD pitch threshold.
  final double? lodPitchThreshold;

  /// Optional LOD zoom shift.
  final double? lodZoomShift;

  /// Optional LOD mode.
  final TileLodMode? lodMode;

  @override
  bool operator ==(Object other) =>
      other is MapTileOptions &&
      other.prefetchZoomDelta == prefetchZoomDelta &&
      other.lodMinRadius == lodMinRadius &&
      other.lodScale == lodScale &&
      other.lodPitchThreshold == lodPitchThreshold &&
      other.lodZoomShift == lodZoomShift &&
      other.lodMode == lodMode;

  @override
  int get hashCode => Object.hash(
    prefetchZoomDelta,
    lodMinRadius,
    lodScale,
    lodPitchThreshold,
    lodZoomShift,
    lodMode,
  );
}

/// Optional map camera constraint fields.
final class BoundOptions {
  /// Creates bound options.
  const BoundOptions({
    this.bounds,
    this.minZoom,
    this.maxZoom,
    this.minPitch,
    this.maxPitch,
  });

  /// Optional coordinate bounds.
  final LatLngBounds? bounds;

  /// Optional minimum zoom.
  final double? minZoom;

  /// Optional maximum zoom.
  final double? maxZoom;

  /// Optional minimum pitch.
  final double? minPitch;

  /// Optional maximum pitch.
  final double? maxPitch;

  @override
  bool operator ==(Object other) =>
      other is BoundOptions &&
      other.bounds == bounds &&
      other.minZoom == minZoom &&
      other.maxZoom == maxZoom &&
      other.minPitch == minPitch &&
      other.maxPitch == maxPitch;

  @override
  int get hashCode => Object.hash(bounds, minZoom, maxZoom, minPitch, maxPitch);
}

/// Free camera position and orientation in MapLibre Native camera space.
final class FreeCameraOptions {
  /// Creates free camera options.
  const FreeCameraOptions({this.position, this.orientation});

  /// Optional position.
  final Vec3? position;

  /// Optional orientation.
  final Quaternion? orientation;

  @override
  bool operator ==(Object other) =>
      other is FreeCameraOptions &&
      other.position == position &&
      other.orientation == orientation;

  @override
  int get hashCode => Object.hash(position, orientation);
}

/// MapLibre axonometric rendering options used for snapshots and commands.
final class ProjectionModeOptions {
  /// Creates projection mode options.
  const ProjectionModeOptions({this.axonometric, this.xSkew, this.ySkew});

  /// Optional axonometric mode switch.
  final bool? axonometric;

  /// Optional native x-skew factor.
  final double? xSkew;

  /// Optional native y-skew factor.
  final double? ySkew;

  @override
  bool operator ==(Object other) =>
      other is ProjectionModeOptions &&
      other.axonometric == axonometric &&
      other.xSkew == xSkew &&
      other.ySkew == ySkew;

  @override
  int get hashCode => Object.hash(axonometric, xSkew, ySkew);
}
