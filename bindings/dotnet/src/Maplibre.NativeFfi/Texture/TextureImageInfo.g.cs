// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The layout of the pixels that <c>RenderSessionHandle.ReadTextureAsync()</c>
/// reads.
/// </summary>
/// <remarks>
/// See <c>mln_texture_image_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Stride">
/// The bytes from the start of one row to the start of the next.
/// </param>
public readonly partial record struct TextureImageInfo(
    uint Width,
    uint Height,
    uint Stride,
    ulong ByteLength
);
