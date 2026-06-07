using Maplibre.Native.Camera;

namespace Maplibre.Native.Map;

/// <summary>Viewport options descriptor.</summary>
public sealed class ViewportOptions
{
    public NorthOrientation? NorthOrientation { get; set; }
    public ConstrainMode? ConstrainMode { get; set; }
    public ViewportMode? ViewportMode { get; set; }
    public EdgeInsets? FrustumOffset { get; set; }
}

/// <summary>Tile tuning options descriptor.</summary>
public sealed class TileOptions
{
    public uint? PrefetchZoomDelta { get; set; }
    public double? LodMinimumRadius { get; set; }
    public double? LodScale { get; set; }
    public double? LodPitchThreshold { get; set; }
    public double? LodZoomShift { get; set; }
    public TileLodMode? LodMode { get; set; }
}

/// <summary>Projection mode options descriptor.</summary>
public sealed class ProjectionModeOptions
{
    public bool? Axonometric { get; set; }
    public double? XSkew { get; set; }
    public double? YSkew { get; set; }
}

/// <summary>Rendering statistics snapshot.</summary>
public readonly record struct RenderingStats(
    double EncodingTime,
    double RenderingTime,
    long FrameCount,
    long DrawCallCount,
    long TotalDrawCallCount
);
