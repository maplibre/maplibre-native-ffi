namespace Maplibre.Native.Render;

public enum RenderMode : uint
{
    Partial = 0,
    Full = 1,
}

/// <summary>Render backend support flags reported by the native library.</summary>
[Flags]
public enum RenderBackend : uint
{
    None = 0,
    Metal = 1u << 0,
    Vulkan = 1u << 1,
    OpenGL = 1u << 2,
}

/// <summary>OpenGL context providers reported by the native library.</summary>
[Flags]
public enum OpenGLContextProvider : uint
{
    None = 0,
    Wgl = 1u << 0,
    Egl = 1u << 1,
}

public readonly record struct RenderTargetExtent(uint Width, uint Height, double ScaleFactor);

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
