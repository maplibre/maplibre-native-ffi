using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class StyleLayerTests
{
    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task DemAndLocationLayerHelpersAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        _ = map.AddRasterDemSourceTilesAsync(
            "dem",
            ["https://example.test/dem/{z}/{x}/{y}.png"],
            null,
            TestContext.Current.CancellationToken
        );

        _ = map.AddHillshadeLayerAsync(
            "hillshade",
            "dem",
            "",
            TestContext.Current.CancellationToken
        );
        _ = map.AddColorReliefLayerAsync(
            "relief",
            "dem",
            "",
            TestContext.Current.CancellationToken
        );
        _ = map.AddLocationIndicatorLayerAsync(
            "location",
            "",
            TestContext.Current.CancellationToken
        );
        _ = map.SetLocationIndicatorLocationAsync(
            "location",
            new LatLng(12.5, 34.25),
            100,
            TestContext.Current.CancellationToken
        );
        _ = map.SetLocationIndicatorBearingAsync(
            "location",
            45,
            TestContext.Current.CancellationToken
        );
        _ = map.SetLocationIndicatorAccuracyRadiusAsync(
            "location",
            12,
            TestContext.Current.CancellationToken
        );
        _ = map.SetLocationIndicatorImageNameAsync(
            "location",
            LocationIndicatorImageKind.Top,
            "missing-image-name",
            TestContext.Current.CancellationToken
        );

        Assert.Equal(
            "hillshade",
            (
                await map.StyleLayerInfoAsync("hillshade", TestContext.Current.CancellationToken)
            )?.Type
        );
        Assert.Equal(
            "color-relief",
            (await map.StyleLayerInfoAsync("relief", TestContext.Current.CancellationToken))?.Type
        );
        Assert.Equal(
            "location-indicator",
            (await map.StyleLayerInfoAsync("location", TestContext.Current.CancellationToken))?.Type
        );
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task LayerBaseAccessorsRoundTripThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 64, Height = 64 });
        _ = map.SetStyleJsonAsync(
            """
            {"version":8,
             "sources":{"geo":{"type":"geojson",
                               "data":{"type":"FeatureCollection","features":[]}}},
             "layers":[{"id":"bg","type":"background"},
                       {"id":"fill","type":"fill","source":"geo"}]}
            """u8.ToArray(),
            TestContext.Current.CancellationToken
        );

        Assert.Null(
            (
                await map.StyleLayerInfoAsync("fill", TestContext.Current.CancellationToken)
            )?.SourceLayer
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetLayerSourceLayerAsync("fill", "roads", TestContext.Current.CancellationToken)
        );
        Assert.Equal(
            "roads",
            (
                await map.StyleLayerInfoAsync("fill", TestContext.Current.CancellationToken)
            )?.SourceLayer
        );
        Assert.Equal(
            "geo",
            (await map.StyleLayerInfoAsync("fill", TestContext.Current.CancellationToken))?.SourceId
        );

        // A layer type that takes no source rejects a source-layer mutation.
        RuntimeEventTestHelpers.AssertFailed(
            map.SetLayerSourceLayerAsync("bg", "roads", TestContext.Current.CancellationToken),
            MaplibreStatus.InvalidArgument
        );
        Assert.Null(
            (await map.StyleLayerInfoAsync("bg", TestContext.Current.CancellationToken))?.SourceId
        );

        // An unset zoom range crosses the boundary as infinities.
        var unset = Assert.IsType<LayerInfo>(
            await map.StyleLayerInfoAsync("fill", TestContext.Current.CancellationToken)
        );
        Assert.Equal(double.NegativeInfinity, unset.MinZoom);
        Assert.Equal(double.PositiveInfinity, unset.MaxZoom);
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetLayerMinZoomAsync("fill", 4, TestContext.Current.CancellationToken)
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetLayerMaxZoomAsync("fill", 12.5, TestContext.Current.CancellationToken)
        );

        Assert.Equal(StyleLayerVisibility.Visible, unset.Visibility);
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetLayerVisibilityAsync(
                "fill",
                StyleLayerVisibility.None,
                TestContext.Current.CancellationToken
            )
        );

        // The layer-info aggregate reports everything at once.
        var info = Assert.IsType<LayerInfo>(
            await map.StyleLayerInfoAsync("fill", TestContext.Current.CancellationToken)
        );
        Assert.Equal("fill", info.Id);
        Assert.Equal("fill", info.Type);
        Assert.Equal(4, info.MinZoom);
        Assert.Equal(12.5, info.MaxZoom);
        Assert.Equal(StyleLayerVisibility.None, info.Visibility);
        Assert.Equal((uint)StyleLayerVisibility.None, info.RawVisibility);
        Assert.Equal("geo", info.SourceId);
        Assert.Equal("roads", info.SourceLayer);

        // An unknown raw visibility is rejected by the completion.
        RuntimeEventTestHelpers.AssertFailed(
            map.SetLayerVisibilityAsync(
                "fill",
                (StyleLayerVisibility)900,
                TestContext.Current.CancellationToken
            ),
            MaplibreStatus.InvalidArgument
        );
        Assert.Equal(
            StyleLayerVisibility.None,
            (
                await map.StyleLayerInfoAsync("fill", TestContext.Current.CancellationToken)
            )?.Visibility
        );

        // The narrow copies read the same two IDs the aggregate reports.
        Assert.Equal(
            "geo",
            await map.GetLayerSourceIdAsync("fill", TestContext.Current.CancellationToken)
        );
        Assert.Equal(
            "roads",
            await map.GetLayerSourceLayerAsync("fill", TestContext.Current.CancellationToken)
        );
        Assert.Null(await map.GetLayerSourceIdAsync("bg", TestContext.Current.CancellationToken));

        // A missing layer reports no value from the aggregate, and not found from every command
        // and narrow query that names it.
        Assert.Null(
            await map.StyleLayerInfoAsync("missing", TestContext.Current.CancellationToken)
        );
        await AssertNotFoundAsync(() =>
            map.GetLayerSourceIdAsync("missing", TestContext.Current.CancellationToken)
        );
        await AssertNotFoundAsync(() =>
            map.GetLayerSourceLayerAsync("missing", TestContext.Current.CancellationToken)
        );
        RuntimeEventTestHelpers.AssertFailed(
            map.SetLayerMinZoomAsync("missing", 1, TestContext.Current.CancellationToken),
            MaplibreStatus.NotFound
        );
        RuntimeEventTestHelpers.AssertFailed(
            map.SetLayerSourceIdAsync("missing", "geo", TestContext.Current.CancellationToken),
            MaplibreStatus.NotFound
        );
        RuntimeEventTestHelpers.AssertFailed(
            map.MoveStyleLayerAsync("missing", "bg", TestContext.Current.CancellationToken),
            MaplibreStatus.NotFound
        );
    }

    private static async Task AssertNotFoundAsync<T>(Func<Task<T>> query)
    {
        var error = await Assert.ThrowsAsync<MaplibreException>(query);
        Assert.Equal(MaplibreStatus.NotFound, error.Status);
    }

    [BindingSpecTest("BND-061")]
    [Fact]
    public async Task StyleTransitionOptionsRoundTripThroughNativeMap()
    {
        byte[] transitionStyleJson =
            """
            {"version":8,"transition":{"duration":750,"delay":100},
             "sources":{},"layers":[]}
            """u8.ToArray();
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 64, Height = 64 });

        // A map with no style yet reports no duration or delay. The placement flag always
        // reports, because MapLibre Native always holds a value for it.
        var empty = await map.GetStyleTransitionOptionsAsync(TestContext.Current.CancellationToken);
        Assert.Null(empty.Duration);
        Assert.Null(empty.Delay);
        Assert.True(empty.EnablePlacementTransitions);

        // The style parser fills in its own 300ms duration for a style that declares no
        // transition.
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var parsed = await map.GetStyleTransitionOptionsAsync(
            TestContext.Current.CancellationToken
        );
        Assert.Equal(300, parsed.Duration);
        Assert.Null(parsed.Delay);

        _ = map.SetStyleJsonAsync(transitionStyleJson, TestContext.Current.CancellationToken);
        var declared = await map.GetStyleTransitionOptionsAsync(
            TestContext.Current.CancellationToken
        );
        Assert.Equal(750, declared.Duration);
        Assert.Equal(100, declared.Delay);
        Assert.True(declared.EnablePlacementTransitions);

        // A present zero stays distinguishable from an absent field, and an absent field clears
        // what the style declared rather than merging into it.
        var options = new StyleTransitionOptions
        {
            Duration = 0,
            EnablePlacementTransitions = false,
        };
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetStyleTransitionOptionsAsync(options, TestContext.Current.CancellationToken)
        );
        Assert.Equal(
            options,
            await map.GetStyleTransitionOptionsAsync(TestContext.Current.CancellationToken)
        );

        // Loading a style replaces the override with what that style declares.
        _ = map.SetStyleJsonAsync(transitionStyleJson, TestContext.Current.CancellationToken);
        Assert.Equal(
            declared,
            await map.GetStyleTransitionOptionsAsync(TestContext.Current.CancellationToken)
        );

        RuntimeEventTestHelpers.AssertFailed(
            map.SetStyleTransitionOptionsAsync(
                new StyleTransitionOptions { Delay = -1 },
                TestContext.Current.CancellationToken
            ),
            MaplibreStatus.InvalidArgument
        );
    }
}
