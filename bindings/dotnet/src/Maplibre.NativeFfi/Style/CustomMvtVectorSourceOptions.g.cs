// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for custom MVT vector sources. The callback may run on any native
/// worker thread, concurrently with runtime and map calls.
/// </summary>
/// <remarks>
/// See <c>mln_custom_mvt_vector_source_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record CustomMvtVectorSourceOptions
{
    /// <summary>
    /// Tile fetch callback.
    /// </summary>
    public required CustomSourceFetchTileCallback FetchTile { get; init; }

    /// <summary>
    /// Optional best-effort tile cancel callback.
    /// </summary>
    public CustomSourceCancelTileCallback? CancelTile { get; init; }
    public double? MinZoom { get; init; }
    public double? MaxZoom { get; init; }
}
