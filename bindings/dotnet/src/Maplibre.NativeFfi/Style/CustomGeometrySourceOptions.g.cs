// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The callbacks and options of a custom geometry source.
/// </summary>
/// <remarks>
/// <para>Zoom levels are whole numbers from 0 to 32.</para>
/// <para>See <c>mln_custom_geometry_source_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record CustomGeometrySourceOptions
{
    public required CustomSourceFetchTileCallback FetchTile { get; init; }
    public CustomSourceCancelTileCallback? CancelTile { get; init; }
    public double? MinZoom { get; init; }
    public double? MaxZoom { get; init; }

    /// <summary>
    /// How far, in pixels, simplified geometry can stray from the original.
    /// </summary>
    public double? Tolerance { get; init; }

    /// <summary>
    /// The width of a tile in pixels, a power of two no larger than 8192.
    /// </summary>
    public uint? TileSize { get; init; }

    /// <summary>
    /// The width in pixels of the margin around a tile that holds geometry from
    /// beyond its edges. <c>buffer</c> times 8192 / <c>tile_size</c> must be at
    /// most 65535.
    /// </summary>
    public uint? Buffer { get; init; }

    /// <summary>
    /// Whether to cut geometry at the outer edge of a tile's margin.
    /// </summary>
    public bool? Clip { get; init; }

    /// <summary>
    /// Whether to wrap geometry that crosses the antimeridian.
    /// </summary>
    public bool? Wrap { get; init; }
}
