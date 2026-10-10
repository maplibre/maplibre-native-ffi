// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Fields available in <c>mln_style_source_info</c>.
/// </summary>
/// <remarks>
/// See <c>mln_style_source_info_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum StyleSourceInfoField : uint
{
    /// <summary>
    /// The source retains a URL.
    /// </summary>
    Url = 1,

    /// <summary>
    /// The tile source was defined with an inline TileJSON description.
    /// </summary>
    Tilejson = 2,

    /// <summary>
    /// The inline TileJSON description contains geographic bounds.
    /// </summary>
    Bounds = 4,

    /// <summary>
    /// The source exposes a tile size.
    /// </summary>
    TileSize = 8,

    /// <summary>
    /// The source exposes a vector tile encoding.
    /// </summary>
    VectorEncoding = 16,

    /// <summary>
    /// The source exposes a DEM raster encoding.
    /// </summary>
    RasterEncoding = 32,

    /// <summary>
    /// The source declares an attribution string.
    /// </summary>
    Attribution = 64,
}
