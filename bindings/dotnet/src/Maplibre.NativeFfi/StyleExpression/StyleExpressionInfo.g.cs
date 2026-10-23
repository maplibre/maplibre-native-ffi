// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// What a parsed expression depends on.
/// </summary>
/// <remarks>
/// See <c>mln_style_expression_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
/// </remarks>
/// <param name="IsFeatureConstant">
/// Whether the result is the same for every feature and feature state.
/// </param>
/// <param name="IsZoomConstant">
/// Whether the result is the same at every zoom.
/// </param>
public readonly partial record struct StyleExpressionInfo(
    bool IsFeatureConstant,
    bool IsZoomConstant
);
