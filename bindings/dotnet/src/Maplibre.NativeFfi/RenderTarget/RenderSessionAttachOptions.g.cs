// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Common attachment policy copied before an attach call returns.
/// </summary>
/// <remarks>
/// See <c>mln_render_session_attach_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Driver">
/// One <c>mln_render_driver_kind</c> value. Defaults to
/// <c>MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD</c>.
/// </param>
/// <param name="RequestedTextureRingDepth">
/// Requested slot count of a session-owned texture ring, from one to
/// <c>MLN_TEXTURE_RING_MAX_DEPTH</c>. Private targets grant one slot regardless
/// of this value. A borrowed texture ring's depth is its texture count, so
/// borrowed and other targets ignore this value. Defaults to 1.
/// </param>
/// <param name="ProducerSyncKind">
/// Producer synchronization for acquired texture frames. Defaults to
/// <c>MLN_GPU_SYNC_CPU_COMPLETE</c>. <c>MLN_GPU_SYNC_OPENGL_FENCE</c> requires
/// a shared EGL or WGL texture target and a caller-graphics-thread driver.
/// Other targets support only CPU completion and fail an unsupported request.
/// </param>
/// <param name="FrameWake">
/// Wakes the receiver when the frame-result queue becomes nonempty.
/// </param>
/// <param name="DriverWorkWake">
/// Wakes the graphics receiver when caller-driver work is available, including
/// work that a finished driver result releases.
/// </param>
/// <param name="QueueLock">
/// Host lock on the graphics queue, disabled by default. Only Vulkan targets
/// accept an enabled lock; other backends fail the attach with
/// <c>MLN_STATUS_UNSUPPORTED</c>.
/// </param>
public readonly partial record struct RenderSessionAttachOptions(
    RenderDriverKind Driver,
    uint RequestedTextureRingDepth,
    GpuSyncKind ProducerSyncKind,
    Wake FrameWake,
    Wake DriverWorkWake,
    QueueLock QueueLock
)
{
    public RenderSessionAttachOptions()
        : this(default!, default!, default!, default!, default!, default!)
    {
        this = Default;
    }

    public static RenderSessionAttachOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyRenderSessionAttachOptions(
                NativeMethods.mln_render_session_attach_options_default()
            );
        }
    }
}
