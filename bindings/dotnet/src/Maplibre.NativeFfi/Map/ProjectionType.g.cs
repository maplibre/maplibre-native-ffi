// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// A projection that a style's
/// [projection](https://maplibre.org/maplibre-style-spec/projection/) names.
/// </summary>
/// <remarks>
/// See <c>mln_projection_type</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum ProjectionType : uint
{
    Mercator = 0,

    /// <summary>
    /// Draws the map as a globe.
    /// </summary>
    VerticalPerspective = 1,

    /// <summary>
    /// Draws the map as a globe at low zooms, and hands over to Mercator
    /// between zoom 11 and 12.
    /// </summary>
    Globe = 2,
}
