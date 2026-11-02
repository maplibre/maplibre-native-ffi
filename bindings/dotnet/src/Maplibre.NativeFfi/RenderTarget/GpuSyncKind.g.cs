// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How one side waits for the other's GPU work on a shared texture.
/// </summary>
/// <remarks>
/// <para>A sync file descriptor or an OpenGL fence signals once, so when
/// <c>AcquiredFrameHandle.Release()</c> succeeds, the session takes the one you
/// pass. A Vulkan timeline semaphore, Metal shared event, or D3D12 fence counts
/// up with each frame, so the one you pass stays yours. Keep it valid until a
/// detach or disposal of the session completes.</para>
/// <para>See <c>mln_gpu_sync_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public enum GpuSyncKind : uint
{
    /// <summary>
    /// The work has already finished, so there is nothing to wait for.
    /// </summary>
    CpuComplete = 0,

    /// <summary>
    /// <c>handle</c> is a <c>GLsync</c> in the session's share group.
    /// </summary>
    OpenGlFence = 1,

    /// <summary>
    /// The work was submitted to the queue that both sides share, so work
    /// submitted to that queue later runs after it.
    /// </summary>
    QueueOrder = 2,

    /// <summary>
    /// <c>handle</c> is a <c>VkSemaphore</c>. Wait until it reaches
    /// <c>value</c>.
    /// </summary>
    VulkanTimelineSemaphore = 3,

    /// <summary>
    /// <c>handle</c> is a sync file descriptor.
    /// </summary>
    SyncFd = 4,

    /// <summary>
    /// <c>handle</c> is an <c>MTLSharedEvent</c>. Wait until it reaches
    /// <c>value</c>.
    /// </summary>
    MetalSharedEvent = 5,

    /// <summary>
    /// <c>handle</c> is a shared Windows handle to an <c>ID3D12Fence</c>. Wait
    /// until it reaches <c>value</c>.
    /// </summary>
    D3d12Fence = 6,
}
