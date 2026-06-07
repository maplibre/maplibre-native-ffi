using Maplibre.Native.Geo;

namespace Maplibre.Native.Camera;

/// <summary>Mutable camera descriptor used for camera snapshots and commands.</summary>
public sealed class CameraOptions
{
    public LatLng? Center { get; set; }
    public double? CenterAltitude { get; set; }
    public EdgeInsets? Padding { get; set; }
    public ScreenPoint? Anchor { get; set; }
    public double? Zoom { get; set; }
    public double? Bearing { get; set; }
    public double? Pitch { get; set; }
    public double? Roll { get; set; }
    public double? FieldOfView { get; set; }
}

/// <summary>Camera animation descriptor.</summary>
public sealed class AnimationOptions
{
    public double? Duration { get; set; }
    public UnitBezier? Easing { get; set; }
    public double? MinimumZoom { get; set; }
    public double? Velocity { get; set; }
}

/// <summary>Camera fitting descriptor.</summary>
public sealed class CameraFitOptions
{
    public EdgeInsets? Padding { get; set; }
    public double? Bearing { get; set; }
    public double? Pitch { get; set; }
}

/// <summary>Camera bound constraint descriptor.</summary>
public sealed class BoundOptions
{
    public LatLngBounds? Bounds { get; set; }
    public double? MinimumZoom { get; set; }
    public double? MaximumZoom { get; set; }
    public double? MinimumPitch { get; set; }
    public double? MaximumPitch { get; set; }
}

/// <summary>Free camera descriptor.</summary>
public sealed class FreeCameraOptions
{
    public Vec3? Position { get; set; }
    public Quaternion? Orientation { get; set; }
}
