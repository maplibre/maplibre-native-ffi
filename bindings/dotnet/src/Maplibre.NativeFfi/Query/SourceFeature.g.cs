// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A feature that a source feature query found.
/// </summary>
/// <remarks>
/// See <c>mln_source_feature</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Feature">
/// The feature, as a GeoJSON feature.
/// </param>
/// <param name="SourceLayerId">
/// The feature's source layer, or empty when its source has none.
/// </param>
public readonly partial record struct SourceFeature(string Feature, string SourceLayerId);
