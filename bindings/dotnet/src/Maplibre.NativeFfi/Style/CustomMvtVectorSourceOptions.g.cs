// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Style;

public sealed record CustomMvtVectorSourceOptions
{
    public Action<CanonicalTileId>? FetchTile { get; set; }
    public Action<CanonicalTileId>? CancelTile { get; set; }
    public double? MinZoom { get; set; }
    public double? MaxZoom { get; set; }
    public static CustomMvtVectorSourceOptions Default
    {
        get
        {
            using var call = Enter(null, "mln_custom_mvt_vector_source_options_default");
            return CopyCustomMvtVectorSourceOptions(
                NativeMethods.mln_custom_mvt_vector_source_options_default()
            );
        }
    }
}
