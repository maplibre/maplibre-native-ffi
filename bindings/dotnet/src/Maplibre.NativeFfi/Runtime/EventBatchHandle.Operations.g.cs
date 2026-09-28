// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi.Runtime;

public sealed unsafe partial class EventBatchHandle : IDisposable
{
    private readonly NativeHandleState<MlnEventBatch> state;
    private readonly ulong nativeId;
    internal ulong NativeId => nativeId;

    internal EventBatchHandle(MlnEventBatch handle)
    {
        nativeId = handle.Value;
        state = new NativeHandleState<MlnEventBatch>(
            handle,
            static live =>
            {
                NativeMethods.mln_event_batch_release(live);
                return mln_status.MLN_STATUS_OK;
            },
            nameof(EventBatchHandle),
            static live =>
            {
                NativeMethods.mln_event_batch_release(live);
                return mln_status.MLN_STATUS_OK;
            }
        );
    }

    internal static EventBatchHandle Adopt(MlnEventBatch handle)
    {
        EventBatchHandle? owner = null;
        try
        {
            owner = new EventBatchHandle(handle);
            return owner;
        }
        catch
        {
            if (owner is null)
                NativeMethods.mln_event_batch_release(handle);
            else
                owner.state.Retire();
            throw;
        }
    }

    internal MlnEventBatch Handle => state.Handle;

    internal NativeHandleState<MlnEventBatch>.ReadScope Borrow() => state.Borrow();

    internal global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackOwner CallbackOwner =>
        state.CallbackOwner;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_event_batch_release"
        );
        state.Retire();
    }

    public RuntimeEventBatchView Get()
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_event_batch_get"
        );
        var outView = new mln_runtime_event_batch_view
        {
            size = (uint)sizeof(mln_runtime_event_batch_view),
        };
        NativeStatus.Check(NativeMethods.mln_event_batch_get(read.Handle, &outView));
        return CopyRuntimeEventBatchView(outView);
    }

    public void Close()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_event_batch_release"
        );
        state.Close();
    }
}
