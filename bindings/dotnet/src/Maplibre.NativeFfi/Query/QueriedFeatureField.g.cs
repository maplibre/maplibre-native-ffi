// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Optional fields for <c>mln_queried_feature</c>.
/// </summary>
/// <remarks>
/// See <c>mln_queried_feature_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum QueriedFeatureField : uint
{
    SourceId = 1,
    SourceLayerId = 2,
    State = 4,
}
