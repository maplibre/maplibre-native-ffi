// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A layer in the map's style.
/// </summary>
/// <remarks>
/// See <c>mln_style_layer_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Type">
/// The layer's [type](https://maplibre.org/maplibre-style-spec/layers/#type),
/// such as <c>fill</c>.
/// </param>
/// <param name="MinZoom">
/// The zoom level where the layer starts to show, or negative infinity when the
/// layer doesn't set one.
/// </param>
/// <param name="MaxZoom">
/// The zoom level where the layer stops showing, or infinity when the layer
/// doesn't set one.
/// </param>
public readonly partial record struct StyleLayerInfo(
    string Id,
    string Type,
    string SourceId,
    string SourceLayer,
    double MinZoom,
    double MaxZoom,
    StyleLayerVisibility Visibility
);
