// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The callbacks and options of a custom vector source.
/// </summary>
/// <remarks>
/// <para>Zoom levels are whole numbers from 0 to 32.</para>
/// <para>See <c>mln_custom_vector_source_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record CustomVectorSourceOptions
{
    public required CustomSourceFetchTileCallback FetchTile { get; init; }
    public CustomSourceCancelTileCallback? CancelTile { get; init; }
    public double? MinZoom { get; init; }
    public double? MaxZoom { get; init; }
}
