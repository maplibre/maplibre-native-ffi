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
            using var call = Enter(null, "mln_texture_image_info_default");
            return CopyTextureImageInfo(NativeMethods.mln_texture_image_info_default());
        }
    }
}
