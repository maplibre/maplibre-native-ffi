// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// EGL context fields shared by OpenGL render targets.
/// </summary>
/// <remarks>
/// See <c>mln_egl_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct EglContextDescriptor(
    NativePointer Display,
    NativePointer Config,
    NativePointer ShareContext,
    OpenglClientApi ClientApi,
    NativePointer GetProcAddress
);
