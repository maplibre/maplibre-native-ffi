// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The Metal texture of an acquired frame.
/// </summary>
/// <remarks>
/// <para>The depth members are set when the frame has a depth texture: one of a
/// borrowed ring's <c>depth_textures</c>, or an exported ring's with
/// <c>RenderSessionAttachOptions.KeepDepth</c>. Otherwise they are zero.</para>
/// <para>See <c>mln_metal_texture_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
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
/// <param name="DepthTexture">
/// The MTLTexture that holds the frame's depth and stencil.
/// </param>
/// <param name="DepthPixelFormat">
/// The depth texture's MTLPixelFormat.
/// </param>
public readonly partial record struct MetalTextureFrame(
    uint Width,
    uint Height,
    double ScaleFactor,
    uint Slot,
    NativePointer Texture,
    NativePointer Device,
    ulong PixelFormat,
    NativePointer DepthTexture,
    ulong DepthPixelFormat
);
