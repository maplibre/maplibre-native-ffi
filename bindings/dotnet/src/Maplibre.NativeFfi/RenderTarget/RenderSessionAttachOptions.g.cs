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
        Wake DriverWorkWake,
        QueueLock QueueLock
    )
    {
        this.Driver = Driver;
        this.ProducerSyncKinds = ProducerSyncKinds;
        this.FrameWake = FrameWake;
        this.DriverWorkWake = DriverWorkWake;
        this.QueueLock = QueueLock;
    }

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
