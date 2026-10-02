// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public readonly partial record struct TextureImageInfo(
    uint Width,
    uint Height,
    uint Stride,
    ulong ByteLength
)
{
    public static TextureImageInfo Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_texture_image_info_default");
            return GeneratedValues.CopyTextureImageInfo(
                NativeMethods.mln_texture_image_info_default()
            );
        }
    }
}
