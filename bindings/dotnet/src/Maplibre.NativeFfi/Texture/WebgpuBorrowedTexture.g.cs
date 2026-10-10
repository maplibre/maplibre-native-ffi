// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One caller-owned WebGPU texture of a borrowed texture ring.
/// </summary>
/// <remarks>
/// See <c>mln_webgpu_borrowed_texture</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Texture">
/// Borrowed WGPUTexture. Required.
/// </param>
/// <param name="TextureView">
/// Borrowed WGPUTextureView for texture. Required. The view must be a 2D color
/// view compatible with texture and the descriptor's format.
/// </param>
public readonly partial record struct WebgpuBorrowedTexture(
    NativePointer Texture,
    NativePointer TextureView
);
