// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Caller-owned premultiplied RGBA8 image pixels.
/// </summary>
/// <remarks>
/// See <c>mln_premultiplied_rgba8_image</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
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
            using var call = NativeCall.Enter(null, "mln_premultiplied_rgba8_image_default");
            return GeneratedValues.CopyPremultipliedRgba8Image(
                NativeMethods.mln_premultiplied_rgba8_image_default()
            );
        }
    }
}
