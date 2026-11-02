// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The OpenGL texture of an acquired frame.
/// </summary>
/// <remarks>
/// See <c>mln_open_gl_texture_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Slot">
/// The texture's index in the ring. For a borrowed ring, it indexes the
/// descriptor's <c>textures</c>.
/// </param>
/// <param name="InternalFormat">
/// The texture's internal format, or 0 for a borrowed ring.
/// </param>
/// <param name="Format">
/// The texture's pixel format, or 0 for a borrowed ring.
/// </param>
/// <param name="Type">
/// The texture's pixel type, or 0 for a borrowed ring.
/// </param>
public readonly partial record struct OpenGlTextureFrame(
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
