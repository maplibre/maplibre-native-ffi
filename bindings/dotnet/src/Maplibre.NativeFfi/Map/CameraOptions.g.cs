// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Camera fields used by snapshots and camera updates.
/// </summary>
/// <remarks>
/// See <c>mln_camera_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record CameraOptions
{
    public LatLng? Center { get; init; }
    public double? CenterAltitude { get; init; }
    public EdgeInsets? Padding { get; init; }

    /// <summary>
    /// Optional screen-space focal point in logical map pixels.
    /// </summary>
    public ScreenPoint? Anchor { get; init; }
    public double? Zoom { get; init; }
    public double? Bearing { get; init; }
    public double? Pitch { get; init; }
    public double? Roll { get; init; }
    public double? FieldOfView { get; init; }
}
