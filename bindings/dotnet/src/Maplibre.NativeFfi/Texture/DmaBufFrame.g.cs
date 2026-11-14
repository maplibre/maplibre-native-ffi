// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The dmabuf of an exported ring's acquired frame.
/// </summary>
/// <remarks>
/// See <c>mln_dma_buf_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct DmaBufFrame
{
    public DmaBufFrame(
        uint Width,
        uint Height,
        double ScaleFactor,
        uint Slot,
        uint Fourcc,
        ulong Modifier,
        ImmutableArray<DmaBufPlane> Planes
    )
    {
        this.Width = Width;
        this.Height = Height;
        this.ScaleFactor = ScaleFactor;
        this.Slot = Slot;
        this.Fourcc = Fourcc;
        this.Modifier = Modifier;
        this.Planes = Planes;
    }

    public uint Width { get; init; }
    public uint Height { get; init; }
    public double ScaleFactor { get; init; }

    /// <summary>
    /// The buffer's index in the ring.
    /// </summary>
    public uint Slot { get; init; }
    public uint Fourcc { get; init; }
    public ulong Modifier { get; init; }

    /// <summary>
    /// The buffer's planes, in the order that its fourcc and modifier define.
    /// Import all of them, as EGL_EXT_image_dma_buf_import_modifiers planes 0
    /// and up, or with one <c>zwp_linux_buffer_params_v1.add</c> call each.
    /// </summary>
    public ImmutableArray<DmaBufPlane> Planes
    {
        get => PlanesStorage.AsImmutable();
        init => PlanesStorage = ValueArray.From(value);
    }
    internal ValueArray<DmaBufPlane> PlanesStorage { get; init; }
}
