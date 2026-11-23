// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Your D3D12 device and direct command queue, for
/// <c>GpuDeviceHandle.WrapD3d12()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_d3d12_device_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct D3d12DeviceDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public D3d12DeviceDescriptor(NativePointer Device, NativePointer CommandQueue)
    {
        this.Device = Device;
        this.CommandQueue = CommandQueue;
    }

    /// <summary>
    /// Your <c>ID3D12Device</c>.
    /// </summary>
    public required NativePointer Device { get; init; }

    /// <summary>
    /// Your <c>ID3D12CommandQueue</c> of <c>D3D12_COMMAND_LIST_TYPE_DIRECT</c>
    /// on <c>device</c>, which sessions on the device submit to. Submit your
    /// reads of a borrowed ring's textures to it for
    /// <c>GpuSyncKind.QueueOrder</c>.
    /// </summary>
    public required NativePointer CommandQueue { get; init; }
}
