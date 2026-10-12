// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Field mask values for <c>mln_style_tile_source_options</c>.
/// </summary>
/// <remarks>
/// See <c>mln_style_tile_source_option_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum StyleTileSourceOptionField : uint
{
    MinZoom = 1,
    MaxZoom = 2,
    Attribution = 4,
    Scheme = 8,
    Bounds = 16,
    TileSize = 32,
    VectorEncoding = 64,
    RasterEncoding = 128,
}
