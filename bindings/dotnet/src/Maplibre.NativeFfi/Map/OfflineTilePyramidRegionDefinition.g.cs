// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// An offline region that covers a rectangle of the map.
/// </summary>
/// <remarks>
/// See <c>mln_offline_tile_pyramid_region_definition</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
/// <param name="StyleUrl">
/// The style whose resources the region holds. Maps use the region when they
/// load the same style URL.
/// </param>
/// <param name="MaxZoom">
/// INFINITY covers every zoom level that the style's sources have.
/// </param>
/// <param name="PixelRatio">
/// The pixel ratio of the images to download, such as 2 for dense screens.
/// </param>
/// <param name="IncludeIdeographs">
/// Whether the region holds the glyphs of CJK ideographs.
/// </param>
public readonly partial record struct OfflineTilePyramidRegionDefinition(
    string StyleUrl,
    LatLngBounds Bounds,
    double MinZoom,
    double MaxZoom,
    float PixelRatio,
    bool IncludeIdeographs
)
{
    public OfflineTilePyramidRegionDefinition()
        : this("", default, default, default, default, default) { }
}
