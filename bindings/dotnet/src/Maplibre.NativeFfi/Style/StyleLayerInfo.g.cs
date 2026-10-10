// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One style layer, borrowed for a completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_style_layer_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Id">
/// Layer ID.
/// </param>
/// <param name="Type">
/// The style-spec layer type string.
/// </param>
/// <param name="SourceId">
/// Source ID. Empty for a layer type that takes no source.
/// </param>
/// <param name="SourceLayer">
/// Source-layer ID. Empty when the layer sets none.
/// </param>
/// <param name="MinZoom">
/// Lowest zoom at which the layer draws; -INFINITY with no lower bound.
/// </param>
/// <param name="MaxZoom">
/// Highest zoom at which the layer draws; INFINITY with no upper bound.
/// </param>
/// <param name="Visibility">
/// One of <c>mln_style_layer_visibility</c>.
/// </param>
public readonly partial record struct StyleLayerInfo(
    string Id,
    string Type,
    string? SourceId,
    string? SourceLayer,
    double MinZoom,
    double MaxZoom,
    StyleLayerVisibility Visibility
);
