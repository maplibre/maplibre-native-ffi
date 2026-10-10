// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// WGL context fields shared by OpenGL render targets on Windows.
/// </summary>
/// <remarks>
/// See <c>mln_wgl_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct WglContextDescriptor(
    NativePointer DeviceContext,
    NativePointer ShareContext,
    NativePointer GetProcAddress
);
