namespace Maplibre.Native.Render;

public enum RenderMode : uint { Partial = 0, Full = 1 }

public readonly record struct RenderTargetExtent(uint Width, uint Height, double ScaleFactor);
public readonly record struct TextureImageInfo(uint Width, uint Height, uint Stride, ulong ByteLength);
public sealed record PremultipliedRgba8Image(byte[] Bytes, TextureImageInfo Info);

public sealed class MetalContextDescriptor { public NativePointer Device { get; set; } public NativePointer Queue { get; set; } }
public sealed class VulkanContextDescriptor { public NativePointer Instance { get; set; } public NativePointer PhysicalDevice { get; set; } public NativePointer Device { get; set; } public NativePointer Queue { get; set; } public uint GraphicsQueueFamilyIndex { get; set; } }
public sealed class MetalSurfaceDescriptor { public RenderTargetExtent Extent { get; set; } public NativePointer Layer { get; set; } public MetalContextDescriptor? Context { get; set; } }
public sealed class VulkanSurfaceDescriptor { public RenderTargetExtent Extent { get; set; } public NativePointer Surface { get; set; } public VulkanContextDescriptor? Context { get; set; } }
public sealed class MetalOwnedTextureDescriptor { public RenderTargetExtent Extent { get; set; } public MetalContextDescriptor? Context { get; set; } }
public sealed class MetalBorrowedTextureDescriptor { public RenderTargetExtent Extent { get; set; } public NativePointer Texture { get; set; } public MetalContextDescriptor? Context { get; set; } }
public sealed class VulkanOwnedTextureDescriptor { public RenderTargetExtent Extent { get; set; } public VulkanContextDescriptor? Context { get; set; } public uint Format { get; set; } public uint FinalLayout { get; set; } }
public sealed class VulkanBorrowedTextureDescriptor { public RenderTargetExtent Extent { get; set; } public NativePointer Image { get; set; } public VulkanContextDescriptor? Context { get; set; } public uint Format { get; set; } public uint FinalLayout { get; set; } }

public readonly record struct MetalOwnedTextureFrame(NativePointer Texture, uint Width, uint Height, double ScaleFactor, ulong Generation);
public readonly record struct VulkanOwnedTextureFrame(NativePointer Image, NativePointer Device, uint Width, uint Height, uint Format, uint Layout, ulong Generation);
