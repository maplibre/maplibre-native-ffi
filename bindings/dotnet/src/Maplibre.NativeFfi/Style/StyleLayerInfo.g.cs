// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Fixed layer metadata included in <c>mln_style_layer_result</c>.
/// </summary>
/// <remarks>
/// See <c>mln_style_layer_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct StyleLayerInfo(
    string Type,
    double MinZoom,
    double MaxZoom,
    StyleLayerVisibility Visibility
);
