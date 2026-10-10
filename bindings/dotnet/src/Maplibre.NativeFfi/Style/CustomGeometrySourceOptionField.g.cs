// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Field mask values for <c>mln_custom_geometry_source_options</c>.
/// </summary>
/// <remarks>
/// See <c>mln_custom_geometry_source_option_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum CustomGeometrySourceOptionField : uint
{
    MinZoom = 1,
    MaxZoom = 2,
    Tolerance = 4,
    TileSize = 8,
    Buffer = 16,
    Clip = 32,
    Wrap = 64,
}
