// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The DXGI shared texture of an acquired frame.
/// </summary>
/// <remarks>
/// See <c>mln_dxgi_shared_texture_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Slot">
/// The texture's index in the ring.
/// </param>
/// <param name="Format">
/// The texture's <c>DXGI_FORMAT</c>.
/// </param>
/// <param name="Handle">
/// The texture's shared NT <c>HANDLE</c>.
/// </param>
public readonly partial record struct DxgiSharedTextureFrame(
    uint Width,
    uint Height,
    double ScaleFactor,
    uint Slot,
    uint Format,
    NativePointer Handle
);
