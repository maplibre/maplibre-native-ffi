// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Tile-pyramid offline region definition.
/// </summary>
/// <remarks>
/// See <c>mln_offline_tile_pyramid_region_definition</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct OfflineTilePyramidRegionDefinition(
    string StyleUrl,
    LatLngBounds Bounds,
    double MinZoom,
    double MaxZoom,
    float PixelRatio,
    bool IncludeIdeographs
);
