// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// OpenGL frame acquired from a session-owned texture target.
/// </summary>
/// <remarks>
/// See <c>mln_opengl_owned_texture_frame</c> in the <see
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
/// <param name="Texture">
/// Borrowed OpenGL texture object name. Valid until frame release.
/// </param>
/// <param name="Target">
/// OpenGL texture target. GL_TEXTURE_2D is the expected target.
/// </param>
/// <param name="InternalFormat">
/// OpenGL internal format, such as GL_RGBA8.
/// </param>
/// <param name="Format">
/// OpenGL pixel format, such as GL_RGBA.
/// </param>
/// <param name="Type">
/// OpenGL pixel type, such as GL_UNSIGNED_BYTE.
/// </param>
public readonly partial record struct OpenglOwnedTextureFrame(
    ulong Generation,
    uint Width,
    uint Height,
    double ScaleFactor,
    ulong FrameId,
    uint Texture,
    uint Target,
    uint InternalFormat,
    uint Format,
    uint Type
);
