// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for vector and raster tile sources.
/// </summary>
/// <remarks>
/// See <c>mln_style_tile_source_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleTileSourceOptions
{
    public double? MinZoom { get; set; }
    public double? MaxZoom { get; set; }
    public string? Attribution { get; set; }

    /// <summary>
    /// One of <c>mln_style_tile_scheme</c>. Defaults to
    /// <c>MLN_STYLE_TILE_SCHEME_XYZ</c>.
    /// </summary>
    public StyleTileScheme? Scheme { get; set; }
    public LatLngBounds? Bounds { get; set; }

    /// <summary>
    /// Raster tile size in pixels. Defaults to 512.
    /// </summary>
    public uint? TileSize { get; set; }

    /// <summary>
    /// One of <c>mln_style_vector_tile_encoding</c>. Defaults to MVT.
    /// </summary>
    public StyleVectorTileEncoding? VectorEncoding { get; set; }

    /// <summary>
    /// One of <c>mln_style_raster_dem_encoding</c>. Defaults to Mapbox.
    /// </summary>
    public StyleRasterDemEncoding? RasterEncoding { get; set; }
    public static StyleTileSourceOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_style_tile_source_options_default");
            return GeneratedValues.CopyStyleTileSourceOptions(
                NativeMethods.mln_style_tile_source_options_default()
            );
        }
    }
}
