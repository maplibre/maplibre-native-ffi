using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Loader;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;

namespace Maplibre.NativeFfi.Render;

public enum RenderMode : uint
{
    Partial = 0,
    Full = 1,
}

/// <summary>
/// Terminal disposition of one accepted frame demand. Unknown values keep their raw value.
/// </summary>
public enum RenderResult : uint
{
    /// <summary>The session rendered a frame into the render target.</summary>
    Rendered = 0,

    /// <summary>No newer map update was available.</summary>
    NoUpdate = 1,

    /// <summary>An ordered extent change had not reached the driver.</summary>
    SizePending = 2,

    /// <summary>The target could not produce a frame.</summary>
    TargetNotReady = 3,

    /// <summary>A newer demand replaced this demand in its coalescing boundary.</summary>
    Superseded = 4,

    /// <summary>The demand's timeout elapsed before work began.</summary>
    DeadlineMissed = 5,
}

public enum RenderSessionState : uint
{
    Attaching = 1,
    Attached = 2,
    Detaching = 3,
    Detached = 4,
    TargetLost = 5,
    Abandoned = 6,
}

public enum RenderDriverKind : uint
{
    CoreWorker = 1,
    CallerGraphicsThread = 2,
}

[Flags]
public enum RenderSessionCapabilities : uint
{
    None = 0,
    FrameAcquisition = 1u << 0,
    Readback = 1u << 1,
    ConsumerSync = 1u << 2,
    Presentation = 1u << 3,
}

[Flags]
public enum FrameDemandFlags : uint
{
    None = 0,
    IfNeeded = 1u << 0,
    Present = 1u << 1,
}

public readonly record struct FrameDemand(
    FrameDemandFlags Flags,
    ulong Token,
    ulong CoalescingBoundary,
    ulong TimeoutNanoseconds
);

/// <summary>
/// One frame demand's outcome. <see cref="NeedsRepaint"/> is meaningful only when
/// <see cref="Disposition"/> is <see cref="RenderResult.Rendered"/>; it is true when the map
/// asked for another frame while it rendered this one, as during an ongoing camera transition.
/// </summary>
public readonly record struct RenderFrameResult(
    RenderResult Disposition,
    ulong Token,
    ulong MapUpdateGeneration,
    ulong ExtentGeneration,
    ulong FrameGeneration,
    bool NeedsRepaint
);

public readonly record struct RenderSessionSnapshot(
    RenderSessionState State,
    RenderDriverKind Driver,
    RenderResult LatestResult,
    RenderTargetExtent Extent,
    ulong Generation,
    ulong MapUpdateGeneration,
    ulong RenderedUpdateGeneration,
    ulong ExtentGeneration,
    ulong FrameGeneration,
    ulong LatestDemandToken,
    uint PendingDemandCount,
    uint AcquiredFrameCount,
    bool TargetReady,
    bool PendingChanges
);

public readonly record struct RenderSessionCapabilityInfo(
    RenderDriverKind Driver,
    uint TextureRingDepth,
    RenderSessionCapabilities Flags
);

public enum GpuSyncKind : uint
{
    CpuComplete = 0,
    MetalSharedEvent = 1,
    VulkanTimelineSemaphore = 2,
    OpenGLFence = 3,
    WebGpuToken = 4,
}

/// <summary>
/// Backend synchronization copied by frame access and release calls.
/// </summary>
/// <param name="Kind">Which backend primitive <paramref name="ObjectBits"/> names.</param>
/// <param name="ObjectBits">
/// Bit pattern of the backend object that <paramref name="Kind"/> names: the
/// <c>id&lt;MTLSharedEvent&gt;</c> pointer, the <c>VkSemaphore</c> handle, the <c>GLsync</c>
/// pointer, or the WebGPU token. A Vulkan handle stays 64 bits wide even where a pointer is not.
/// Zero when <paramref name="Kind"/> is <see cref="GpuSyncKind.CpuComplete"/>.
/// </param>
/// <param name="Value">The signal or timeline value the backend primitive waits on.</param>
public readonly record struct GpuSync(GpuSyncKind Kind, ulong ObjectBits, ulong Value)
{
    public static GpuSync CpuComplete => new(GpuSyncKind.CpuComplete, 0, 0);
}

public enum RenderAbandonDisposition : uint
{
    Clean = 0,
    Quarantined = 1,
}

public readonly record struct RenderAbandonResult(
    RenderAbandonDisposition Disposition,
    uint QuarantinedResourceCount
);

