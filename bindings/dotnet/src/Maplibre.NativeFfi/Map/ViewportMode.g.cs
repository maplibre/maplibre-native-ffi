// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// The vertical direction of the image that the map renders.
/// </summary>
/// <remarks>
/// See <c>mln_viewport_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum ViewportMode : uint
{
    Default = 0,

    /// <summary>
    /// Renders the image upside down.
    /// </summary>
    FlippedY = 1,
}
