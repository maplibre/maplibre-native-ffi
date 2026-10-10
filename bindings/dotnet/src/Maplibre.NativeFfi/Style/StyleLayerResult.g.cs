// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Complete layer metadata borrowed for a completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_style_layer_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct StyleLayerResult(
    StyleLayerInfo Info,
    string? SourceId,
    string? SourceLayer
);
