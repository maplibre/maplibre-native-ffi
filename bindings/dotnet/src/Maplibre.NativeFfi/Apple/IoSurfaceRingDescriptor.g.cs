// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of IOSurfaces that the library allocates, for
/// <c>MapHandle.AttachIoSurfaceRing()</c>.
/// </summary>
/// <remarks>
/// <para>Each surface has the map's size in pixels, <c>logical_size</c> times
/// the map's scale factor, and the pixel format kCVPixelFormatType_32BGRA,
/// which Metal names MTLPixelFormatBGRA8Unorm.</para>
/// <para>See <c>mln_io_surface_ring_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct IoSurfaceRingDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public IoSurfaceRingDescriptor(
        LogicalSize LogicalSize,
        uint BufferCount,
        MetalContextDescriptor Context
    )
    {
        this.LogicalSize = LogicalSize;
        this.BufferCount = BufferCount;
        this.Context = Context;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// How many surfaces the ring has, from 1 to 8.
    /// </summary>
    public required uint BufferCount { get; init; }

    /// <summary>
    /// The MTLDevice to render with. A null device picks the system's default.
    /// </summary>
    public MetalContextDescriptor Context { get; init; }
}
