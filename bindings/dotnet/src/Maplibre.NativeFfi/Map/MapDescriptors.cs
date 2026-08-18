using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Runtime;

namespace Maplibre.NativeFfi.Map;

/// <summary>Viewport options descriptor.</summary>
/// <remarks>
/// Compares and hashes by property value; keep an instance unmodified while it is a key in a
/// hash-based collection.
/// </remarks>
public sealed record ViewportOptions
{
    public NorthOrientation? NorthOrientation { get; set; }
    public ConstrainMode? ConstrainMode { get; set; }
    public ViewportMode? ViewportMode { get; set; }
    public EdgeInsets? FrustumOffset { get; set; }
}

/// <summary>Tile tuning options descriptor.</summary>
/// <remarks>
/// Compares and hashes by property value; keep an instance unmodified while it is a key in a
/// hash-based collection.
/// </remarks>
public sealed record TileOptions
{
    public uint? PrefetchZoomDelta { get; set; }
    public double? LodMinimumRadius { get; set; }
    public double? LodScale { get; set; }
    public double? LodPitchThreshold { get; set; }
    public double? LodZoomShift { get; set; }
    public TileLodMode? LodMode { get; set; }
}

/// <summary>Projection mode options descriptor.</summary>
/// <remarks>
/// Compares and hashes by property value; keep an instance unmodified while it is a key in a
/// hash-based collection.
/// </remarks>
public sealed record ProjectionModeOptions
{
    public bool? Axonometric { get; set; }
    public double? XSkew { get; set; }
    public double? YSkew { get; set; }
}

public readonly record struct LogicalExtent(uint Width, uint Height, double ScaleFactor);

/// <summary>A synchronous copy of the map's committed state.</summary>
/// <remarks>
/// Every committed map command publishes a new snapshot and reports its generation in the
/// completion, so a snapshot whose <see cref="Generation" /> is at or past that value observes
/// the commit.
/// </remarks>
/// <param name="FullyLoaded">True once every requested style and tile resource finished loading.</param>
public readonly record struct MapSnapshot(
    ulong Generation,
    DebugOptions DebugOptions,
    CameraOptions Camera,
    LogicalExtent LogicalExtent,
    ProjectionModeOptions ProjectionMode,
    ViewportOptions Viewport,
    bool FullyLoaded,
    bool RenderingStatsViewEnabled,
    bool RepaintDemand,
    RuntimeEventMask EventMask,
    ulong LatestRenderUpdateGeneration,
    TileOptions Tile,
    BoundOptions Bounds,
    FreeCameraOptions FreeCamera
);

/// <summary>Rendering statistics snapshot.</summary>
public readonly record struct RenderingStats(
    double EncodingTime,
    double RenderingTime,
    long FrameCount,
    long DrawCallCount,
    long TotalDrawCallCount
);
