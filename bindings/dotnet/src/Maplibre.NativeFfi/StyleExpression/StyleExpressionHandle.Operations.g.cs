// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A parsed style expression.
/// </summary>
/// <remarks>
/// See <c>mln_style_expression</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class StyleExpressionHandle
    : IDisposable,
        INativeOwner<MlnStyleExpression>
{
    private readonly NativeHandleState<MlnStyleExpression> state;

    internal StyleExpressionHandle(MlnStyleExpression handle)
    {
        state = new(handle, Abandon, nameof(StyleExpressionHandle), Abandon);
    }

    internal static StyleExpressionHandle Adopt(MlnStyleExpression handle) =>
        NativeHandleState<MlnStyleExpression>.Adopt(
            handle,
            () => new StyleExpressionHandle(handle),
            Abandon
        );

    private static mln_status Abandon(MlnStyleExpression live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_style_expression_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnStyleExpression> INativeOwner<MlnStyleExpression>.State => state;
    internal MlnStyleExpression Handle => state.Handle;

    /// <summary>
    /// The native handle value. It stays readable after release.
    /// </summary>
    public ulong Id => state.Handle.Value;

    /// <summary>
    /// Whether a release, dispose, or discard has ended the handle.
    /// </summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>
    /// Names the handle's type, its <see cref="Id" />, and whether it is closed.
    /// </summary>
    public override string ToString() => state.ToString();

    void IDisposable.Dispose()
    {
        state.Close();
    }

    /// <summary>
    /// Parses a style
    /// [expression](https://maplibre.org/maplibre-style-spec/expressions/) to
    /// evaluate without a map.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_style_expression_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when <c>expression</c>
    /// isn't a valid expression, or its result can't have the expected
    /// type.</description></item>
    /// </list>
    /// </remarks>
    public static StyleExpressionHandle Create(
        string expression,
        StyleExpressionOptions? options = null
    )
    {
        using var scope = new NativeCallScope(null);
        var nativeOptions = options is null
            ? default(mln_style_expression_options)
            : NativeStyleExpressionOptions(options);
        MlnStyleExpression outExpression = default;
        Check(
            NativeMethods.mln_style_expression_create(
                scope.Utf8(expression),
                options is null ? null : &nativeOptions,
                &outExpression,
                Diagnostic
            )
        );
        scope.Accept();
        return StyleExpressionHandle.Adopt(outExpression);
    }

    /// <summary>
    /// Evaluates a style expression and writes its result as JSON.
    /// </summary>
    /// <remarks>
    /// <para>A number in the result that isn't finite becomes null. A color is
    /// <c>["rgba", r, g, b, a]</c>, with red, green, and blue from 0 to 255 and
    /// alpha from 0 to 1, each exact only to single precision. Formatted text
    /// is a <c>["format", …]</c> expression, an image is <c>{"name": …,
    /// "available": …}</c>, padding is <c>[top, right, bottom, left]</c>, and a
    /// variable anchor offset is <c>[anchor, [x, y], …]</c>.</para>
    /// <para>See <c>mln_style_expression_evaluate</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when the expression
    /// fails to evaluate, such as when it reads an input that <c>inputs</c>
    /// doesn't set.</description></item>
    /// </list>
    /// </remarks>
    public string Evaluate(StyleExpressionInputs? inputs = null)
    {
        using var scope = new NativeCallScope(this);
        var nativeInputs = inputs is null
            ? default(mln_style_expression_inputs)
            : NativeStyleExpressionInputs(inputs, scope);
        nuint jsonCapacity = 256;
        nuint outJsonLength = 0;
        var outJson = scope.OutputArray<sbyte>((int)jsonCapacity);
        while (true)
        {
            Check(
                NativeMethods.mln_style_expression_evaluate(
                    Handle,
                    inputs is null ? null : &nativeInputs,
                    outJson,
                    jsonCapacity,
                    &outJsonLength,
                    Diagnostic
                )
            );
            if (outJsonLength < jsonCapacity)
                break;
            jsonCapacity = outJsonLength + 1;
            outJson = scope.OutputArray<sbyte>(checked((int)jsonCapacity));
        }
        scope.Accept();
        return RuntimeStructs.CopyUtf8(outJson, outJsonLength);
    }

    /// <summary>
    /// Reads what a style expression's result depends on.
    /// </summary>
    /// <remarks>
    /// See <c>mln_style_expression_get_info</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
    /// </remarks>
    public StyleExpressionInfo GetInfo()
    {
        using var call = Enter(this);
        var outInfo = new mln_style_expression_info
        {
            size = (uint)sizeof(mln_style_expression_info),
        };
        Check(NativeMethods.mln_style_expression_get_info(Handle, &outInfo, Diagnostic));
        return CopyStyleExpressionInfo(outInfo);
    }

    /// <summary>
    /// Releases a parsed style expression.
    /// </summary>
    /// <remarks>
    /// See <c>mln_style_expression_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
    /// </remarks>
    public void Release()
    {
        state.Close();
    }
}
