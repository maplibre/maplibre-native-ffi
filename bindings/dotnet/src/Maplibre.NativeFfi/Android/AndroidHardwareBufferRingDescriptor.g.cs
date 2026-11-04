// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of Android hardware buffers that the library allocates, for
/// <c>MapHandle.AttachAndroidHardwareBufferRing()</c>.
/// </summary>
/// <remarks>
/// <para>Each buffer has the map's size in pixels, <c>logical_size</c> times
/// the map's scale factor, and the format
/// AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM. Its usage is
/// AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT,
/// AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, and the bits of
/// <c>usage</c>.</para>
/// <para>See <c>mln_android_hardware_buffer_ring_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct AndroidHardwareBufferRingDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public AndroidHardwareBufferRingDescriptor(
        LogicalSize LogicalSize,
        uint BufferCount,
        ulong Usage
    )
    {
        this.LogicalSize = LogicalSize;
        this.BufferCount = BufferCount;
        this.Usage = Usage;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// How many buffers the ring has, from 1 to 8.
    /// </summary>
    public required uint BufferCount { get; init; }

    /// <summary>
    /// More <c>AHARDWAREBUFFER_USAGE_*</c> bits that your reads need, such as
    /// AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, or zero.
    /// </summary>
    public ulong Usage { get; init; }
}
