// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of textures that the session allocates on its device and shares with
/// you, in one of the ways that you can import.
/// </summary>
/// <remarks>
/// <para>Each texture has the size in pixels of <c>logical_size</c> at the
/// map's scale factor. <c>RenderSessionHandle.ResizeAsync()</c> gives the ring
/// textures of the new size, and a frame that you hold keeps its texture of the
/// old size until you release or discard it.</para>
/// <para>The ring shares its textures in the first kind of <c>export_kinds</c>
/// that the device exports, in this order, and
/// <c>RenderSessionCapabilities.ExportKind</c> reports it:</para>
/// <list type="bullet">
/// <item><description><c>GpuExportKind.IoSurface</c>: IOSurfaces of the pixel
/// format kCVPixelFormatType_32BGRA, which Metal names
/// MTLPixelFormatBGRA8Unorm. Get each frame's surface with
/// <c>AcquiredFrameHandle.WithIoSurface()</c>.</description></item>
/// <item><description><c>GpuExportKind.AndroidHardwareBuffer</c>: hardware
/// buffers of the format AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM, with the usage
/// AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT,
/// AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, and the bits of
/// <c>android_hardware_buffer_usage</c>. Get each frame's buffer with
/// <c>AcquiredFrameHandle.WithAndroidHardwareBuffer()</c>.</description></item>
/// <item><description><c>GpuExportKind.DxgiSharedHandle</c>: D3D11 textures of
/// the format DXGI_FORMAT_B8G8R8A8_UNORM on the device's GPU, which bind as
/// shader resources and render targets. Open them on a D3D11 or D3D12 device of
/// the same adapter, which you pick with <c>GpuDeviceOptions.AdapterLuid</c>.
/// Get each frame's texture with
/// <c>AcquiredFrameHandle.WithDxgiSharedTexture()</c>.</description></item>
/// <item><description><c>GpuExportKind.DmaBuf</c>: dmabufs of a format and
/// modifier from <c>dma_buf_formats</c>. Get each frame's buffer with
/// <c>AcquiredFrameHandle.WithDmaBuf()</c>.</description></item>
/// <item><description><c>GpuExportKind.OpaqueFd</c>: the memory of Vulkan
/// images, which only a Vulkan device or an OpenGL context with
/// GL_EXT_memory_object_fd on the same GPU and driver imports. To pick the GPU,
/// create the device with <c>GpuDeviceOptions.AdapterUuid</c> set to your
/// context's GL_DEVICE_UUID_EXT. Get each frame's memory with
/// <c>AcquiredFrameHandle.WithOpaqueFd()</c>.</description></item>
/// </list>
/// <para><c>GpuDeviceCapabilities.ExportKinds</c> reports the kinds that a
/// device exports. Attach with <c>RenderDriverKind.CoreWorker</c>. Without a
/// device, the session renders on the first of the platform's backends whose
/// device exports one of <c>export_kinds</c>.</para>
/// <para>Each frame hands over with <c>GpuSyncKind.CpuComplete</c> or with the
/// sync kind that crosses APIs on its platform:
/// <c>GpuSyncKind.MetalSharedEvent</c> for an IOSurface, and
/// <c>GpuSyncKind.SyncFd</c> for the others, where the device has them. For a
/// reader that waits on neither, such as a Wayland compositor without explicit
/// synchronization or Flutter's <c>copyPixelBuffer</c>, leave them out of
/// <c>RenderSessionAttachOptions.ProducerSyncKinds</c>, so that each frame
/// finishes before you acquire it, and release each frame only once the reader
/// is done with its texture. Give the ring 3 textures, so the session can
/// render while you hold two frames.</para>
/// <para>See <c>mln_exported_ring_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct ExportedRingTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public ExportedRingTarget(
        LogicalSize LogicalSize,
        uint TextureCount,
        GpuExportKindFlag ExportKinds,
        ImmutableArray<DmaBufFormat> DmaBufFormats,
        ulong AndroidHardwareBufferUsage
    )
    {
        this.LogicalSize = LogicalSize;
        this.TextureCount = TextureCount;
        this.ExportKinds = ExportKinds;
        this.DmaBufFormats = DmaBufFormats;
        this.AndroidHardwareBufferUsage = AndroidHardwareBufferUsage;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// How many textures the ring has, from 1 to
    /// <c>MaplibreNative.TextureRingMaxDepth</c>.
    /// </summary>
    public required uint TextureCount { get; init; }

    /// <summary>
    /// The ways of sharing a texture that you can import.
    /// </summary>
    public required GpuExportKindFlag ExportKinds { get; init; }

    /// <summary>
    /// The dmabuf formats and modifiers that you import, most preferred first,
    /// such as those that eglQueryDmaBufModifiersEXT() reports. A ring exports
    /// dmabufs only of one of them. The library renders only
    /// DRM_FORMAT_ABGR8888 and DRM_FORMAT_ARGB8888. A modifier can give a
    /// buffer more planes than its format has, such as a plane of compression
    /// metadata, and the library takes any such modifier that its GPU can
    /// render into and share. The library takes no implicit modifier: it skips
    /// DRM_FORMAT_MOD_INVALID, so a list of only that one gets no dmabuf.
    /// </summary>
    public ImmutableArray<DmaBufFormat> DmaBufFormats
    {
        get => DmaBufFormatsStorage.AsImmutable();
        init => DmaBufFormatsStorage = ValueArray.From(value);
    }
    internal ValueArray<DmaBufFormat> DmaBufFormatsStorage { get; init; }

    /// <summary>
    /// More <c>AHARDWAREBUFFER_USAGE_*</c> bits that your reads of a hardware
    /// buffer need, such as AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, or zero.
    /// </summary>
    public ulong AndroidHardwareBufferUsage { get; init; }
}
