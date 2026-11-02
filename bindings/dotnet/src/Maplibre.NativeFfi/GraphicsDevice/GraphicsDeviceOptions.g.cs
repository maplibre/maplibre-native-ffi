// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for <c>GraphicsDeviceHandle.Create()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_graphics_device_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html">C API reference</see>.
/// </remarks>
public sealed record GraphicsDeviceOptions
{
    /// <summary>
    /// The GPU to use, as an index into the platform's list of GPUs.
    /// </summary>
    public uint? AdapterIndex { get; init; }

    /// <summary>
    /// Whether the device's texture sessions take turns in one OpenGL context
    /// instead of having one each. Only an EGL device reads it.
    /// </summary>
    public bool? SingleContext { get; init; }
}
