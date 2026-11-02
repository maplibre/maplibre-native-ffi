// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A camera, with only the members that <c>fields</c> selects.
/// </summary>
/// <remarks>
/// See <c>mln_camera_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record CameraOptions
{
    public LatLng? Center { get; init; }

    /// <summary>
    /// The altitude of the center, in meters above sea level.
    /// </summary>
    public double? CenterAltitude { get; init; }

    /// <summary>
    /// Insets from the map's edges. The camera centers within the rest.
    /// </summary>
    public EdgeInsets? Padding { get; init; }

    /// <summary>
    /// The screen point that a change to the zoom, bearing, or pitch keeps in
    /// place. A <c>center</c> overrides it.
    /// </summary>
    public ScreenPoint? Anchor { get; init; }
    public double? Zoom { get; init; }

    /// <summary>
    /// The rotation from north, in degrees.
    /// </summary>
    public double? Bearing { get; init; }

    /// <summary>
    /// The tilt toward the horizon, in degrees. 0 looks straight down.
    /// </summary>
    public double? Pitch { get; init; }

    /// <summary>
    /// The roll, in degrees.
    /// </summary>
    public double? Roll { get; init; }

    /// <summary>
    /// The vertical field of view, in degrees.
    /// </summary>
    public double? FieldOfView { get; init; }
}
