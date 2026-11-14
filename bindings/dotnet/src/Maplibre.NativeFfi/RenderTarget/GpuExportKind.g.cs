// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// A way to share a texture's memory with another API or process.
/// </summary>
/// <remarks>
/// See <c>mln_gpu_export_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public enum GpuExportKind : uint
{
    /// <summary>
    /// The texture isn't shared.
    /// </summary>
    None = 0,

    /// <summary>
    /// An <c>IOSurfaceRef</c>.
    /// </summary>
    IoSurface = 1,

    /// <summary>
    /// A Linux dmabuf, as file descriptors with a DRM format and modifier.
    /// </summary>
    DmaBuf = 2,

    /// <summary>
    /// An <c>AHardwareBuffer*</c>.
    /// </summary>
    AndroidHardwareBuffer = 3,

    /// <summary>
    /// A DXGI shared texture's NT handle.
    /// </summary>
    DxgiSharedHandle = 4,

    /// <summary>
    /// A Vulkan or OpenGL opaque memory file descriptor.
    /// </summary>
    OpaqueFd = 5,
}
