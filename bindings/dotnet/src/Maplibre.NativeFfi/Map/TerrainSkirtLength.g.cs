// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How far the skirts that hide the cracks between terrain tiles reach down.
/// </summary>
/// <remarks>
/// See <c>mln_terrain_skirt_length</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum TerrainSkirtLength : uint
{
    /// <summary>
    /// Far enough to hide the cracks.
    /// </summary>
    Auto = 0,

    /// <summary>
    /// No skirts, so they don't show through translucent layers over the
    /// terrain.
    /// </summary>
    None = 1,
}
