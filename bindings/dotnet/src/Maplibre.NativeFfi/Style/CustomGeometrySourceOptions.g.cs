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
    /// Tile fetch callback. Required in callback mode; null in queue mode.
    /// </summary>
    public required CustomSourceFetchTileCallback FetchTile { get; init; }

    /// <summary>
    /// Optional best-effort tile cancel callback. Null in queue mode.
    /// </summary>
    public CustomSourceCancelTileCallback? CancelTile { get; init; }
    public double? MinZoom { get; init; }
    public double? MaxZoom { get; init; }
    public double? Tolerance { get; init; }
    public uint? TileSize { get; init; }
    public uint? Buffer { get; init; }
    public bool? Clip { get; init; }
    public bool? Wrap { get; init; }
}
