// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Field mask values for MapLibre axonometric rendering options.
/// </summary>
/// <remarks>
/// See <c>mln_projection_mode_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum ProjectionModeField : uint
{
    Axonometric = 1,
    XSkew = 2,
    YSkew = 4,
}
