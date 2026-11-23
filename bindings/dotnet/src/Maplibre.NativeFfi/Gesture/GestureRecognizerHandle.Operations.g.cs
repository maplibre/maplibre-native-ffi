// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A gesture recognizer that moves a map's camera.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_recognizer</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class GestureRecognizerHandle
    : IDisposable,
        INativeOwner<MlnGestureRecognizer>
{
    private readonly NativeHandleState<MlnGestureRecognizer> state;

    internal GestureRecognizerHandle(MlnGestureRecognizer handle)
    {
        state = new(handle, Abandon, nameof(GestureRecognizerHandle), Abandon);
    }

    internal static GestureRecognizerHandle Adopt(MlnGestureRecognizer handle) =>
        NativeHandleState<MlnGestureRecognizer>.Adopt(
            handle,
            () => new GestureRecognizerHandle(handle),
            Abandon
        );

    private static mln_status Abandon(MlnGestureRecognizer live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_gesture_recognizer_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnGestureRecognizer> INativeOwner<MlnGestureRecognizer>.State => state;
    internal MlnGestureRecognizer NativeHandle => state.Handle;

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
    /// Handles one input at <c>timeMs</c>, in milliseconds on any clock that
    /// only goes forward, the same for every input and frame.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_gesture_recognizer_handle</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when <c>input</c> isn't
    /// a valid input.</description></item>
    /// </list>
    /// </remarks>
    public GestureResult Handle(GestureInput input, double timeMs)
    {
        using var call = Enter(this);
        var nativeInput = NativeGestureInput(input);
        var outResult = new mln_gesture_result { size = (uint)sizeof(mln_gesture_result) };
        Check(
            NativeMethods.mln_gesture_recognizer_handle(
                NativeHandle,
                &nativeInput,
                timeMs,
                &outResult,
                Diagnostic
            )
        );
        return CopyGestureResult(outResult);
    }

    /// <summary>
    /// Releases a gesture recognizer. A glide it started keeps going.
    /// </summary>
    /// <remarks>
    /// See <c>mln_gesture_recognizer_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
    /// </remarks>
    public void Release()
    {
        state.Close();
    }

    /// <summary>
    /// Changes the settings whose bits are set, from the next input on.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_gesture_recognizer_set_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when an option is out
    /// of range.</description></item>
    /// </list>
    /// </remarks>
    public void SetOptions(GestureOptions options)
    {
        using var scope = new NativeCallScope(this);
        var nativeOptions = NativeGestureOptions(options, scope);
        Check(
            NativeMethods.mln_gesture_recognizer_set_options(
                NativeHandle,
                &nativeOptions,
                Diagnostic
            )
        );
        scope.Accept();
    }

    /// <summary>
    /// Times long presses, steps the eases of discrete deltas, ends deltas once
    /// they idle, and samples the camera for notches, at <c>timeMs</c> on the
    /// inputs' clock. Call it when the last result's <c>next_tick</c> says: on
    /// the next display frame, or at its time with a frame or a timer. A tick
    /// at any other time does no harm.
    /// </summary>
    /// <remarks>
    /// See <c>mln_gesture_recognizer_tick</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
    /// </remarks>
    public GestureResult Tick(double timeMs)
    {
        using var call = Enter(this);
        var outResult = new mln_gesture_result { size = (uint)sizeof(mln_gesture_result) };
        Check(
            NativeMethods.mln_gesture_recognizer_tick(NativeHandle, timeMs, &outResult, Diagnostic)
        );
        return CopyGestureResult(outResult);
    }
}
