// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How the map keeps the camera within the world or within its bounds.
/// </summary>
/// <remarks>
/// See <c>mln_constrain_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum ConstrainMode : uint
{
    None = 0,

    /// <summary>
    /// Keeps the world filling the view from top to bottom.
    /// </summary>
    HeightOnly = 1,

    /// <summary>
    /// Keeps the world filling the view in both directions.
    /// </summary>
    WidthAndHeight = 2,

    /// <summary>
    /// Keeps the whole view inside the bounds from
    /// <c>MapHandle.SetBoundsAsync()</c>.
    /// </summary>
    Screen = 3,
}
