// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Debug overlay mask values for <c>mln_map_set_debug_options()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_map_debug_option</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum MapDebugOption : uint
{
    TileBorders = 2,
    ParseStatus = 4,
    Timestamps = 8,
    Collision = 16,
    Overdraw = 32,
    StencilClip = 64,
    DepthBuffer = 128,
}
