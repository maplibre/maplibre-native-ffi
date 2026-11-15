// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The Android hardware buffer of an exported ring's acquired frame.
/// </summary>
/// <remarks>
/// See <c>mln_android_hardware_buffer_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Slot">
/// The buffer's index in the ring.
/// </param>
/// <param name="Buffer">
/// The <c>AHardwareBuffer*</c>.
/// </param>
public readonly partial record struct AndroidHardwareBufferFrame(
    uint Width,
    uint Height,
    double ScaleFactor,
    uint Slot,
    NativePointer Buffer
);
