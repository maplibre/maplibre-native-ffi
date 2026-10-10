// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Metal frame acquired from a session-owned texture target.
/// </summary>
/// <remarks>
/// See <c>mln_metal_owned_texture_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Generation">
/// Session generation that produced this frame.
/// </param>
/// <param name="Width">
/// Physical Metal texture width in device pixels.
/// </param>
/// <param name="Height">
/// Physical Metal texture height in device pixels.
/// </param>
/// <param name="ScaleFactor">
/// UI-to-device pixel scale used for this frame.
/// </param>
/// <param name="FrameId">
/// Opaque frame identity used to reject stale releases.
/// </param>
/// <param name="Texture">
/// Borrowed <c>id&lt;MTLTexture&gt;</c> / <c>MTL::Texture*</c>. Valid until
/// frame release.
/// </param>
/// <param name="Device">
/// Borrowed <c>id&lt;MTLDevice&gt;</c> / <c>MTL::Device*</c>. Valid until frame
/// release.
/// </param>
/// <param name="PixelFormat">
/// Backend-native pixel format value. Metal uses MTLPixelFormat.
/// </param>
public readonly partial record struct MetalOwnedTextureFrame(
    ulong Generation,
    uint Width,
    uint Height,
    double ScaleFactor,
    ulong FrameId,
    NativePointer Texture,
    NativePointer Device,
    ulong PixelFormat
);
