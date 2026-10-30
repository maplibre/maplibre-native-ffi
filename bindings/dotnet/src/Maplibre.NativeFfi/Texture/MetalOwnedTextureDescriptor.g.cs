// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Metal attachment options for an owned texture target.
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

    /// <summary>
    /// Logical texture extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    /// </summary>
    public required LogicalExtent Extent { get; init; }

    /// <summary>
    /// Metal backend context. device is required.
    /// </summary>
    public MetalContextDescriptor Context { get; init; }

    /// <summary>
    /// Backend-native MTLPixelFormat value of the ring's textures: either
    /// MTLPixelFormatRGBA8Unorm or MTLPixelFormatBGRA8Unorm. Required. Readback
    /// returns RGBA8 for either format.
    /// </summary>
    public required ulong PixelFormat { get; init; }

    /// <summary>
    /// <c>id&lt;MTLCommandQueue&gt;</c> / <c>MTL::CommandQueue*</c> on
    /// context.device that the session submits its work to, retained by the
    /// session. Optional; null makes the session create its own queue.
    /// <c>GpuSyncKind.QueueOrder</c> orders frames on this queue.
    /// </summary>
    public NativePointer CommandQueue { get; init; }
}
