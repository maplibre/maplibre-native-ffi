// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// The type of a style expression's result, or of the items of an array result.
/// </summary>
/// <remarks>
/// See <c>mln_style_expression_type_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
/// </remarks>
public enum StyleExpressionTypeKind : uint
{
    /// <summary>
    /// Any type.
    /// </summary>
    Value = 0,
    Number = 1,
    String = 2,
    Boolean = 3,
    Color = 4,
    Object = 5,
    Formatted = 6,
    Image = 7,
    Padding = 8,
    VariableAnchorOffsetCollection = 9,
    Array = 10,
}
