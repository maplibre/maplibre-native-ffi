// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// WebGPU backend context fields shared by WebGPU render targets.
/// </summary>
/// <remarks>
/// See <c>mln_webgpu_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct WebgpuContextDescriptor(
    NativePointer Instance,
    NativePointer Device,
    NativePointer Queue
);
