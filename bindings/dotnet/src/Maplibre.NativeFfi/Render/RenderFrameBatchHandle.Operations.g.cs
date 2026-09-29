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

namespace Maplibre.NativeFfi.Render;

public sealed unsafe partial class RenderFrameBatchHandle : IDisposable
{
    private readonly NativeHandleState<MlnRenderFrameBatch> state;
    private readonly ulong nativeId;
    public ulong Id => nativeId;

    internal RenderFrameBatchHandle(MlnRenderFrameBatch handle)
    {
        nativeId = handle.Value;
        state = new NativeHandleState<MlnRenderFrameBatch>(
            handle,
            static (live, _) =>
            {
                NativeMethods.mln_render_frame_batch_release(live);
                return mln_status.MLN_STATUS_OK;
            },
            nameof(RenderFrameBatchHandle),
            static (live, _) =>
            {
                NativeMethods.mln_render_frame_batch_release(live);
                return mln_status.MLN_STATUS_OK;
            }
        );
    }

    internal static RenderFrameBatchHandle Adopt(MlnRenderFrameBatch handle)
    {
        RenderFrameBatchHandle? owner = null;
        try
        {
            owner = new RenderFrameBatchHandle(handle);
            return owner;
        }
        catch
        {
            if (owner is null)
                NativeMethods.mln_render_frame_batch_release(handle);
            else
                owner.state.Retire();
            throw;
        }
    }

    internal MlnRenderFrameBatch Handle => state.Handle;

    internal NativeHandleState<MlnRenderFrameBatch>.ReadScope Borrow() => state.Borrow();

    internal global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackOwner CallbackOwner =>
        state.CallbackOwner;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_frame_batch_release"
        );
        state.Retire();
    }

    public ulong Count()
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_frame_batch_count"
        );
        nuint outCount = default;
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_render_frame_batch_count(
                read.Handle,
                &outCount,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        return (ulong)outCount;
    }

    public RenderFrameResult Get(ulong index)
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_frame_batch_get"
        );
        var outResult = new mln_render_frame_result
        {
            size = (uint)sizeof(mln_render_frame_result),
        };
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_render_frame_batch_get(
                read.Handle,
                checked((nuint)index),
                &outResult,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        return CopyRenderFrameResult(outResult);
    }

    public void Close()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_frame_batch_release"
        );
        state.Close();
    }
}
