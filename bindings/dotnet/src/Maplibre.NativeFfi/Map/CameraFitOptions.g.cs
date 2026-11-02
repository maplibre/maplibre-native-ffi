// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for finding the camera that fits an area.
/// </summary>
/// <remarks>
/// See <c>mln_camera_fit_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record CameraFitOptions
{
    /// <summary>
    /// Space to leave between the area and each edge of the map.
    /// </summary>
    public EdgeInsets? Padding { get; init; }
    public double? Bearing { get; init; }
    public double? Pitch { get; init; }
}
