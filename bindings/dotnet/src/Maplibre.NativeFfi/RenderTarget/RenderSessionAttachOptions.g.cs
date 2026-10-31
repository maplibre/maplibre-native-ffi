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
        GpuSyncKindFlag ProducerSyncKinds,
        Wake FrameWake,
        Wake DriverWorkWake,
        QueueLock QueueLock
    )
    {
        this.Driver = Driver;
        this.RequestedTextureRingDepth = RequestedTextureRingDepth;
        this.ProducerSyncKinds = ProducerSyncKinds;
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
    /// Producer synchronization kinds that the host accepts for acquired
    /// texture frames. The session picks the first one its target supports in
    /// this order: <c>GpuSyncKind.QueueOrder</c>,
    /// <c>GpuSyncKind.OpenGlFence</c>,
    /// <c>GpuSyncKind.VulkanTimelineSemaphore</c>,
    /// <c>GpuSyncKind.MetalSharedEvent</c>, <c>GpuSyncKind.D3d12Fence</c>,
    /// <c>GpuSyncKind.SyncFd</c>, then <c>GpuSyncKind.CpuComplete</c>. Its
    /// capabilities report the pick. Zero accepts
    /// <c>GpuSyncKind.CpuComplete</c>, or <c>GpuSyncKind.QueueOrder</c> on
    /// WebGPU, whose device has a single queue. A set that the target cannot
    /// satisfy fails the attach with <c>Status.Unsupported</c>.
    /// </summary>
    public GpuSyncKindFlag ProducerSyncKinds { get; init; }

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
