// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One caller-owned OpenGL texture of a borrowed texture ring.
/// </summary>
/// <remarks>
/// See <c>mln_opengl_borrowed_texture</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Texture">
/// Borrowed OpenGL texture object name. Required.
/// </param>
public readonly partial record struct OpenglBorrowedTexture(uint Texture);
