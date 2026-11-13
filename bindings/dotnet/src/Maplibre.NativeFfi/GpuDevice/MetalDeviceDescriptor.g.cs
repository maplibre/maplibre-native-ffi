// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Your Metal device and command queue, for <c>GpuDeviceHandle.WrapMetal()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_metal_device_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct MetalDeviceDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public MetalDeviceDescriptor(NativePointer Device, NativePointer CommandQueue)
    {
        this.Device = Device;
        this.CommandQueue = CommandQueue;
    }

    /// <summary>
    /// Your <c>MTLDevice</c>.
    /// </summary>
    public required NativePointer Device { get; init; }

    /// <summary>
    /// Your <c>MTLCommandQueue</c> on <c>device</c>, or null for a queue of the
    /// library's. With your queue, borrowed rings can use
    /// <c>GpuSyncKind.QueueOrder</c>.
    /// </summary>
    public NativePointer CommandQueue { get; init; }
}
