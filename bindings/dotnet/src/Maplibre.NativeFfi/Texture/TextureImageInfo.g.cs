// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// CPU image readback metadata for a texture target frame.
/// </summary>
/// <remarks>
/// See <c>mln_texture_image_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
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
