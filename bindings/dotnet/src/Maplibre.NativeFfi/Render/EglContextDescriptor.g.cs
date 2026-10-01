// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public readonly partial record struct EglContextDescriptor(
    NativePointer Display,
    NativePointer Config,
    NativePointer ShareContext,
    OpenglClientApi ClientApi,
    NativePointer GetProcAddress
);
