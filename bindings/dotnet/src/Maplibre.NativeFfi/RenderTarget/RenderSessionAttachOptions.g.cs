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
public readonly partial record struct RenderSessionAttachOptions
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public RenderSessionAttachOptions(
        RenderDriverKind Driver,
        uint RequestedTextureRingDepth,
        GpuSyncKind ProducerSyncKind,
        Wake FrameWake,
        Wake DriverWorkWake,
        QueueLock QueueLock
    )
    {
        this.Driver = Driver;
        this.RequestedTextureRingDepth = RequestedTextureRingDepth;
        this.ProducerSyncKind = ProducerSyncKind;
        this.FrameWake = FrameWake;
        this.DriverWorkWake = DriverWorkWake;
        this.QueueLock = QueueLock;
    }

    /// <summary>
    /// Where the session's driver runs. Required.
    /// </summary>
    public required RenderDriverKind Driver { get; init; }

    /// <summary>
    /// Requested slot count of a session-owned texture ring, up to 8. Zero
    /// selects 2, so the host can retain one frame while the session renders
    /// its replacement. Private targets grant one slot regardless of this
    /// value. A borrowed texture ring's depth is its texture count, so borrowed
    /// and other targets ignore this value.
    /// </summary>
    public uint RequestedTextureRingDepth { get; init; }

    /// <summary>
    /// Producer synchronization for acquired texture frames.
    /// <c>GpuSyncKind.OpenGlFence</c> requires a shared EGL or WGL texture
    /// target and a caller-graphics-thread driver. Other targets support only
    /// CPU completion and fail an unsupported request.
    /// </summary>
    public GpuSyncKind ProducerSyncKind { get; init; }

    /// <summary>
    /// Wakes the receiver when the frame-result queue becomes nonempty.
    /// </summary>
    public Wake FrameWake { get; init; }

    /// <summary>
    /// Wakes the graphics receiver when caller-driver work is available,
    /// including work that a finished driver result releases.
    /// </summary>
    public Wake DriverWorkWake { get; init; }

    /// <summary>
    /// Host lock on the graphics queue, disabled by default. Only Vulkan
    /// targets accept an enabled lock; other backends fail the attach with
    /// <c>Status.Unsupported</c>.
    /// </summary>
    public QueueLock QueueLock { get; init; }
}
