// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A point on the screen, in logical pixels from the map's top-left corner.
/// </summary>
/// <remarks>
/// See <c>mln_screen_point</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct ScreenPoint(double X, double Y);
