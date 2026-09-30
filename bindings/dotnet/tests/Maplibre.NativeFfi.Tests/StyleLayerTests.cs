using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class StyleLayerTests
{
    [Fact]
    public async Task GlobalStateDefaultsUpdatesAndStyleReplacement()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(runtime, MapOptions.Default);
        var rejected = await map.SetGlobalStatePropertyAsync("theme", "true"u8.ToArray());
        Assert.Equal(CommandDisposition.Failed, rejected.Disposition);
        Assert.Contains("style JSON has not loaded", rejected.Diagnostic);
        await map.SetStyleJsonAsync(
            """{"version":8,"sources":{},"layers":[],"state":{"theme":{"default":"light"}}}"""u8.ToArray()
        );
        Assert.Equal("""{"theme":"light"}"""u8.ToArray(), await map.GetGlobalStateAsync());
        await map.SetGlobalStatePropertyAsync("theme", """["dark",{"enabled":true}]"""u8.ToArray());
        var snapshot = await map.GetGlobalStateAsync();
        await map.SetGlobalStatePropertyAsync("theme", "null"u8.ToArray());
        Assert.Equal("""{"theme":"light"}"""u8.ToArray(), await map.GetGlobalStateAsync());
        Assert.Equal("""{"theme":["dark",{"enabled":true}]}"""u8.ToArray(), snapshot);
        await map.SetStyleJsonAsync("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());
        Assert.Equal("{}"u8.ToArray(), await map.GetGlobalStateAsync());
        await map.SetGlobalStatePropertyAsync("theme", "true"u8.ToArray());
        await map.SetGlobalStatePropertyAsync("theme", "null"u8.ToArray());
        Assert.Equal("""{"theme":null}"""u8.ToArray(), await map.GetGlobalStateAsync());
    }

    [Fact]
    public async Task DemAndLocationLayerHelpersAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
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
            (await map.GetStyleLayerInfoAsync("hillshade", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );
        Assert.Equal(
            "color-relief",
            (await map.GetStyleLayerInfoAsync("relief", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );
        Assert.Equal(
            "location-indicator",
            (await map.GetStyleLayerInfoAsync("location", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );
    }

    [Fact]
    public async Task LayerBaseAccessorsRoundTripThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 64, Height = 64 },
            }
        );
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
                await map.GetStyleLayerInfoAsync("fill", TestContext.Current.CancellationToken)
            )?.SourceLayer
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetLayerSourceLayerAsync("fill", "roads", TestContext.Current.CancellationToken)
        );
        Assert.Equal(
            "roads",
            (
                await map.GetStyleLayerInfoAsync("fill", TestContext.Current.CancellationToken)
            )?.SourceLayer
        );
        Assert.Equal(
            "geo",
            (
                await map.GetStyleLayerInfoAsync("fill", TestContext.Current.CancellationToken)
            )?.SourceId
        );

        // A layer type that takes no source rejects a source-layer mutation.
        RuntimeEventTestHelpers.AssertFailed(
            map.SetLayerSourceLayerAsync("bg", "roads", TestContext.Current.CancellationToken),
            MaplibreStatus.InvalidArgument
        );
        Assert.Null(
            (
                await map.GetStyleLayerInfoAsync("bg", TestContext.Current.CancellationToken)
            )?.SourceId
        );

        // An unset zoom range crosses the boundary as infinities.
        var unset = Assert.IsType<StyleLayerResult>(
            await map.GetStyleLayerInfoAsync("fill", TestContext.Current.CancellationToken)
        );
        Assert.Equal(double.NegativeInfinity, unset.Info.MinZoom);
        Assert.Equal(double.PositiveInfinity, unset.Info.MaxZoom);
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetLayerMinZoomAsync("fill", 4, TestContext.Current.CancellationToken)
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetLayerMaxZoomAsync("fill", 12.5, TestContext.Current.CancellationToken)
        );

        Assert.Equal(StyleLayerVisibility.Visible, unset.Info.Visibility);
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetLayerVisibilityAsync(
                "fill",
                StyleLayerVisibility.None,
                TestContext.Current.CancellationToken
            )
        );

        // The layer-info aggregate reports everything at once.
        var info = Assert.IsType<StyleLayerResult>(
            await map.GetStyleLayerInfoAsync("fill", TestContext.Current.CancellationToken)
        );
        Assert.Equal("fill", info.Info.Type);
        Assert.Equal(4, info.Info.MinZoom);
        Assert.Equal(12.5, info.Info.MaxZoom);
        Assert.Equal(StyleLayerVisibility.None, info.Info.Visibility);
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
            (await map.GetStyleLayerInfoAsync("fill", TestContext.Current.CancellationToken))
                ?.Info
                .Visibility
        );

        // The narrow copies read the same two IDs the aggregate reports.
        Assert.Equal(
            "geo",
            await map.CopyLayerSourceIdAsync("fill", TestContext.Current.CancellationToken)
        );
        Assert.Equal(
            "roads",
            await map.CopyLayerSourceLayerAsync("fill", TestContext.Current.CancellationToken)
        );
        Assert.Null(await map.CopyLayerSourceIdAsync("bg", TestContext.Current.CancellationToken));

        // A missing layer reports no value from the aggregate, and not found from every command
        // and narrow query that names it.
        Assert.Null(
            await map.GetStyleLayerInfoAsync("missing", TestContext.Current.CancellationToken)
        );
        await AssertNotFoundAsync(() =>
            map.CopyLayerSourceIdAsync("missing", TestContext.Current.CancellationToken)
        );
        await AssertNotFoundAsync(() =>
            map.CopyLayerSourceLayerAsync("missing", TestContext.Current.CancellationToken)
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

    [Fact]
    public async Task StyleTransitionOptionsRoundTripThroughNativeMap()
    {
        byte[] transitionStyleJson =
            """
            {"version":8,"transition":{"duration":750,"delay":100},
             "sources":{},"layers":[]}
            """u8.ToArray();
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 64, Height = 64 },
            }
        );

        // A map with no style yet reports no duration or delay. The placement flag always
        // reports, because MapLibre Native always holds a value for it.
        var empty = await map.GetStyleTransitionOptionsAsync(TestContext.Current.CancellationToken);
        Assert.Null(empty.DurationMs);
        Assert.Null(empty.DelayMs);
        Assert.True(empty.EnablePlacementTransitions);

        // The style parser fills in its own 300ms duration for a style that declares no
        // transition.
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var parsed = await map.GetStyleTransitionOptionsAsync(
            TestContext.Current.CancellationToken
        );
        Assert.Equal(300, parsed.DurationMs);
        Assert.Null(parsed.DelayMs);

        _ = map.SetStyleJsonAsync(transitionStyleJson, TestContext.Current.CancellationToken);
        var declared = await map.GetStyleTransitionOptionsAsync(
            TestContext.Current.CancellationToken
        );
        Assert.Equal(750, declared.DurationMs);
        Assert.Equal(100, declared.DelayMs);
        Assert.True(declared.EnablePlacementTransitions);

        // A present zero stays distinguishable from an absent field, and an absent field clears
        // what the style declared rather than merging into it.
        var options = new StyleTransitionOptions
        {
            DurationMs = 0,
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
                new StyleTransitionOptions { DelayMs = -1 },
                TestContext.Current.CancellationToken
            ),
            MaplibreStatus.InvalidArgument
        );
    }
}
