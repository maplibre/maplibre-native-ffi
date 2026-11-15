// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The memory of an exported ring's acquired frame, as an opaque file
/// descriptor.
/// </summary>
/// <remarks>
/// <para>The memory holds a 2D Vulkan image of <c>format</c>, with one mip
/// level, one layer, optimal tiling, and the usage
/// VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, and
/// VK_IMAGE_USAGE_TRANSFER_SRC_BIT, which a Vulkan image that imports it has
/// too. In OpenGL, import it with glImportMemoryFdEXT() and
/// GL_HANDLE_TYPE_OPAQUE_FD_EXT, and make the texture with
/// GL_TEXTURE_TILING_EXT set to GL_OPTIMAL_TILING_EXT and
/// glTexStorageMem2DEXT() at offset 0. Each frame leaves the image in
/// VK_IMAGE_LAYOUT_GENERAL, released to VK_QUEUE_FAMILY_EXTERNAL.</para>
/// <para>With <c>RenderSessionAttachOptions.KeepDepth</c>, on a device whose
/// <c>GpuDeviceCapabilities.DepthExportKinds</c> has
/// <c>GpuExportKind.OpaqueFd</c>, the depth members share the memory of the
/// frame's depth image in the same way. Otherwise <c>depth_fd</c> is -1 and the
/// others are zero. The depth image is 2D, of <c>depth_format</c>, with one mip
/// level, one layer, optimal tiling, and the usage
/// VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, and
/// VK_IMAGE_USAGE_TRANSFER_SRC_BIT. Each frame leaves it in
/// VK_IMAGE_LAYOUT_GENERAL, released to VK_QUEUE_FAMILY_EXTERNAL. In OpenGL,
/// make its texture with the internal format that <c>DepthStencilFormatFlag</c>
/// names for <c>depth_format</c>.</para>
/// <para>See <c>mln_opaque_fd_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Slot">
/// The texture's index in the ring.
/// </param>
/// <param name="Fd">
/// The file descriptor of the image's memory.
/// </param>
/// <param name="AllocationSize">
/// The size of the memory in bytes.
/// </param>
/// <param name="Dedicated">
/// Whether the memory is the image's own, which an import says, such as with
/// GL_DEDICATED_MEMORY_OBJECT_EXT.
/// </param>
/// <param name="Format">
/// The image's <c>VkFormat</c>: VK_FORMAT_R8G8B8A8_UNORM, OpenGL's GL_RGBA8.
/// </param>
/// <param name="DepthFd">
/// The file descriptor of the depth image's memory.
/// </param>
/// <param name="DepthAllocationSize">
/// The size of the depth image's memory in bytes.
/// </param>
/// <param name="DepthDedicated">
/// Whether the memory is the depth image's own.
/// </param>
/// <param name="DepthFormat">
/// The depth image's <c>VkFormat</c>.
/// </param>
public readonly partial record struct OpaqueFdFrame(
    uint Width,
    uint Height,
    double ScaleFactor,
    uint Slot,
    int Fd,
    ulong AllocationSize,
    bool Dedicated,
    uint Format,
    int DepthFd,
    ulong DepthAllocationSize,
    bool DepthDedicated,
    uint DepthFormat
);
