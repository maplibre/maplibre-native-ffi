// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// What a style expression's result depends on.
/// </summary>
/// <remarks>
/// See <c>mln_style_expression_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
/// </remarks>
/// <param name="IsFeatureConstant">
/// True when the result doesn't depend on the feature or its state.
/// </param>
/// <param name="IsZoomConstant">
/// True when the result doesn't depend on the zoom level.
/// </param>
public readonly partial record struct StyleExpressionInfo(
    bool IsFeatureConstant,
    bool IsZoomConstant
);