public sealed record RenderSessionAttachOptions
{
    public RenderDriverKind Driver { get; init; } = RenderDriverKind.CoreWorker;
    public uint RequestedTextureRingDepth { get; init; }
}

/// <summary>Render backend support flags reported by the native library.</summary>
[Flags]
public enum RenderBackend : uint
{
    None = 0,
    Metal = 1u << 0,
    Vulkan = 1u << 1,
    OpenGL = 1u << 2,
    WebGpu = 1u << 3,
}

/// <summary>OpenGL context providers reported by the native library.</summary>
[Flags]
public enum OpenGLContextProvider : uint
{
    None = 0,
    Wgl = 1u << 0,
    Egl = 1u << 1,
    WebGl = 1u << 2,
}

public readonly record struct RenderTargetExtent(uint Width, uint Height, double ScaleFactor)
{
    /// <summary>
    /// Returns the physical device-pixel size as ceil(logical * <see cref="ScaleFactor"/>) per
    /// dimension, which is how surface and session-owned texture targets are sized. Borrowed
    /// texture targets state their physical size instead.
    /// </summary>
    public unsafe (uint Width, uint Height) PhysicalSize()
    {
        NativeLibraryLoader.EnsureLoaded();
        var native = RenderStructs.ToNative(this);
        uint width;
        uint height;
        NativeStatus.Check(
            NativeMethods.mln_render_target_extent_physical_size(&native, &width, &height)
        );
        return (width, height);
    }
}

public readonly record struct TextureImageInfo(
    uint Width,
    uint Height,
    uint Stride,
    ulong ByteLength
);

public sealed record PremultipliedRgba8Image
{
    private readonly byte[] bytes;

    public PremultipliedRgba8Image(byte[] Bytes, TextureImageInfo Info)
    {
        bytes = Bytes is null ? [] : (byte[])Bytes.Clone();
        this.Info = Info;
    }

    public byte[] Bytes => (byte[])bytes.Clone();

    internal byte[] BytesTransit => bytes;

    public TextureImageInfo Info { get; }

    public bool Equals(PremultipliedRgba8Image? other) =>
        other is not null && Info == other.Info && bytes.AsSpan().SequenceEqual(other.bytes);

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(Info);
        hash.AddBytes(bytes);
        return hash.ToHashCode();
    }
}

public sealed class MetalContextDescriptor
{
    public NativePointer Device { get; set; }
}

public sealed class VulkanContextDescriptor
{
    public NativePointer Instance { get; set; }
    public NativePointer PhysicalDevice { get; set; }
    public NativePointer Device { get; set; }
    public NativePointer Queue { get; set; }
    public uint GraphicsQueueFamilyIndex { get; set; }
    public NativePointer GetInstanceProcAddr { get; set; }
    public NativePointer GetDeviceProcAddr { get; set; }
}

/// <summary>
/// How a render session's OpenGL context relates to its driver thread and host graphics state. This
/// is an open domain: a value may have no named member here, so a switch over it needs a default
/// case. Unknown values keep their raw value.
/// </summary>
public enum OpenGLContextOwnership : uint
{
    /// <summary>
    /// The session shares its thread with host graphics work. Every render makes the session
    /// context current and restores whatever was current before, and the session context joins the
    /// share group named by the descriptor.
    /// </summary>
    Shared = 0,

    /// <summary>
    /// The session owns its driver thread's OpenGL context. It keeps the context current between
    /// renders and joins no share group. The driver may be a native core worker or a dedicated host
    /// thread.
    /// </summary>
    Dedicated = 1,
}

/// <summary>
/// OpenGL client API a dedicated EGL session creates its context for. This is an open domain: a
/// value may have no named member here, so a switch over it needs a default case. Unknown values
/// keep their raw value.
/// </summary>
public enum OpenGLClientApi : uint
{
    /// <summary>No client API is named.</summary>
    Unspecified = 0,

    /// <summary>Desktop OpenGL, as EGL_OPENGL_API names it.</summary>
    Gl = 1,

    /// <summary>OpenGL ES, as EGL_OPENGL_ES_API names it.</summary>
    Gles = 2,
}

public abstract class OpenGLContextDescriptor
{
    private protected OpenGLContextDescriptor() { }

    /// <summary>
    /// Whether the session shares its driver thread and graphics objects with the host. A private
    /// EGL owned texture target uses dedicated ownership and grants readback without acquisition.
    /// </summary>
    public OpenGLContextOwnership Ownership { get; set; } = OpenGLContextOwnership.Shared;
}

