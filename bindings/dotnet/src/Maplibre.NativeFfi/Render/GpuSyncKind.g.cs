// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi.Render;

public enum GpuSyncKind : uint
{
    CpuComplete = 0,
    MetalSharedEvent = 1,
    VulkanTimelineSemaphore = 2,
    OpenglFence = 3,
    WebgpuToken = 4,
}
