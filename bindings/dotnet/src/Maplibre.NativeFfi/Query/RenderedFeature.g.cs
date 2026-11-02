// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A feature that a rendered feature query found.
/// </summary>
/// <remarks>
/// See <c>mln_rendered_feature</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Feature">
/// The feature, as a GeoJSON feature.
/// </param>
/// <param name="LayerId">
/// The layer that drew the feature.
/// </param>
/// <param name="SourceLayerId">
/// The feature's source layer, or empty when its source has none.
/// </param>
/// <param name="State">
/// The feature's state, as a JSON object.
/// </param>
public readonly partial record struct RenderedFeature(
    string Feature,
    string LayerId,
    string SourceId,
    string SourceLayerId,
    string State
);
