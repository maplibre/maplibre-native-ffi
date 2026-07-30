using Maplibre.Native.Camera;
using Maplibre.Native.Geo;
using Maplibre.Native.Json;
using Maplibre.Native.Map;
using Maplibre.Native.Query;
using Maplibre.Native.Runtime;
using Maplibre.Native.Style;
using Xunit;

namespace Maplibre.Native.Tests;

/// <summary>
/// BND-070: option descriptors compare and hash by property value, and <c>with</c> produces an
/// independent instance. Each case lists one mutator per declared property, so a property left out
/// of the record's equality fails its mutator assertion.
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

    [BindingSpecTest("BND-070")]
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

    [BindingSpecTest("BND-070")]
    [Fact]
    public void AnimationOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new AnimationOptions
                {
                    Duration = 1,
                    Easing = new UnitBezier(0.1, 0.2, 0.3, 0.4),
                    MinimumZoom = 3,
                    Velocity = 4,
                    TransitionId = 5,
                },
            options => options.Duration = 10,
            options => options.Easing = new UnitBezier(0.9, 0.8, 0.7, 0.6),
            options => options.MinimumZoom = 30,
            options => options.Velocity = 40,
            options => options.TransitionId = 50
        );
    }

    [BindingSpecTest("BND-070")]
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

    [BindingSpecTest("BND-070")]
    [Fact]
    public void BoundOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new BoundOptions
                {
                    Bounds = new BoundsConstraint.Bounded(
                        new LatLngBounds(new LatLng(0, 0), new LatLng(1, 1))
                    ),
                    MinimumZoom = 2,
                    MaximumZoom = 3,
                    MinimumPitch = 4,
                    MaximumPitch = 5,
                },
            options => options.Bounds = BoundsConstraint.Unbounded.Instance,
            options => options.MinimumZoom = 20,
            options => options.MaximumZoom = 30,
            options => options.MinimumPitch = 40,
            options => options.MaximumPitch = 50
        );
    }

    [BindingSpecTest("BND-070")]
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

    [BindingSpecTest("BND-070")]
    [Fact]
    public void ViewportOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new ViewportOptions
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

    [BindingSpecTest("BND-070")]
    [Fact]
    public void TileOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new TileOptions
                {
                    PrefetchZoomDelta = 1,
                    LodMinimumRadius = 2,
                    LodScale = 3,
                    LodPitchThreshold = 4,
                    LodZoomShift = 5,
                    LodMode = TileLodMode.Default,
                },
            options => options.PrefetchZoomDelta = 7,
            options => options.LodMinimumRadius = 20,
            options => options.LodScale = 30,
            options => options.LodPitchThreshold = 40,
            options => options.LodZoomShift = 50,
            options => options.LodMode = TileLodMode.Distance
        );
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void ProjectionModeOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new ProjectionModeOptions
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

    [BindingSpecTest("BND-070")]
    [Fact]
    public void MapOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new MapOptions
                {
                    Width = 100,
                    Height = 200,
                    ScaleFactor = 2,
                    MapMode = MapMode.Continuous,
                    FastPforEnabled = false,
                },
            options => options.Width = 300,
            options => options.Height = 400,
            options => options.ScaleFactor = 3,
            options => options.MapMode = MapMode.Static,
            options => options.FastPforEnabled = true
        );
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void RuntimeOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () => new RuntimeOptions { AssetPath = "assets", CachePath = "cache" },
            options => options.AssetPath = "other-assets",
            options => options.CachePath = "other-cache"
        );
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void TileSourceOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new TileSourceOptions
                {
                    Scheme = TileScheme.Xyz,
                    MinimumZoom = 1,
                    MaximumZoom = 2,
                    TileSize = 256,
                    Attribution = "attribution",
                    VectorEncoding = VectorTileEncoding.Mvt,
                    RasterEncoding = RasterDemEncoding.Mapbox,
                    Bounds = new LatLngBounds(new LatLng(0, 0), new LatLng(1, 1)),
                },
            options => options.Scheme = TileScheme.Tms,
            options => options.MinimumZoom = 10,
            options => options.MaximumZoom = 20,
            options => options.TileSize = 512,
            options => options.Attribution = "other",
            options => options.VectorEncoding = VectorTileEncoding.Mlt,
            options => options.RasterEncoding = RasterDemEncoding.Terrarium,
            options => options.Bounds = new LatLngBounds(new LatLng(-1, -1), new LatLng(2, 2))
        );
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void GeoJsonSourceOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () =>
                new GeoJsonSourceOptions
                {
                    MinimumZoom = 1,
                    MaximumZoom = 2,
                    TileSize = 256,
                    Buffer = 64,
                    Tolerance = 0.5,
                    LineMetrics = true,
                    Cluster = true,
                    ClusterRadius = 60,
                    ClusterMaximumZoom = 15,
                    ClusterMinimumPoints = 3,
                    SynchronousUpdate = true,
                    ClusterProperties = new JsonValue.Object([
                        new JsonMember("sum", new JsonValue.Int(1)),
                    ]),
                },
            options => options.MinimumZoom = 10,
            options => options.MaximumZoom = 20,
            options => options.TileSize = 512,
            options => options.Buffer = 128,
            options => options.Tolerance = 0.375,
            options => options.LineMetrics = false,
            options => options.Cluster = false,
            options => options.ClusterRadius = 50,
            options => options.ClusterMaximumZoom = 17,
            options => options.ClusterMinimumPoints = 2,
            options => options.SynchronousUpdate = false,
            options =>
                options.ClusterProperties = new JsonValue.Object([
                    new JsonMember("sum", new JsonValue.Int(2)),
                ])
        );

        // A present zero-valued field stays distinguishable from an absent one.
        Assert.NotEqual(new GeoJsonSourceOptions { ClusterRadius = 0 }, new GeoJsonSourceOptions());

        // Distinct cluster-property trees holding equal contents compare equal.
        Assert.Equal(
            new GeoJsonSourceOptions
            {
                ClusterProperties = new JsonValue.Object([
                    new JsonMember("sum", new JsonValue.Int(1)),
                ]),
            },
            new GeoJsonSourceOptions
            {
                ClusterProperties = new JsonValue.Object([
                    new JsonMember("sum", new JsonValue.Int(1)),
                ]),
            }
        );
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void StyleImageOptionsComparesByPropertyValue()
    {
        AssertValueSemantics(
            () => new StyleImageOptions { PixelRatio = 2f, Sdf = true },
            options => options.PixelRatio = 3f,
            options => options.Sdf = false
        );
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void QueryOptionsCompareLayerIdsElementByElement()
    {
        AssertValueSemantics(
            () =>
                new RenderedFeatureQueryOptions
                {
                    LayerIds = new[] { "a", "b" },
                    Filter = new JsonValue.Bool(true),
                },
            options => options.LayerIds = new[] { "a" },
            options => options.Filter = new JsonValue.String("filter")
        );
        AssertValueSemantics(
            () =>
                new SourceFeatureQueryOptions
                {
                    SourceLayerIds = new[] { "a", "b" },
                    Filter = new JsonValue.Bool(true),
                },
            options => options.SourceLayerIds = new[] { "a" },
            options => options.Filter = new JsonValue.String("filter")
        );

        // Distinct list instances holding the same elements compare equal.
        Assert.Equal(
            new RenderedFeatureQueryOptions { LayerIds = new[] { "a", "b" } },
            new RenderedFeatureQueryOptions
            {
                LayerIds = new List<string> { "a", "b" },
            }
        );
    }

    [BindingSpecTest("BND-069", "BND-070")]
    [Fact]
    public void QueryOptionsSnapshotCallerOwnedLayerIds()
    {
        var layerIds = new List<string> { "a" };
        var options = new RenderedFeatureQueryOptions { LayerIds = layerIds };
        var copy = options with { };

        layerIds.Add("b");

        Assert.Equal(["a"], options.LayerIds);
        Assert.Equal(["a"], copy.LayerIds);

        var sourceLayerIds = new List<string> { "a" };
        var sourceOptions = new SourceFeatureQueryOptions { SourceLayerIds = sourceLayerIds };

        sourceLayerIds.Add("b");

        Assert.Equal(["a"], sourceOptions.SourceLayerIds);
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void AbsentLayerIdsDifferFromEmptyLayerIds()
    {
        // The native field mask distinguishes an absent layer filter from an empty one.
        Assert.NotEqual(
            new RenderedFeatureQueryOptions(),
            new RenderedFeatureQueryOptions { LayerIds = Array.Empty<string>() }
        );
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void WithProducesAnIndependentInstance()
    {
        var original = new CameraOptions { Zoom = 1 };
        var derived = original with { Zoom = 2 };

        Assert.Equal(1, original.Zoom);
        Assert.Equal(2, derived.Zoom);
        Assert.NotSame(original, derived);
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void JsonContainerValuesCompareStructurally()
    {
        // Query filters compare by value, so the JSON tree they hold has to as well.
        Assert.Equal(
            new JsonValue.Array(new JsonValue[] { new JsonValue.Int(1) }),
            new JsonValue.Array(new JsonValue[] { new JsonValue.Int(1) })
        );
        Assert.Equal(
            new JsonValue.Object(new[] { new JsonMember("k", new JsonValue.Int(1)) }),
            new JsonValue.Object(new[] { new JsonMember("k", new JsonValue.Int(1)) })
        );
        Assert.NotEqual(
            new JsonValue.Object(new[] { new JsonMember("k", new JsonValue.Int(1)) }),
            new JsonValue.Object(new[] { new JsonMember("k", new JsonValue.Int(2)) })
        );
    }
}