public sealed class WglContextDescriptor : OpenGLContextDescriptor
{
    public NativePointer DeviceContext { get; set; }

    /// <summary>
    /// Context whose share group the session context joins. Required under
    /// <see cref="OpenGLContextOwnership.Shared"/>, and null under
    /// <see cref="OpenGLContextOwnership.Dedicated"/>.
    /// </summary>
    public NativePointer ShareContext { get; set; }
    public NativePointer GetProcAddress { get; set; }
}

public sealed class EglContextDescriptor : OpenGLContextDescriptor
{
    public NativePointer Display { get; set; }
    public NativePointer Config { get; set; }

    /// <summary>
    /// Context whose share group the session context joins. Required under
    /// <see cref="OpenGLContextOwnership.Shared"/>, where the session also takes its client API
    /// from this context, and null under <see cref="OpenGLContextOwnership.Dedicated"/>.
    /// </summary>
    public NativePointer ShareContext { get; set; }

    /// <summary>
    /// Client API the session creates its context for. Required under
    /// <see cref="OpenGLContextOwnership.Dedicated"/>, and ignored under
    /// <see cref="OpenGLContextOwnership.Shared"/>.
    /// </summary>
    public OpenGLClientApi ClientApi { get; set; } = OpenGLClientApi.Unspecified;
    public NativePointer GetProcAddress { get; set; }
}

public enum WebGLContextKind : uint
{
    Existing = 0,
    TransferredCanvas = 1,
}

public sealed class WebGLContextDescriptor : OpenGLContextDescriptor
{
    public WebGLContextKind Kind { get; set; }
    public int Context { get; set; }
    public string CanvasSelector { get; set; } = string.Empty;
}

public sealed class WebGpuContextDescriptor
{
    public NativePointer Instance { get; set; }
    public NativePointer Device { get; set; }
    public NativePointer Queue { get; set; }
}

public sealed class MetalSurfaceDescriptor
{
    public RenderTargetExtent Extent { get; set; }
    public NativePointer Layer { get; set; }
    public MetalContextDescriptor? Context { get; set; }
}

public sealed class VulkanSurfaceDescriptor
{
    public RenderTargetExtent Extent { get; set; }
    public VulkanHandle Surface { get; set; }
    public VulkanContextDescriptor? Context { get; set; }
}

public sealed class OpenGLSurfaceDescriptor
{
    public RenderTargetExtent Extent { get; set; }
    public NativePointer Surface { get; set; }
    public OpenGLContextDescriptor? Context { get; set; }
}

public sealed class WebGpuSurfaceDescriptor
{
    public RenderTargetExtent Extent { get; set; }
    public WebGpuContextDescriptor? Context { get; set; }
    public NativePointer Surface { get; set; }
    public uint Format { get; set; }
}

public sealed class MetalOwnedTextureDescriptor
{
    public RenderTargetExtent Extent { get; set; }
    public MetalContextDescriptor? Context { get; set; }
}

public sealed class MetalBorrowedTextureDescriptor
{
    public RenderTargetExtent Extent { get; set; }

    /// <summary>
    /// Physical texture size in device pixels. The texture is sized by its owner, so this is
    /// stated rather than derived from <see cref="Extent"/>.
    /// </summary>
    public uint PhysicalWidth { get; set; }
    public uint PhysicalHeight { get; set; }
    public NativePointer Texture { get; set; }
}

public sealed class VulkanOwnedTextureDescriptor
{
    public RenderTargetExtent Extent { get; set; }
    public VulkanContextDescriptor? Context { get; set; }
}

public sealed class VulkanBorrowedTextureDescriptor
{
    public RenderTargetExtent Extent { get; set; }

    /// <summary>
    /// Physical image size in device pixels. The image is sized by its owner, so this is
    /// stated rather than derived from <see cref="Extent"/>.
    /// </summary>
    public uint PhysicalWidth { get; set; }
    public uint PhysicalHeight { get; set; }
    public VulkanHandle Image { get; set; }
    public VulkanHandle ImageView { get; set; }
    public VulkanContextDescriptor? Context { get; set; }
    public uint Format { get; set; }
    public uint InitialLayout { get; set; }
    public uint FinalLayout { get; set; }
}

public sealed class OpenGLOwnedTextureDescriptor
{
    public RenderTargetExtent Extent { get; set; }
    public OpenGLContextDescriptor? Context { get; set; }
}

