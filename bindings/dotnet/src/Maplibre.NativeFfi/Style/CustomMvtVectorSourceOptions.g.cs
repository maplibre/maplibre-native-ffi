// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for custom MVT vector sources.
/// </summary>
/// <remarks>
/// See <c>mln_custom_mvt_vector_source_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record CustomMvtVectorSourceOptions
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
    public static CustomMvtVectorSourceOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_custom_mvt_vector_source_options_default");
            return GeneratedValues.CopyCustomMvtVectorSourceOptions(
                NativeMethods.mln_custom_mvt_vector_source_options_default()
            );
        }
    }
}
