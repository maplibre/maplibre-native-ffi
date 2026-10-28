// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for creating a graphics device.
/// </summary>
/// <remarks>
/// See <c>mln_graphics_device_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html">C API reference</see>.
/// </remarks>
public sealed record GraphicsDeviceOptions
{
    /// <summary>
    /// The adapter to create the device on, as a position in the graphics API's
    /// own list of adapters, from zero. Without it, the backend picks its
    /// default adapter.
    /// </summary>
    public uint? AdapterIndex { get; init; }

    /// <summary>
    /// Renders every texture session of the device in one graphics context, one
    /// session at a time, instead of in a context of each session's own.
    /// </summary>
    public bool SingleContext { get; init; }
}
