// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public sealed unsafe partial class RenderFrameBatchHandle
    : IDisposable,
        INativeOwner<MlnRenderFrameBatch>
{
    private readonly NativeHandleState<MlnRenderFrameBatch> state;

    internal RenderFrameBatchHandle(MlnRenderFrameBatch handle)
    {
        state = new(handle, Abandon, nameof(RenderFrameBatchHandle), Abandon);
    }

    internal static RenderFrameBatchHandle Adopt(MlnRenderFrameBatch handle) =>
        NativeHandleState<MlnRenderFrameBatch>.Adopt(
            handle,
            () => new RenderFrameBatchHandle(handle),
            Abandon
        );

    private static mln_status Abandon(MlnRenderFrameBatch live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_render_frame_batch_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnRenderFrameBatch> INativeOwner<MlnRenderFrameBatch>.State => state;
    internal MlnRenderFrameBatch Handle => state.Handle;
    internal NativeCallbackOwner CallbackOwner => state.CallbackOwner;

    // Runtime events report their source by this identity.
    public ulong Id => state.IssuedHandle.Value;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_render_frame_batch_release");
        state.Retire();
    }

    public ulong Count()
    {
        using var read = state.Read(this, "mln_render_frame_batch_count");
        nuint outCount = default;
        Check(NativeMethods.mln_render_frame_batch_count(read.Handle, &outCount, Diagnostic));
        return (ulong)outCount;
    }

    public RenderFrameResult Get(ulong index)
    {
        using var read = state.Read(this, "mln_render_frame_batch_get");
        var outResult = new mln_render_frame_result
        {
            size = (uint)sizeof(mln_render_frame_result),
        };
        Check(
            NativeMethods.mln_render_frame_batch_get(
                read.Handle,
                checked((nuint)index),
                &outResult,
                Diagnostic
            )
        );
        return CopyRenderFrameResult(outResult);
    }

    public void Close()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_render_frame_batch_release");
        state.Close();
    }
}
