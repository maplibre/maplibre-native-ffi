// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One rendered-feature query hit.
/// </summary>
/// <remarks>
/// See <c>mln_rendered_feature</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct RenderedFeature(
    string Feature,
    string LayerId,
    string SourceId,
    string SourceLayerId,
    string State
);
