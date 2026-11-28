// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How the map orients and constrains its view.
/// </summary>
/// <remarks>
/// See <c>mln_map_viewport_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record MapViewportOptions
{
    public NorthOrientation? NorthOrientation { get; init; }
    public ConstrainMode? ConstrainMode { get; init; }
    public ViewportMode? ViewportMode { get; init; }
    public EdgeInsets? FrustumOffset { get; init; }

    /// <summary>
    /// Whether the camera's center stands on the terrain, in place of the
    /// center altitude it's given, the style's included. On by default.
    /// </summary>
    public bool? CenterClampedToGround { get; init; }

    /// <summary>
    /// Whether drags and zooms on a globe turn the bearing to keep the planet's
    /// orientation, which lets a drag carry the camera over a pole. Off by
    /// default.
    /// </summary>
    public bool? GlobeFreeBearing { get; init; }
}
