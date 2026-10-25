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
    /// Tile fetch callback. Required in callback mode; null in queue mode.
    /// </summary>
    public required CustomSourceFetchTileCallback FetchTile { get; init; }

    /// <summary>
    /// Optional best-effort tile cancel callback. Null in queue mode.
    /// </summary>
    public CustomSourceCancelTileCallback? CancelTile { get; init; }
    public double? MinZoom { get; init; }
    public double? MaxZoom { get; init; }
}
