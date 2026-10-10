// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// WebGPU frame acquired from a session-owned texture target.
/// </summary>
/// <remarks>
/// See <c>mln_webgpu_owned_texture_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Generation">
/// Session generation that produced this frame.
/// </param>
/// <param name="Width">
/// Physical WebGPU texture width in device pixels.
/// </param>
/// <param name="Height">
/// Physical WebGPU texture height in device pixels.
/// </param>
/// <param name="ScaleFactor">
/// UI-to-device pixel scale used for this frame.
/// </param>
/// <param name="FrameId">
/// Opaque frame identity used to reject stale releases.
/// </param>
/// <param name="Texture">
/// Borrowed WGPUTexture. Valid until frame release.
/// </param>
/// <param name="TextureView">
/// Borrowed WGPUTextureView. Valid until frame release.
/// </param>
/// <param name="Device">
/// Borrowed WGPUDevice. Valid until frame release.
/// </param>
/// <param name="Format">
/// Backend-native WGPUTextureFormat value.
/// </param>
public readonly partial record struct WebgpuOwnedTextureFrame(
    ulong Generation,
    uint Width,
    uint Height,
    double ScaleFactor,
    ulong FrameId,
    NativePointer Texture,
    NativePointer TextureView,
    NativePointer Device,
    uint Format
);
