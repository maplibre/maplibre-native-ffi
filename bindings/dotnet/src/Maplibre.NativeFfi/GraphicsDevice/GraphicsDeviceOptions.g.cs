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
    /// Whether the device's texture sessions take turns rendering in one EGL
    /// context instead of each creating its own. Absent selects false.
    /// </summary>
    public bool? SingleContext { get; init; }
}
