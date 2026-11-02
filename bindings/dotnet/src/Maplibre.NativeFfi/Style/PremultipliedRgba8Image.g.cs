// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// An image of 8-bit RGBA pixels whose colors are multiplied by their alpha.
/// </summary>
/// <remarks>
/// See <c>mln_premultiplied_rgba8_image</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct PremultipliedRgba8Image
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public PremultipliedRgba8Image(
        uint Width,
        uint Height,
        uint Stride,
        ImmutableArray<byte> Pixels
    )
    {
        this.Width = Width;
        this.Height = Height;
        this.Stride = Stride;
        this.Pixels = Pixels;
    }

    public required uint Width { get; init; }
    public required uint Height { get; init; }

    /// <summary>
    /// The number of bytes from the start of one row to the start of the next,
    /// at least <c>width</c> * 4.
    /// </summary>
    public required uint Stride { get; init; }

    /// <summary>
    /// The rows of pixels, <c>stride</c> bytes apart, from the top row down.
    /// </summary>
    public required ImmutableArray<byte> Pixels
    {
        get => PixelsStorage.AsImmutable();
        init => PixelsStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> PixelsStorage { get; init; }
}
