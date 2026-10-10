// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Inline tile metadata selected as one value by the source-info field mask.
/// </summary>
/// <remarks>
/// See <c>mln_style_source_tile_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
/// <param name="TileCount">
/// Inline tile URL count.
/// </param>
/// <param name="MinZoom">
/// Minimum zoom.
/// </param>
/// <param name="MaxZoom">
/// Maximum zoom.
/// </param>
/// <param name="Scheme">
/// One of <c>mln_style_tile_scheme</c>.
/// </param>
public readonly partial record struct StyleSourceTileInfo(
    ulong TileCount,
    double MinZoom,
    double MaxZoom,
    StyleTileScheme Scheme
);