public sealed class OpenGLBorrowedTextureDescriptor
{
    public RenderTargetExtent Extent { get; set; }

    /// <summary>
    /// Physical texture size in device pixels. The texture is sized by its owner, so this is
    /// stated rather than derived from <see cref="Extent"/>.
    /// </summary>
    public uint PhysicalWidth { get; set; }
    public uint PhysicalHeight { get; set; }
    public OpenGLContextDescriptor? Context { get; set; }
    public uint Texture { get; set; }
    public uint Target { get; set; }
}

public sealed class WebGpuOwnedTextureDescriptor
{
    public RenderTargetExtent Extent { get; set; }
    public WebGpuContextDescriptor? Context { get; set; }
}

public sealed class WebGpuBorrowedTextureDescriptor
{
    public RenderTargetExtent Extent { get; set; }
    public uint PhysicalWidth { get; set; }
    public uint PhysicalHeight { get; set; }
    public WebGpuContextDescriptor? Context { get; set; }
    public NativePointer Texture { get; set; }
    public NativePointer TextureView { get; set; }
    public uint Format { get; set; }
}

public sealed class WebGpuOwnedTextureFrame
{
    private readonly FrameScope scope;
    private readonly ulong generation;
    private readonly uint width;
    private readonly uint height;
    private readonly double scaleFactor;
    private readonly ulong frameId;
    private readonly NativePointer texture;
    private readonly NativePointer textureView;
    private readonly NativePointer device;
    private readonly uint format;

    internal WebGpuOwnedTextureFrame(
        FrameScope scope,
        ulong generation,
        uint width,
        uint height,
        double scaleFactor,
        ulong frameId,
        NativePointer texture,
        NativePointer textureView,
        NativePointer device,
        uint format
    )
    {
        this.scope = scope;
        this.generation = generation;
        this.width = width;
        this.height = height;
        this.scaleFactor = scaleFactor;
        this.frameId = frameId;
        this.texture = texture;
        this.textureView = textureView;
        this.device = device;
        this.format = format;
    }

    public ulong Generation
    {
        get
        {
            scope.EnsureActive();
            return generation;
        }
    }
    public uint Width
    {
        get
        {
            scope.EnsureActive();
            return width;
        }
    }
    public uint Height
    {
        get
        {
            scope.EnsureActive();
            return height;
        }
    }
    public double ScaleFactor
    {
        get
        {
            scope.EnsureActive();
            return scaleFactor;
        }
    }
    public ulong FrameId
    {
        get
        {
            scope.EnsureActive();
            return frameId;
        }
    }
    public NativePointer Texture
    {
        get
        {
            scope.EnsureActive();
            return texture;
        }
    }
    public NativePointer TextureView
    {
        get
        {
            scope.EnsureActive();
            return textureView;
        }
    }
    public NativePointer Device
    {
        get
        {
            scope.EnsureActive();
            return device;
        }
    }
    public uint Format
    {
        get
        {
            scope.EnsureActive();
            return format;
        }
    }
}

public sealed class MetalOwnedTextureFrame
{
    private readonly FrameScope scope;
    private readonly ulong generation;
    private readonly uint width;
    private readonly uint height;
    private readonly double scaleFactor;
    private readonly ulong frameId;
    private readonly NativePointer texture;
    private readonly NativePointer device;
    private readonly ulong pixelFormat;

    internal MetalOwnedTextureFrame(
        FrameScope scope,
        ulong generation,
        uint width,
        uint height,
        double scaleFactor,
        ulong frameId,
        NativePointer texture,
        NativePointer device,
        ulong pixelFormat
    )
    {
        this.scope = scope;
        this.generation = generation;
        this.width = width;
        this.height = height;
        this.scaleFactor = scaleFactor;
        this.frameId = frameId;
        this.texture = texture;
        this.device = device;
        this.pixelFormat = pixelFormat;
    }

    public ulong Generation
    {
        get
        {
            scope.EnsureActive();
            return generation;
        }
    }
    public uint Width
    {
        get
        {
            scope.EnsureActive();
            return width;
        }
    }
    public uint Height
    {
        get
        {
            scope.EnsureActive();
            return height;
        }
    }
    public double ScaleFactor
    {
        get
        {
            scope.EnsureActive();
            return scaleFactor;
        }
    }
    public ulong FrameId
    {
        get
        {
            scope.EnsureActive();
            return frameId;
        }
    }
    public NativePointer Texture
    {
        get
        {
            scope.EnsureActive();
            return texture;
        }
    }
    public NativePointer Device
    {
        get
        {
            scope.EnsureActive();
            return device;
        }
    }
    public ulong PixelFormat
    {
        get
        {
            scope.EnsureActive();
            return pixelFormat;
        }
    }
}

