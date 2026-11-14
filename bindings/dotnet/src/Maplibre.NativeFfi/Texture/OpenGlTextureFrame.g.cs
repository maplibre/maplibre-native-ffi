// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The OpenGL texture of an acquired frame.
/// </summary>
/// <remarks>
/// <para>The depth members are set when the frame has a depth texture, as for
/// <c>MetalTextureFrame</c>. Otherwise they are zero.</para>
/// <para>See <c>mln_open_gl_texture_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Slot">
/// The texture's index in the descriptor's <c>textures</c>.
/// </param>
/// <param name="DepthTexture">
/// The <c>GL_TEXTURE_2D</c> texture that holds the frame's depth and stencil.
/// </param>
/// <param name="DepthInternalFormat">
/// The depth texture's internal format, such as GL_DEPTH24_STENCIL8, or zero
/// for a borrowed ring's.
/// </param>
public readonly partial record struct OpenGlTextureFrame(
    uint Width,
    uint Height,
    double ScaleFactor,
    uint Slot,
    uint Texture,
    uint Target,
    uint DepthTexture,
    uint DepthInternalFormat
);
