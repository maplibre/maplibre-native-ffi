namespace Maplibre.Native.Render;

public enum RenderMode : uint { Partial = 0, Full = 1 }

public readonly record struct RenderTargetExtent(uint Width, uint Height, double ScaleFactor);
public readonly record struct TextureImageInfo(uint Width, uint Height, uint Stride, ulong ByteLength);
public sealed record PremultipliedRgba8Image(byte[] Bytes, TextureImageInfo Info);

public sealed class MetalContextDescriptor { public NativePointer Device { get; set; } }
public sealed class VulkanContextDescriptor { public NativePointer Instance { get; set; } public NativePointer PhysicalDevice { get; set; } public NativePointer Device { get; set; } public NativePointer Queue { get; set; } public uint GraphicsQueueFamilyIndex { get; set; } }
public sealed class MetalSurfaceDescriptor { public RenderTargetExtent Extent { get; set; } public NativePointer Layer { get; set; } public MetalContextDescriptor? Context { get; set; } }
public sealed class VulkanSurfaceDescriptor { public RenderTargetExtent Extent { get; set; } public NativePointer Surface { get; set; } public VulkanContextDescriptor? Context { get; set; } }
public sealed class MetalOwnedTextureDescriptor { public RenderTargetExtent Extent { get; set; } public MetalContextDescriptor? Context { get; set; } }
public sealed class MetalBorrowedTextureDescriptor { public RenderTargetExtent Extent { get; set; } public NativePointer Texture { get; set; } }
public sealed class VulkanOwnedTextureDescriptor { public RenderTargetExtent Extent { get; set; } public VulkanContextDescriptor? Context { get; set; } }
public sealed class VulkanBorrowedTextureDescriptor { public RenderTargetExtent Extent { get; set; } public NativePointer Image { get; set; } public NativePointer ImageView { get; set; } public VulkanContextDescriptor? Context { get; set; } public uint Format { get; set; } public uint InitialLayout { get; set; } public uint FinalLayout { get; set; } }

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

    internal MetalOwnedTextureFrame(FrameScope scope, ulong generation, uint width, uint height, double scaleFactor, ulong frameId, NativePointer texture, NativePointer device, ulong pixelFormat)
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

    public ulong Generation { get { scope.EnsureActive(); return generation; } }
    public uint Width { get { scope.EnsureActive(); return width; } }
    public uint Height { get { scope.EnsureActive(); return height; } }
    public double ScaleFactor { get { scope.EnsureActive(); return scaleFactor; } }
    public ulong FrameId { get { scope.EnsureActive(); return frameId; } }
    public NativePointer Texture { get { scope.EnsureActive(); return texture; } }
    public NativePointer Device { get { scope.EnsureActive(); return device; } }
    public ulong PixelFormat { get { scope.EnsureActive(); return pixelFormat; } }
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

    internal VulkanOwnedTextureFrame(FrameScope scope, ulong generation, uint width, uint height, double scaleFactor, ulong frameId, NativePointer image, NativePointer imageView, NativePointer device, uint format, uint layout)
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

    public ulong Generation { get { scope.EnsureActive(); return generation; } }
    public uint Width { get { scope.EnsureActive(); return width; } }
    public uint Height { get { scope.EnsureActive(); return height; } }
    public double ScaleFactor { get { scope.EnsureActive(); return scaleFactor; } }
    public ulong FrameId { get { scope.EnsureActive(); return frameId; } }
    public NativePointer Image { get { scope.EnsureActive(); return image; } }
    public NativePointer ImageView { get { scope.EnsureActive(); return imageView; } }
    public NativePointer Device { get { scope.EnsureActive(); return device; } }
    public uint Format { get { scope.EnsureActive(); return format; } }
    public uint Layout { get { scope.EnsureActive(); return layout; } }
}
