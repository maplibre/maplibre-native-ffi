// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A style expression type.
/// </summary>
/// <remarks>
/// See <c>mln_style_expression_type</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Kind">
/// One of <c>mln_style_expression_type_kind</c>.
/// </param>
/// <param name="ItemKind">
/// For an array, its items' kind; ignored for any other kind.
/// </param>
/// <param name="ItemCount">
/// For an array, its fixed length, or zero for any length.
/// </param>
public readonly partial record struct StyleExpressionType(
    StyleExpressionTypeKind Kind,
    StyleExpressionTypeKind ItemKind,
    uint ItemCount
);
