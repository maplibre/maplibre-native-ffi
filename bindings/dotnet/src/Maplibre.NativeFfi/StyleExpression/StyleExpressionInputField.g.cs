// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Fields present in <c>mln_style_expression_inputs</c>.
/// </summary>
/// <remarks>
/// See <c>mln_style_expression_input_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum StyleExpressionInputField : uint
{
    Zoom = 1,
    ColorRampParameter = 2,
    Elevation = 4,
    Feature = 8,
    FeatureState = 16,
    GlobalState = 32,
    CanonicalTileId = 64,
}
