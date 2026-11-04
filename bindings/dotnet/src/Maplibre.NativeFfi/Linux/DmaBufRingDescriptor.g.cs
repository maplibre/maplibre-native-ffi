// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of dmabufs that the library allocates, for
/// <c>MapHandle.AttachDmaBufRing()</c>.
/// </summary>
/// <remarks>
/// <para>Each buffer has the map's size in pixels and a format and modifier
/// from <c>formats</c>. The library renders only DRM_FORMAT_ABGR8888 and
/// DRM_FORMAT_ARGB8888, and only with modifiers that keep one plane.</para>
/// <para>See <c>mln_dma_buf_ring_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct DmaBufRingDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public DmaBufRingDescriptor(
        LogicalSize LogicalSize,
        uint BufferCount,
        ImmutableArray<DmaBufFormat> Formats
    )
    {
        this.LogicalSize = LogicalSize;
        this.BufferCount = BufferCount;
        this.Formats = Formats;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// How many buffers the ring has, from 1 to 8.
    /// </summary>
    public required uint BufferCount { get; init; }

    /// <summary>
    /// The formats and modifiers that your reader imports, most preferred
    /// first, such as those that eglQueryDmaBufModifiersEXT() reports.
    /// </summary>
    public required ImmutableArray<DmaBufFormat> Formats
    {
        get => FormatsStorage.AsImmutable();
        init => FormatsStorage = ValueArray.From(value);
    }
    internal ValueArray<DmaBufFormat> FormatsStorage { get; init; }
}
