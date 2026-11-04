// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for attaching a render session to a map.
/// </summary>
/// <remarks>
/// See <c>mln_render_session_attach_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct RenderSessionAttachOptions
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public RenderSessionAttachOptions(
        RenderDriverKind Driver,
        GpuSyncKindFlag ProducerSyncKinds,
        Wake FrameWake,
        Wake RedrawWake,
        Wake DriverWorkWake,
        QueueLock QueueLock
    )
    {
        this.Driver = Driver;
        this.ProducerSyncKinds = ProducerSyncKinds;
        this.FrameWake = FrameWake;
        this.RedrawWake = RedrawWake;
        this.DriverWorkWake = DriverWorkWake;
        this.QueueLock = QueueLock;
    }

    /// <summary>
    /// The thread that runs the session's graphics work, which depends on the
    /// target:
    /// </summary>
    /// <remarks>
    /// <list type="bullet">
    /// <item><description><c>RenderDriverKind.CallerGraphicsThread</c> for a
    /// shared or dedicated EGL or WGL context, an existing WebGL context, or a
    /// browser's WebGPU device.</description></item>
    /// <item><description><c>RenderDriverKind.CoreWorker</c> for a transferred
    /// WebGL canvas, an Android or OpenHarmony window, or an offscreen
    /// session.</description></item>
    /// <item><description>Either one for a Metal or Vulkan
    /// target.</description></item>
    /// </list>
    /// <para>An attach with another driver fails with
    /// <c>Status.Unsupported</c>.</para>
    /// </remarks>
    public required RenderDriverKind Driver { get; init; }

    /// <summary>
    /// The kinds of <c>GpuSync</c> that you can wait on before you read an
    /// acquired frame. The session uses one of them that its target supports.
    /// Zero lets the session pick a kind that names no object to wait on.
    /// </summary>
    public GpuSyncKindFlag ProducerSyncKinds { get; init; }

    /// <summary>
    /// Called when frame results are waiting for
    /// <c>RenderSessionHandle.DrainFrameResults()</c>. It isn't called again
    /// until a drain takes them.
    /// </summary>
    public Wake FrameWake { get; init; }

    /// <summary>
    /// Called when the session has something new to draw. Demand a frame at
    /// your next display refresh. It isn't called again until you demand a
    /// frame, or while the session has no target.
    /// </summary>
    public Wake RedrawWake { get; init; }

    /// <summary>
    /// Called when a session driven by
    /// <c>RenderDriverKind.CallerGraphicsThread</c> has work for
    /// <c>RenderSessionHandle.ServiceDriverWork()</c>. It isn't called again
    /// until a service runs.
    /// </summary>
    public Wake DriverWorkWake { get; init; }

    /// <summary>
    /// Your lock on the Vulkan queue, when you submit to it yourself.
    /// </summary>
    public QueueLock QueueLock { get; init; }
}