public sealed class VulkanOwnedTextureFrame
{
    private readonly FrameScope scope;
    private readonly ulong generation;
    private readonly uint width;
    private readonly uint height;
    private readonly double scaleFactor;
    private readonly ulong frameId;
    private readonly VulkanHandle image;
    private readonly VulkanHandle imageView;
    private readonly NativePointer device;
    private readonly uint format;
    private readonly uint layout;

    internal VulkanOwnedTextureFrame(
        FrameScope scope,
        ulong generation,
        uint width,
        uint height,
        double scaleFactor,
        ulong frameId,
        VulkanHandle image,
        VulkanHandle imageView,
        NativePointer device,
        uint format,
        uint layout
    )
    {
        this.scope = scope;
        this.generation = generation;
        this.width = width;
        this.height = height;
        this.scaleFactor = scaleFactor;
        this.frameId = frameId;
        this.image = image;
        this.imageView = imageView;
        this.device = device;
        this.format = format;
        this.layout = layout;
    }

    public ulong Generation
    {
        get
        {
            scope.EnsureActive();
            return generation;
        }
    }
    public uint Width
    {
        get
        {
            scope.EnsureActive();
            return width;
        }
    }
    public uint Height
    {
        get
        {
            scope.EnsureActive();
            return height;
        }
    }
    public double ScaleFactor
    {
        get
        {
            scope.EnsureActive();
            return scaleFactor;
        }
    }
    public ulong FrameId
    {
        get
        {
            scope.EnsureActive();
            return frameId;
        }
    }
    public VulkanHandle Image
    {
        get
        {
            scope.EnsureActive();
            return image;
        }
    }
    public VulkanHandle ImageView
    {
        get
        {
            scope.EnsureActive();
            return imageView;
        }
    }
    public NativePointer Device
    {
        get
        {
            scope.EnsureActive();
            return device;
        }
    }
    public uint Format
    {
        get
        {
            scope.EnsureActive();
            return format;
        }
    }
    public uint Layout
    {
        get
        {
            scope.EnsureActive();
            return layout;
        }
    }
}

public sealed class OpenGLOwnedTextureFrame
{
    private readonly FrameScope scope;
    private readonly ulong generation;
    private readonly uint width;
    private readonly uint height;
    private readonly double scaleFactor;
    private readonly ulong frameId;
    private readonly uint texture;
    private readonly uint target;
    private readonly uint internalFormat;
    private readonly uint format;
    private readonly uint type;

    internal OpenGLOwnedTextureFrame(
        FrameScope scope,
        ulong generation,
        uint width,
        uint height,
        double scaleFactor,
        ulong frameId,
        uint texture,
        uint target,
        uint internalFormat,
        uint format,
        uint type
    )
    {
        this.scope = scope;
        this.generation = generation;
        this.width = width;
        this.height = height;
        this.scaleFactor = scaleFactor;
        this.frameId = frameId;
        this.texture = texture;
        this.target = target;
        this.internalFormat = internalFormat;
        this.format = format;
        this.type = type;
    }

    public ulong Generation
    {
        get
        {
            scope.EnsureActive();
            return generation;
        }
    }
    public uint Width
    {
        get
        {
            scope.EnsureActive();
            return width;
        }
    }
    public uint Height
    {
        get
        {
            scope.EnsureActive();
            return height;
        }
    }
    public double ScaleFactor
    {
        get
        {
            scope.EnsureActive();
            return scaleFactor;
        }
    }
    public ulong FrameId
    {
        get
        {
            scope.EnsureActive();
            return frameId;
        }
    }
    public uint Texture
    {
        get
        {
            scope.EnsureActive();
            return texture;
        }
    }
    public uint Target
    {
        get
        {
            scope.EnsureActive();
            return target;
        }
    }
    public uint InternalFormat
    {
        get
        {
            scope.EnsureActive();
            return internalFormat;
        }
    }
    public uint Format
    {
        get
        {
            scope.EnsureActive();
            return format;
        }
    }
    public uint Type
    {
        get
        {
            scope.EnsureActive();
            return type;
        }
    }
}
