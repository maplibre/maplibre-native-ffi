// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// OpenGL frame acquired from a texture ring.
/// </summary>
/// <remarks>
/// See <c>mln_opengl_texture_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Generation">
/// Session generation that produced this frame.
/// </param>
/// <param name="Width">
/// Physical OpenGL texture width in device pixels.
/// </param>
/// <param name="Height">
/// Physical OpenGL texture height in device pixels.
/// </param>
/// <param name="ScaleFactor">
/// UI-to-device pixel scale used for this frame.
/// </param>
/// <param name="FrameId">
/// Opaque frame identity used to reject stale releases.
/// </param>
/// <param name="Slot">
/// Ring slot that holds this frame. For a borrowed target, the index of its
/// texture in the descriptor's textures array.
/// </param>
/// <param name="Texture">
/// Borrowed OpenGL texture object name. Valid until frame release.
/// </param>
/// <param name="Target">
/// OpenGL texture target. GL_TEXTURE_2D is the expected target.
/// </param>
/// <param name="InternalFormat">
/// OpenGL internal format, such as GL_RGBA8. Zero for a borrowed texture, whose
/// format the host chose.
/// </param>
/// <param name="Format">
/// OpenGL pixel format, such as GL_RGBA. Zero for a borrowed texture.
/// </param>
/// <param name="Type">
/// OpenGL pixel type, such as GL_UNSIGNED_BYTE. Zero for a borrowed texture.
/// </param>
public readonly partial record struct OpenglTextureFrame(
    ulong Generation,
    uint Width,
    uint Height,
    double ScaleFactor,
    ulong FrameId,
    uint Slot,
    uint Texture,
    uint Target,
    uint InternalFormat,
    uint Format,
    uint Type
);
