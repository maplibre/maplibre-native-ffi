// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A size in logical pixels, which the map's scale factor turns into pixels.
/// </summary>
/// <remarks>
/// See <c>mln_logical_size</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct LogicalSize(uint Width, uint Height);
