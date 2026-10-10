// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One style layer borrowed for a list completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_style_layer_entry</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct StyleLayerEntry(
    string Id,
    string Type,
    string? SourceId,
    string? SourceLayer
);
