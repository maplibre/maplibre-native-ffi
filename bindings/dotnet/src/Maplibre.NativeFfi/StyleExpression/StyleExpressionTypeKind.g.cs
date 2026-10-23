// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Style expression types that a parsed expression may be required to have.
/// </summary>
/// <remarks>
/// See <c>mln_style_expression_type_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
/// </remarks>
public enum StyleExpressionTypeKind : uint
{
    /// <summary>
    /// Any value.
    /// </summary>
    Value = 0,
    Number = 1,
    String = 2,
    Boolean = 3,
    Color = 4,
    Object = 5,
    Formatted = 6,

    /// <summary>
    /// A resolved image, as the image expression returns.
    /// </summary>
    Image = 7,
    Padding = 8,
    VariableAnchorOffsetCollection = 9,

    /// <summary>
    /// An array whose items have item_kind, which is not itself an array.
    /// </summary>
    Array = 10,
}
