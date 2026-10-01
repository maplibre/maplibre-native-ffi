// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Style;

public readonly record struct PremultipliedRgba8Image
{
    public PremultipliedRgba8Image(uint Width, uint Height, uint Stride, byte[] Pixels)
    {
        this.Width = Width;
        this.Height = Height;
        this.Stride = Stride;
        this.Pixels = Pixels;
    }

    public uint Width { get; init; }
    public uint Height { get; init; }
    public uint Stride { get; init; }
    public byte[] Pixels
    {
        get => PixelsStorage.ToArray();
        init => PixelsStorage = ValueArray.Copy(value);
    }
    internal ValueArray<byte> PixelsStorage { get; init; }
    public static PremultipliedRgba8Image Default
    {
        get
        {
            using var call = Enter(null, "mln_premultiplied_rgba8_image_default");
            return CopyPremultipliedRgba8Image(
                NativeMethods.mln_premultiplied_rgba8_image_default()
            );
        }
    }
}
