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
/// Outcome of a successful render-update call. This is an open domain: a value may have no named
/// member here, so a switch over it needs a default case. Unknown values keep their raw value.
/// </summary>
public enum RenderResult : uint
{
    /// <summary>The call rendered a frame into the render target.</summary>
    Rendered = 0,

    /// <summary>
    /// The call produced no frame. Wait for a render-update-available event.
    /// </summary>
    NoUpdate = 1,

    /// <summary>
    /// The map has not applied the session's current size yet. Wait for the next
    /// render-update-available event.
    /// </summary>
    SizePending = 2,

    /// <summary>
    /// The render target had no frame to draw into. Wait for a host event that changes the render
    /// target, or back off and retry.
    /// </summary>
    TargetNotReady = 3,
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

public abstract class OpenGLContextDescriptor
{
    private protected OpenGLContextDescriptor() { }
}

public sealed class WglContextDescriptor : OpenGLContextDescriptor
{
    public NativePointer DeviceContext { get; set; }
    public NativePointer ShareContext { get; set; }
    public NativePointer GetProcAddress { get; set; }
}

public sealed class EglContextDescriptor : OpenGLContextDescriptor
{
    public NativePointer Display { get; set; }
    public NativePointer Config { get; set; }
    public NativePointer ShareContext { get; set; }
    public NativePointer GetProcAddress { get; set; }
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
    public NativePointer Surface { get; set; }
    public VulkanContextDescriptor? Context { get; set; }
}

public sealed class OpenGLSurfaceDescriptor
{
    public RenderTargetExtent Extent { get; set; }
    public NativePointer Surface { get; set; }
    public OpenGLContextDescriptor? Context { get; set; }
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
    public NativePointer Image { get; set; }
    public NativePointer ImageView { get; set; }
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
    private readonly NativePointer image;
    private readonly NativePointer imageView;
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
        NativePointer image,
        NativePointer imageView,
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
    public NativePointer Image
    {
        get
        {
            scope.EnsureActive();
            return image;
        }
    }
    public NativePointer ImageView
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
