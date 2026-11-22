// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The D3D12 objects of a device, as <c>GpuDeviceHandle.GetD3d12Context()</c>
/// reports them.
/// </summary>
/// <remarks>
/// See <c>mln_gpu_device_d3d12_context_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Device">
/// The <c>ID3D12Device</c>.
/// </param>
/// <param name="CommandQueue">
/// The direct <c>ID3D12CommandQueue</c> that sessions on the device submit to.
/// </param>
public readonly partial record struct GpuDeviceD3d12ContextResult(
    NativePointer Device,
    NativePointer CommandQueue
);
