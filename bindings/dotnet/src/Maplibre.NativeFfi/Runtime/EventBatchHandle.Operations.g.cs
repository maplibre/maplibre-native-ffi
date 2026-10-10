// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// An owned batch of runtime events from one drain.
/// </summary>
/// <remarks>
/// See <c>mln_event_batch</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class EventBatchHandle : IDisposable, INativeOwner<MlnEventBatch>
{
    private readonly NativeHandleState<MlnEventBatch> state;

    internal EventBatchHandle(MlnEventBatch handle)
    {
        state = new(handle, Abandon, nameof(EventBatchHandle), Abandon);
    }

    internal static EventBatchHandle Adopt(MlnEventBatch handle) =>
        NativeHandleState<MlnEventBatch>.Adopt(handle, () => new EventBatchHandle(handle), Abandon);

    private static mln_status Abandon(MlnEventBatch live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_event_batch_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnEventBatch> INativeOwner<MlnEventBatch>.State => state;
    internal MlnEventBatch Handle => state.Handle;
    internal NativeCallbackOwner CallbackOwner => state.CallbackOwner;

    // Runtime events report their source by this identity.
    public ulong Id => state.IssuedHandle.Value;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_event_batch_release");
        state.Retire();
    }

    /// <summary>
    /// Borrows the event and message view stored by an owned event batch.
    /// </summary>
    /// <remarks>
    /// See <c>mln_event_batch_get</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public RuntimeEventBatchView Get()
    {
        using var read = state.Read(this, "mln_event_batch_get");
        var outView = new mln_runtime_event_batch_view
        {
            size = (uint)sizeof(mln_runtime_event_batch_view),
        };
        Check(NativeMethods.mln_event_batch_get(read.Handle, &outView, Diagnostic));
        return CopyRuntimeEventBatchView(outView);
    }

    /// <summary>
    /// Releases an owned event batch. A null handle is a no-op.
    /// </summary>
    /// <remarks>
    /// See <c>mln_event_batch_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public void Close()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_event_batch_release");
        state.Close();
    }
}
