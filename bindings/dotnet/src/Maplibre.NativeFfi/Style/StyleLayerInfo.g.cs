// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Complete metadata of one style layer, borrowed for a completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_style_layer_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Visibility">
/// One of <c>mln_style_layer_visibility</c>.
/// </param>
/// <param name="Type">
/// The style-spec layer type string. The view stays valid for the life of the
/// process.
/// </param>
/// <param name="MinZoom">
/// Lowest zoom at which the layer draws; -INFINITY with no lower bound.
/// </param>
/// <param name="MaxZoom">
/// Highest zoom at which the layer draws; INFINITY with no upper bound.
/// </param>
/// <param name="SourceId">
/// Source ID. Empty for a layer type that takes no source.
/// </param>
/// <param name="SourceLayer">
/// Source-layer ID. Empty when the layer sets none.
/// </param>
public readonly partial record struct StyleLayerInfo(
    StyleLayerVisibility Visibility,
    string Type,
    double MinZoom,
    double MaxZoom,
    string? SourceId,
    string? SourceLayer
);
