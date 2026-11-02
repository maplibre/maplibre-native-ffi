// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of Metal textures that the session creates, for
/// <c>MapHandle.AttachMetalOwnedTexture()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_metal_owned_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct MetalOwnedTextureDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public MetalOwnedTextureDescriptor(
        LogicalExtent Extent,
        MetalContextDescriptor Context,
        ulong PixelFormat,
        NativePointer CommandQueue
    )
    {
        this.Extent = Extent;
        this.Context = Context;
        this.PixelFormat = PixelFormat;
        this.CommandQueue = CommandQueue;
    }

    public required LogicalExtent Extent { get; init; }

    /// <summary>
    /// Names the MTLDevice that creates the textures.
    /// </summary>
    public MetalContextDescriptor Context { get; init; }

    /// <summary>
    /// The textures' MTLPixelFormat: MTLPixelFormatRGBA8Unorm or
    /// MTLPixelFormatBGRA8Unorm.
    /// </summary>
    public required ulong PixelFormat { get; init; }

    /// <summary>
    /// Your MTLCommandQueue on the same device, or null for a queue of the
    /// session's own. With your queue, frames can use
    /// <c>GpuSyncKind.QueueOrder</c>.
    /// </summary>
    public NativePointer CommandQueue { get; init; }
}
