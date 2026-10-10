// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for custom geometry sources.
/// </summary>
/// <remarks>
/// See <c>mln_custom_geometry_source_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record CustomGeometrySourceOptions
{
    /// <summary>
    /// Required tile fetch callback.
    /// </summary>
    public Action<CanonicalTileId>? FetchTile { get; set; }

    /// <summary>
    /// Optional best-effort tile cancel callback.
    /// </summary>
    public Action<CanonicalTileId>? CancelTile { get; set; }
    public double? MinZoom { get; set; }
    public double? MaxZoom { get; set; }
    public double? Tolerance { get; set; }
    public uint? TileSize { get; set; }
    public uint? Buffer { get; set; }
    public bool? Clip { get; set; }
    public bool? Wrap { get; set; }
    public static CustomGeometrySourceOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_custom_geometry_source_options_default");
            return GeneratedValues.CopyCustomGeometrySourceOptions(
                NativeMethods.mln_custom_geometry_source_options_default()
            );
        }
    }
}
