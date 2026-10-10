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
