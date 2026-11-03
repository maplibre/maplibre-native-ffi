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
/// The texture's index in the descriptor's <c>textures</c>.
/// </param>
public readonly partial record struct OpenGlTextureFrame(
    ulong Generation,
    uint Width,
    uint Height,
    double ScaleFactor,
    ulong FrameId,
    uint Slot,
    uint Texture,
    uint Target
);
