// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Synchronization payload kind for acquired texture frames.
/// </summary>
/// <remarks>
/// See <c>mln_gpu_sync_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public enum GpuSyncKind : uint
{
    /// <summary>
    /// The host needs no synchronization object. The work completed, or on
    /// WebGPU was submitted to the device's queue, before the frame became
    /// acquirable or before the release call.
    /// </summary>
    CpuComplete = 0,

    /// <summary>
    /// <c>id&lt;MTLSharedEvent&gt;</c> plus a monotonically increasing signal
    /// value.
    /// </summary>
    MetalSharedEvent = 1,

    /// <summary>
    /// VkSemaphore plus a timeline value.
    /// </summary>
    VulkanTimelineSemaphore = 2,

    /// <summary>
    /// GLsync, used only by a caller-graphics-thread driver.
    /// </summary>
    OpenglFence = 3,

    /// <summary>
    /// A backend-defined WebGPU completion token.
    /// </summary>
    WebgpuToken = 4,
}
