// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// A bit for each <c>GpuSyncKind</c>.
/// </summary>
/// <remarks>
/// See <c>mln_gpu_sync_kind_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum GpuSyncKindFlag : uint
{
    CpuComplete = 1,
    OpenGlFence = 2,
    QueueOrder = 4,
    VulkanTimelineSemaphore = 8,
    SyncFd = 16,
    MetalSharedEvent = 32,
    D3d12Fence = 64,
}
