// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Style source type values returned by source metadata queries.
/// </summary>
/// <remarks>
/// See <c>mln_style_source_type</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public enum StyleSourceType : uint
{
    Unknown = 0,
    Vector = 1,
    Raster = 2,
    RasterDem = 3,
    Geojson = 4,
    Image = 5,
    Video = 6,
    Annotations = 7,
    CustomVector = 8,
    CustomMvtVector = 9,
}
