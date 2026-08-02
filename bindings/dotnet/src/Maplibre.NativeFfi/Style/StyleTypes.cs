using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal;
using Maplibre.NativeFfi.Json;
using Maplibre.NativeFfi.Render;

namespace Maplibre.NativeFfi.Style;

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
/// <summary>Whether a style layer draws.</summary>
/// <remarks>
/// This is an open domain: MapLibre Native may report a value with no named member here, so a
/// switch over this type needs a default case.
/// </remarks>
public enum StyleLayerVisibility : uint
{
    Visible = 0,
    None = 1,
}

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
    /// Applies data updates synchronously, so data set through the GeoJSON data update API
    /// reaches the next rendered frame instead of being tiled on a worker and shown in a later
    /// one.
    /// </summary>
    public bool? SynchronousUpdate { get; set; }

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
    bool Sdf,
    ulong StretchXCount,
    ulong StretchYCount,
    ImageContent? Content,
    StyleImageTextFit? TextFitWidth,
    StyleImageTextFit? TextFitHeight
);

/// <summary>One stretchable interval along an image axis, in image pixels.</summary>
public readonly record struct ImageStretch(float From, float To);

/// <summary>Content-box insets in image pixels, from the image's top-left.</summary>
public readonly record struct ImageContent(float Left, float Top, float Right, float Bottom);

/// <summary>How a stretchable image fits text along one axis.</summary>
/// <remarks>
/// This is an open domain: MapLibre Native may report a value with no named member here, so a
/// switch over this type needs a default case.
/// </remarks>
public enum StyleImageTextFit : uint
{
    StretchOrShrink = 0,
    StretchOnly = 1,
    Proportional = 2,
}

/// <remarks>
/// Compares and hashes by property value; <c>with</c> returns an independent instance. Keep an
/// instance unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record StyleImageOptions
{
    public float? PixelRatio { get; set; }
    public bool? Sdf { get; set; }

    private IReadOnlyList<ImageStretch>? stretchX;
    private IReadOnlyList<ImageStretch>? stretchY;

    /// <summary>
    /// Horizontally stretchable intervals. A present empty list stays distinguishable from an
    /// absent one. Assignment stores a snapshot the caller cannot mutate.
    /// </summary>
    public IReadOnlyList<ImageStretch>? StretchX
    {
        get => stretchX;
        set => stretchX = ValueEquality.SnapshotOrNull(value);
    }

    /// <summary>Vertically stretchable intervals. Assignment stores a snapshot.</summary>
    public IReadOnlyList<ImageStretch>? StretchY
    {
        get => stretchY;
        set => stretchY = ValueEquality.SnapshotOrNull(value);
    }

    /// <summary>Content box used when <c>icon-text-fit</c> applies.</summary>
    public ImageContent? Content { get; set; }

    public StyleImageTextFit? TextFitWidth { get; set; }

    public StyleImageTextFit? TextFitHeight { get; set; }

    /// <summary>Compares by property value, including list contents.</summary>
    public bool Equals(StyleImageOptions? other) =>
        other is not null
        && PixelRatio == other.PixelRatio
        && Sdf == other.Sdf
        && StretchesEqual(StretchX, other.StretchX)
        && StretchesEqual(StretchY, other.StretchY)
        && Content == other.Content
        && TextFitWidth == other.TextFitWidth
        && TextFitHeight == other.TextFitHeight;

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(PixelRatio);
        hash.Add(Sdf);
        AddStretches(ref hash, StretchX);
        AddStretches(ref hash, StretchY);
        hash.Add(Content);
        hash.Add(TextFitWidth);
        hash.Add(TextFitHeight);
        return hash.ToHashCode();
    }

    private static bool StretchesEqual(
        IReadOnlyList<ImageStretch>? left,
        IReadOnlyList<ImageStretch>? right
    )
    {
        if (left is null || right is null)
        {
            return ReferenceEquals(left, right);
        }
        return left.SequenceEqual(right);
    }

    private static void AddStretches(ref HashCode hash, IReadOnlyList<ImageStretch>? stretches)
    {
        if (stretches is null)
        {
            hash.Add(0);
            return;
        }
        hash.Add(stretches.Count);
        foreach (var stretch in stretches)
        {
            hash.Add(stretch);
        }
    }
}

/// <summary>The style's global transition options.</summary>
/// <remarks>
/// <para>
/// These control how the style animates paint property changes and whether symbol placement
/// changes cross-fade. They are distinct from camera animation options and from the per-property
/// transitions a style declares.
/// </para>
/// <para>
/// Compares and hashes by property value; <c>with</c> returns an independent instance. Keep an
/// instance unmodified while it is a key in a hash-based collection.
/// </para>
/// </remarks>
public sealed record StyleTransitionOptions
{
    /// <summary>
    /// Transition duration in milliseconds. Null falls back to the duration the style declares
    /// for each transitioning property.
    /// </summary>
    public double? Duration { get; set; }

    /// <summary>
    /// Transition delay in milliseconds. Null falls back to the delay the style declares for each
    /// transitioning property.
    /// </summary>
    public double? Delay { get; set; }

    /// <summary>
    /// Whether symbol placement changes cross-fade. Null leaves the cross-fade on, which is
    /// MapLibre Native's own default.
    /// </summary>
    /// <remarks>
    /// Clearing it makes symbol placement changes apply to the next rendered frame. Hosts that
    /// move symbol-backed features at pointer frequency clear it for the duration of the
    /// interaction so the rendered symbol keeps up. Reading the options always reports it, because
    /// MapLibre Native always holds a value for it.
    /// </remarks>
    public bool? EnablePlacementTransitions { get; set; }
}

public delegate void CustomGeometrySourceCallback(CanonicalTileId tileId);
