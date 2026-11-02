// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How the map picks the tiles to load.
/// </summary>
/// <remarks>
/// <para>The <c>lod_</c> members lower the level of detail away from the
/// camera, so the map loads fewer tiles when the pitch is high.</para>
/// <para>See <c>mln_map_tile_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record MapTileOptions
{
    /// <summary>
    /// How many zoom levels lower the map also loads tiles for, so it can show
    /// a coarse map while the current tiles load. 0 loads only the current
    /// tiles.
    /// </summary>
    public uint? PrefetchZoomDelta { get; init; }

    /// <summary>
    /// The radius around the view point, in tiles, that keeps full detail.
    /// </summary>
    public double? LodMinRadius { get; init; }

    /// <summary>
    /// Scales the distance to the view point. Above 1 lowers the detail.
    /// </summary>
    public double? LodScale { get; init; }

    /// <summary>
    /// The pitch, in radians, above which the map lowers the detail.
    /// </summary>
    public double? LodPitchThreshold { get; init; }

    /// <summary>
    /// Zoom levels to add for picking the detail. Below 0 loads fewer tiles.
    /// </summary>
    public double? LodZoomShift { get; init; }

    /// <summary>
    /// <c>TileLodMode.Distance</c> ignores <c>lod_min_radius</c>.
    /// </summary>
    public TileLodMode? LodMode { get; init; }
}
