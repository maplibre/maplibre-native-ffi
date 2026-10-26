// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One source-feature query hit.
/// </summary>
/// <remarks>
/// See <c>mln_source_feature</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct SourceFeature(string Feature, string SourceLayerId);
