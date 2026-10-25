// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A parsed style expression, immutable and independent of any map.
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
    /// The native handle value. It stays readable after close.
    /// </summary>
    public ulong Id => state.Handle.Value;

    /// <summary>
    /// Whether a release, dispose, close, or discard has ended the handle.
    /// </summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>
    /// Names the handle's type, its <see cref="Id" />, and whether it is closed.
    /// </summary>
    public override string ToString() => state.ToString();

    public void Dispose()
    {
        state.Retire();
    }

    /// <summary>
    /// Parses a style expression.
    /// </summary>
    /// <remarks>
    /// See <c>mln_style_expression_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
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
    /// Evaluates a parsed expression and writes its result as UTF-8 JSON.
    /// </summary>
    /// <remarks>
    /// See <c>mln_style_expression_evaluate</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
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
    /// Copies what a parsed expression depends on into out_info.
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
}
