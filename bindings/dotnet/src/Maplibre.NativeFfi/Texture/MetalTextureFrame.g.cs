// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The Metal texture of an acquired frame.
/// </summary>
/// <remarks>
/// See <c>mln_metal_texture_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Slot">
/// The texture's index in the descriptor's <c>textures</c>.
/// </param>
/// <param name="Texture">
/// The MTLTexture.
/// </param>
/// <param name="Device">
/// The texture's MTLDevice.
/// </param>
/// <param name="PixelFormat">
/// The texture's MTLPixelFormat.
/// </param>
public readonly partial record struct MetalTextureFrame(
    ulong Generation,
    uint Width,
    uint Height,
    double ScaleFactor,
    ulong FrameId,
    uint Slot,
    NativePointer Texture,
    NativePointer Device,
    ulong PixelFormat
);
