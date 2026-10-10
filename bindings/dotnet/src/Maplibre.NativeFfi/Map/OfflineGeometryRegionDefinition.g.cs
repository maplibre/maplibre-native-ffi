// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Geometry offline region definition.
/// </summary>
/// <remarks>
/// See <c>mln_offline_geometry_region_definition</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct OfflineGeometryRegionDefinition
{
    public OfflineGeometryRegionDefinition(
        string StyleUrl,
        byte[] Geometry,
        double MinZoom,
        double MaxZoom,
        float PixelRatio,
        bool IncludeIdeographs
    )
    {
        this.StyleUrl = StyleUrl;
        this.Geometry = Geometry;
        this.MinZoom = MinZoom;
        this.MaxZoom = MaxZoom;
        this.PixelRatio = PixelRatio;
        this.IncludeIdeographs = IncludeIdeographs;
    }

    public string StyleUrl { get; init; }
    public byte[] Geometry
    {
        get => GeometryStorage.ToArray();
        init => GeometryStorage = ValueArray.Copy(value);
    }
    internal ValueArray<byte> GeometryStorage { get; init; }
    public double MinZoom { get; init; }
    public double MaxZoom { get; init; }
    public float PixelRatio { get; init; }
    public bool IncludeIdeographs { get; init; }
}
