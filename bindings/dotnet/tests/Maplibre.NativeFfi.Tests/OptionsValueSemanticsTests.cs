using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// Option descriptors compare and hash by property value, and <c>with</c> produces an independent
/// instance. Each case lists one mutator per declared property, so a property left out of the
/// record's equality fails its mutator assertion.
/// </summary>
public sealed class OptionsValueSemanticsTests
{
    private static void AssertValueSemantics<T>(Func<T> baseline, params Action<T>[] mutators)
        where T : class
    {
        var left = baseline();
        var right = baseline();
        Assert.Equal(left, right);
        Assert.Equal(left.GetHashCode(), right.GetHashCode());
        Assert.NotSame(left, right);

        for (var index = 0; index < mutators.Length; index++)
        {
            var mutated = baseline();
            mutators[index](mutated);
            Assert.False(baseline().Equals(mutated), $"property {index} is missing from equality");
        }
    }

    [Fact]
    public void CameraOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new CameraOptions
                {
                    Center = new LatLng(1, 2),
                    CenterAltitude = 3,
                    Padding = new EdgeInsets(4, 5, 6, 7),
                    Anchor = new ScreenPoint(8, 9),
                    Zoom = 10,
                    Bearing = 11,
                    Pitch = 12,
                    Roll = 13,
                    FieldOfView = 14,
                },
            options => options.Center = new LatLng(90, 90),
            options => options.CenterAltitude = 300,
            options => options.Padding = new EdgeInsets(0, 0, 0, 0),
            options => options.Anchor = new ScreenPoint(80, 90),
            options => options.Zoom = 100,
            options => options.Bearing = 110,
            options => options.Pitch = 120,
            options => options.Roll = 130,
            options => options.FieldOfView = 140
        );
    }

    [Fact]
    public void AnimationOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new AnimationOptions
                {
                    DurationMs = 1,
                    Easing = new UnitBezier(0.1, 0.2, 0.3, 0.4),
                    MinZoom = 3,
                    Velocity = 4,
                    TransitionId = 5,
                },
            options => options.DurationMs = 10,
            options => options.Easing = new UnitBezier(0.9, 0.8, 0.7, 0.6),
            options => options.MinZoom = 30,
            options => options.Velocity = 40,
            options => options.TransitionId = 50
        );
    }

    [Fact]
    public void CameraFitOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new CameraFitOptions
                {
                    Padding = new EdgeInsets(1, 2, 3, 4),
                    Bearing = 5,
                    Pitch = 6,
                },
            options => options.Padding = new EdgeInsets(0, 0, 0, 0),
            options => options.Bearing = 50,
            options => options.Pitch = 60
        );
    }

    [Fact]
    public void BoundOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new BoundOptions
                {
                    Bounds = new LatLngBounds(new LatLng(0, 0), new LatLng(1, 1)),
                    MinZoom = 2,
                    MaxZoom = 3,
                    MinPitch = 4,
                    MaxPitch = 5,
                },
            options => options.Unbounded = true,
            options => options.MinZoom = 20,
            options => options.MaxZoom = 30,
            options => options.MinPitch = 40,
            options => options.MaxPitch = 50
        );
    }

    [Fact]
    public void FreeCameraOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new FreeCameraOptions
                {
                    Position = new Vec3(1, 2, 3),
                    Orientation = new Quaternion(0, 0, 0, 1),
                },
            options => options.Position = new Vec3(9, 9, 9),
            options => options.Orientation = new Quaternion(1, 0, 0, 0)
        );
    }

    [Fact]
    public void ViewportOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new MapViewportOptions
                {
                    NorthOrientation = NorthOrientation.Up,
                    ConstrainMode = ConstrainMode.None,
                    ViewportMode = ViewportMode.Default,
                    FrustumOffset = new EdgeInsets(1, 2, 3, 4),
                },
            options => options.NorthOrientation = NorthOrientation.Down,
            options => options.ConstrainMode = ConstrainMode.Screen,
            options => options.ViewportMode = ViewportMode.FlippedY,
            options => options.FrustumOffset = new EdgeInsets(0, 0, 0, 0)
        );
    }

    [Fact]
    public void TileOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new MapTileOptions
                {
                    PrefetchZoomDelta = 1,
                    LodMinRadius = 2,
                    LodScale = 3,
                    LodPitchThreshold = 4,
                    LodZoomShift = 5,
                    LodMode = TileLodMode.Default,
                },
            options => options.PrefetchZoomDelta = 7,
            options => options.LodMinRadius = 20,
            options => options.LodScale = 30,
            options => options.LodPitchThreshold = 40,
            options => options.LodZoomShift = 50,
            options => options.LodMode = TileLodMode.Distance
        );
    }

    [Fact]
    public void ProjectionModeOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new ProjectionMode
                {
                    Axonometric = true,
                    XSkew = 1,
                    YSkew = 2,
                },
            options => options.Axonometric = false,
            options => options.XSkew = 10,
            options => options.YSkew = 20
        );
    }

    [Fact]
    public void TileSourceOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new StyleTileSourceOptions
                {
                    Scheme = StyleTileScheme.Xyz,
                    MinZoom = 1,
                    MaxZoom = 2,
                    TileSize = 256,
                    Attribution = "attribution",
                    VectorEncoding = StyleVectorTileEncoding.Mvt,
                    RasterEncoding = StyleRasterDemEncoding.Mapbox,
                    Bounds = new LatLngBounds(new LatLng(0, 0), new LatLng(1, 1)),
                },
            options => options.Scheme = StyleTileScheme.Tms,
            options => options.MinZoom = 10,
            options => options.MaxZoom = 20,
            options => options.TileSize = 512,
            options => options.Attribution = "other",
            options => options.VectorEncoding = StyleVectorTileEncoding.Mlt,
            options => options.RasterEncoding = StyleRasterDemEncoding.Terrarium,
            options => options.Bounds = new LatLngBounds(new LatLng(-1, -1), new LatLng(2, 2))
        );
    }

    [Fact]
    public void GeojsonSourceOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new GeojsonSourceOptions
                {
                    MinZoom = 1,
                    MaxZoom = 2,
                    TileSize = 256,
                    Buffer = 64,
                    Tolerance = 0.5,
                    LineMetrics = true,
                    Cluster = true,
                    ClusterRadius = 60,
                    ClusterMaxZoom = 15,
                    ClusterMinPoints = 3,
                    SynchronousTiling = true,
                    ClusterProperties = """{"sum":1}"""u8.ToArray(),
                },
            options => options.MinZoom = 10,
            options => options.MaxZoom = 20,
            options => options.TileSize = 512,
            options => options.Buffer = 128,
            options => options.Tolerance = 0.375,
            options => options.LineMetrics = false,
            options => options.Cluster = false,
            options => options.ClusterRadius = 50,
            options => options.ClusterMaxZoom = 17,
            options => options.ClusterMinPoints = 2,
            options => options.SynchronousTiling = false,
            options => options.ClusterProperties = """{"sum":2}"""u8.ToArray()
        );

        // A present zero-valued field stays distinguishable from an absent one.
        Assert.NotEqual(new GeojsonSourceOptions { ClusterRadius = 0 }, new GeojsonSourceOptions());

        // Distinct cluster-property trees holding equal contents compare equal.
        Assert.Equal(
            new GeojsonSourceOptions { ClusterProperties = """{"sum":1}"""u8.ToArray() },
            new GeojsonSourceOptions { ClusterProperties = """{"sum":1}"""u8.ToArray() }
        );
    }

    [Fact]
    public void StyleImageOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () => new StyleImageOptions { PixelRatio = 2f, Sdf = true },
            options => options.PixelRatio = 3f,
            options => options.Sdf = false
        );
    }

    [Fact]
    public void StyleTransitionOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new StyleTransitionOptions
                {
                    DurationMs = 300,
                    DelayMs = 0,
                    EnablePlacementTransitions = false,
                },
            options => options.DurationMs = 500,
            // A present zero stays distinguishable from an absent property.
            options => options.DelayMs = null,
            // A present false stays distinguishable from an absent property.
            options => options.EnablePlacementTransitions = null
        );
    }

    [Fact]
    public void QueryOptionsCompareLayerIdsElementByElement()
    {
        AssertValueSemantics(
            () =>
                new RenderedFeatureQueryOptions
                {
                    LayerIds = new[] { "a", "b" },
                    Filter = "true"u8.ToArray(),
                },
            options => options.LayerIds = new[] { "a" },
            options => options.Filter = "\"filter\""u8.ToArray()
        );
        AssertValueSemantics(
            () =>
                new SourceFeatureQueryOptions
                {
                    SourceLayerIds = new[] { "a", "b" },
                    Filter = "true"u8.ToArray(),
                },
            options => options.SourceLayerIds = new[] { "a" },
            options => options.Filter = "\"filter\""u8.ToArray()
        );

        // Distinct list instances holding the same elements compare equal.
        Assert.Equal(
            new RenderedFeatureQueryOptions { LayerIds = new[] { "a", "b" } },
            new RenderedFeatureQueryOptions { LayerIds = new[] { "a", "b" } }
        );
    }

    [Fact]
    public void QueryOptionsSnapshotCallerOwnedLayerIds()
    {
        var layerIds = new[] { "a" };
        var options = new RenderedFeatureQueryOptions { LayerIds = layerIds };
        var copy = options with { };

        layerIds[0] = "b";

        Assert.Equal(["a"], options.LayerIds);
        Assert.Equal(["a"], copy.LayerIds);

        var sourceLayerIds = new[] { "a" };
        var sourceOptions = new SourceFeatureQueryOptions { SourceLayerIds = sourceLayerIds };

        sourceLayerIds[0] = "b";

        Assert.Equal(["a"], sourceOptions.SourceLayerIds);
    }

    [Fact]
    public void AbsentLayerIdsDifferFromEmptyLayerIds()
    {
        // The native field mask distinguishes an absent layer filter from an empty one.
        Assert.NotEqual(
            new RenderedFeatureQueryOptions(),
            new RenderedFeatureQueryOptions { LayerIds = Array.Empty<string>() }
        );
    }

    [Fact]
    public void WithProducesAnIndependentInstance()
    {
        var original = new CameraOptions { Zoom = 1 };
        var derived = original with { Zoom = 2 };

        Assert.Equal(1, original.Zoom);
        Assert.Equal(2, derived.Zoom);
        Assert.NotSame(original, derived);
    }
}
