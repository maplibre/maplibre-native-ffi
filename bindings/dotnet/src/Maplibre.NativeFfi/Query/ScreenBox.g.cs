// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A rectangle of the screen, from its <c>min</c> corner to its <c>max</c>
/// corner.
/// </summary>
/// <remarks>
/// See <c>mln_screen_box</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct ScreenBox(ScreenPoint Min, ScreenPoint Max);
