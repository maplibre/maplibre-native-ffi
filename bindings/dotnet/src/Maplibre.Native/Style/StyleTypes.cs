using Maplibre.Native.Geo;
using Maplibre.Native.Json;
using Maplibre.Native.Render;

namespace Maplibre.Native.Style;

public enum SourceType : uint
{
    Unknown = 0,
    Vector = 1,
    Raster = 2,
    RasterDem = 3,
    GeoJson = 4,
    Image = 5,
    Video = 6,
    Annotations = 7,
    CustomVector = 8,
}

public enum TileScheme : uint
{
    Xyz = 0,
    Tms = 1,
}

public enum VectorTileEncoding : uint
{
    Mvt = 0,
    Mlt = 1,
}

public enum RasterDemEncoding : uint
{
    Mapbox = 0,
    Terrarium = 1,
}

public enum LocationIndicatorImageKind : uint
{
    Top = 0,
    Bearing = 1,
    Shadow = 2,
}

public sealed record SourceInfo(
    string Id,
    SourceType Type,
    uint RawType,
    bool IsVolatile,
    string? Attribution
);

/// <remarks>
/// Compares and hashes by property value; <c>with</c> returns an independent instance. Keep an
/// instance unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record TileSourceOptions
{
    public TileScheme? Scheme { get; set; }
    public double? MinimumZoom { get; set; }
    public double? MaximumZoom { get; set; }
    public uint? TileSize { get; set; }
    public string? Attribution { get; set; }
    public VectorTileEncoding? VectorEncoding { get; set; }
    public RasterDemEncoding? RasterEncoding { get; set; }
    public LatLngBounds? Bounds { get; set; }
}

/// <remarks>
/// MapLibre Native fixes these options when the source is created, so
/// <see cref="Map.MapHandle.SetGeoJsonSourceUrl" /> and
/// <see cref="Map.MapHandle.SetGeoJsonSourceData" /> keep the options the source was added with.
/// Compares and hashes by property value; <c>with</c> returns an independent instance. Keep an
/// instance unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record GeoJsonSourceOptions
{
    public double? MinimumZoom { get; set; }
    public double? MaximumZoom { get; set; }
    public uint? TileSize { get; set; }
    public uint? Buffer { get; set; }
    public double? Tolerance { get; set; }
    public bool? LineMetrics { get; set; }
    public bool? Cluster { get; set; }
    public uint? ClusterRadius { get; set; }
    public double? ClusterMaximumZoom { get; set; }
    public uint? ClusterMinimumPoints { get; set; }

    /// <summary>
    /// Cluster aggregation expressions keyed by property name, as a JSON object whose members
    /// follow the MapLibre Style Spec <c>clusterProperties</c> form.
    /// </summary>
    public JsonValue? ClusterProperties { get; set; }
}

public sealed class CustomGeometrySourceOptions
{
    public CustomGeometrySourceCallback? FetchTile { get; set; }
    public CustomGeometrySourceCallback? CancelTile { get; set; }
    public uint? TileSize { get; set; }
    public double? MinimumZoom { get; set; }
    public double? MaximumZoom { get; set; }
    public double? Buffer { get; set; }
    public double? Tolerance { get; set; }
    public bool? Clip { get; set; }
    public bool? Wrap { get; set; }
}

public sealed record StyleImage(PremultipliedRgba8Image Image, StyleImageOptions Options);

public sealed record StyleImageInfo(
    uint Width,
    uint Height,
    uint Stride,
    ulong ByteLength,
    float PixelRatio,
    bool Sdf
);

/// <remarks>
/// Compares and hashes by property value; <c>with</c> returns an independent instance. Keep an
/// instance unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record StyleImageOptions
{
    public float? PixelRatio { get; set; }
    public bool? Sdf { get; set; }
}

public delegate void CustomGeometrySourceCallback(CanonicalTileId tileId);
