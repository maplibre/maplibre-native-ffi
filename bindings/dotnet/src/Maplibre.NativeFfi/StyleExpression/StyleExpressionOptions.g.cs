// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for parsing a style expression.
/// </summary>
/// <remarks>
/// See <c>mln_style_expression_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleExpressionOptions
{
    /// <summary>
    /// The type that the expression must produce. A string type coerces the
    /// result to a string, as a string layer property does; every other type
    /// asserts it. Without it, the expression keeps the type it infers.
    /// </summary>
    public StyleExpressionType? ExpectedType { get; init; }
}
