// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// When a map renders.
/// </summary>
/// <remarks>
/// See <c>mln_map_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum MapMode : uint
{
    /// <summary>
    /// Renders whenever the map changes.
    /// </summary>
    Continuous = 0,

    /// <summary>
    /// Renders only when <c>MapHandle.RequestStillImageAsync()</c> asks for an
    /// image.
    /// </summary>
    Static = 1,

    /// <summary>
    /// Renders still images of single tiles, as <c>MapMode.Static</c> does.
    /// </summary>
    Tile = 2,
}
