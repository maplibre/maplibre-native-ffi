// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Map;

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
