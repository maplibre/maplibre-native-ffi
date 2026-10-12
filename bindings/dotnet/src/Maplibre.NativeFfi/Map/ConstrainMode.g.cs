// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Map constraint modes used by <c>mln_map_viewport_options</c>.
/// </summary>
/// <remarks>
/// See <c>mln_constrain_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum ConstrainMode : uint
{
    None = 0,
    HeightOnly = 1,
    WidthAndHeight = 2,
    Screen = 3,
}
