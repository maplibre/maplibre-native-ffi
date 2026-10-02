// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

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
