// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Screen-space inset in logical map pixels.
/// </summary>
/// <remarks>
/// See <c>mln_edge_insets</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct EdgeInsets(
    double Top,
    double Left,
    double Bottom,
    double Right
);
