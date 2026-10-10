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
/// <param name="Instance">
/// Borrowed WGPUInstance. Optional for texture targets.
/// </param>
/// <param name="Device">
/// Borrowed WGPUDevice. Required.
/// </param>
/// <param name="Queue">
/// Borrowed WGPUQueue. Optional; null uses the device default queue. A non-null
/// queue must belong to device.
/// </param>
public readonly partial record struct WebgpuContextDescriptor(
    NativePointer Instance,
    NativePointer Device,
    NativePointer Queue
);
