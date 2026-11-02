// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The type of a style expression's result.
/// </summary>
/// <remarks>
/// See <c>mln_style_expression_type</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
/// </remarks>
/// <param name="ItemKind">
/// The type of each item when <c>kind</c> is
/// <c>StyleExpressionTypeKind.Array</c>. It can't be an array itself.
/// </param>
/// <param name="ItemCount">
/// The length of an array, or 0 for any length.
/// </param>
public readonly partial record struct StyleExpressionType(
    StyleExpressionTypeKind Kind,
    StyleExpressionTypeKind ItemKind,
    uint ItemCount
);
