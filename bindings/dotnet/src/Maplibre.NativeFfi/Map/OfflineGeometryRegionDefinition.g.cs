// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// An offline region that covers a GeoJSON geometry.
/// </summary>
/// <remarks>
/// <para>The members other than <c>geometry</c> mean the same as those of
/// <c>OfflineTilePyramidRegionDefinition</c>.</para>
/// <para>See <c>mln_offline_geometry_region_definition</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct OfflineGeometryRegionDefinition
{
    public OfflineGeometryRegionDefinition() { }

    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OfflineGeometryRegionDefinition(
        string StyleUrl,
        string Geometry,
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

    public string StyleUrl { get; init; } = "";
    public required string Geometry { get; init; }
    public double MinZoom { get; init; }
    public double MaxZoom { get; init; }
    public float PixelRatio { get; init; }
    public bool IncludeIdeographs { get; init; }
}
