// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Optional fields for <c>mln_feature_state_selector</c>.
/// </summary>
/// <remarks>
/// See <c>mln_feature_state_selector_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum FeatureStateSelectorField : uint
{
    SourceLayerId = 1,
    FeatureId = 2,
    StateKey = 4,
}
