// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A size in logical pixels, and the physical pixels in each logical pixel.
/// </summary>
/// <remarks>
/// See <c>mln_logical_extent</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct LogicalExtent(uint Width, uint Height, double ScaleFactor);
