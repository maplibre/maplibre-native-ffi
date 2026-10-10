// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// An owned batch of frame results from one drain.
/// </summary>
/// <remarks>
/// See <c>mln_render_frame_batch</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
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

    /// <summary>
    /// Borrows the result view stored by an owned frame-result batch.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_frame_batch_get</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public RenderFrameBatchView Get()
    {
        using var read = state.Read(this, "mln_render_frame_batch_get");
        var outView = new mln_render_frame_batch_view
        {
            size = (uint)sizeof(mln_render_frame_batch_view),
        };
        Check(NativeMethods.mln_render_frame_batch_get(read.Handle, &outView, Diagnostic));
        return CopyRenderFrameBatchView(outView);
    }

    /// <summary>
    /// Releases a frame-result batch.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_frame_batch_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public void Close()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_render_frame_batch_release");
        state.Close();
    }
}
