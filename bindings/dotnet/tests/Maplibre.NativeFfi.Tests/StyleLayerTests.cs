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
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        map.SetStyleJsonAsync("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());
        map.AddRasterDemSourceTilesAsync("dem", ["https://example.test/dem/{z}/{x}/{y}.png"], null);

        map.AddHillshadeLayerAsync("hillshade", "dem", "");
        map.AddColorReliefLayerAsync("relief", "dem", "");
        map.AddLocationIndicatorLayerAsync("location", "");
        map.SetLocationIndicatorLocationAsync("location", new LatLng(12.5, 34.25), 100);
        map.SetLocationIndicatorBearingAsync("location", 45);
        map.SetLocationIndicatorAccuracyRadiusAsync("location", 12);
        map.SetLocationIndicatorImageNameAsync(
            "location",
            LocationIndicatorImageKind.Top,
            "missing-image-name"
        );

        Assert.Equal("hillshade", (await map.StyleLayerInfoAsync("hillshade"))?.Type);
        Assert.Equal("color-relief", (await map.StyleLayerInfoAsync("relief"))?.Type);
        Assert.Equal("location-indicator", (await map.StyleLayerInfoAsync("location"))?.Type);
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task LayerBaseAccessorsRoundTripThroughNativeMap()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 64, Height = 64 });
        map.SetStyleJsonAsync(
            System.Text.Encoding.UTF8.GetBytes(
                "{\"version\":8,\"sources\":{\"geo\":{\"type\":\"geojson\",\"data\":"
                    + "{\"type\":\"FeatureCollection\",\"features\":[]}}},\"layers\":["
                    + "{\"id\":\"bg\",\"type\":\"background\"},"
                    + "{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"geo\"}]}"
            )
        );

        Assert.Null((await map.StyleLayerInfoAsync("fill"))?.SourceLayer);
        RuntimeEventTestHelpers.AssertCommitted(map.SetLayerSourceLayerAsync("fill", "roads"));
        Assert.Equal("roads", (await map.StyleLayerInfoAsync("fill"))?.SourceLayer);
        Assert.Equal("geo", (await map.StyleLayerInfoAsync("fill"))?.SourceId);

        // A layer type that takes no source rejects a source-layer mutation.
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.SetLayerSourceLayerAsync("bg", "roads"),
            MaplibreStatus.InvalidArgument
        );
        Assert.Null((await map.StyleLayerInfoAsync("bg"))?.SourceId);

        // An unset zoom range crosses the boundary as infinities.
        var unset = Assert.IsType<LayerInfo>(await map.StyleLayerInfoAsync("fill"));
        Assert.Equal(double.NegativeInfinity, unset.MinZoom);
        Assert.Equal(double.PositiveInfinity, unset.MaxZoom);
        RuntimeEventTestHelpers.AssertCommitted(map.SetLayerMinZoomAsync("fill", 4));
        RuntimeEventTestHelpers.AssertCommitted(map.SetLayerMaxZoomAsync("fill", 12.5));

        Assert.Equal(StyleLayerVisibility.Visible, unset.Visibility);
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetLayerVisibilityAsync("fill", StyleLayerVisibility.None)
        );

        // The layer-info aggregate reports everything at once.
        var info = Assert.IsType<LayerInfo>(await map.StyleLayerInfoAsync("fill"));
        Assert.Equal("fill", info.Id);
        Assert.Equal("fill", info.Type);
        Assert.Equal(4, info.MinZoom);
        Assert.Equal(12.5, info.MaxZoom);
        Assert.Equal(StyleLayerVisibility.None, info.Visibility);
        Assert.Equal((uint)StyleLayerVisibility.None, info.RawVisibility);
        Assert.Equal("geo", info.SourceId);
        Assert.Equal("roads", info.SourceLayer);

        // An unknown raw visibility is rejected by the completion.
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.SetLayerVisibilityAsync("fill", (StyleLayerVisibility)900),
            MaplibreStatus.InvalidArgument
        );
        Assert.Equal(
            StyleLayerVisibility.None,
            (await map.StyleLayerInfoAsync("fill"))?.Visibility
        );

        // A missing layer reports not found rather than throwing.
        Assert.Null(await map.StyleLayerInfoAsync("missing"));
    }

    [BindingSpecTest("BND-061")]
    [Fact]
    public async Task StyleTransitionOptionsRoundTripThroughNativeMap()
    {
        const string transitionStyleJson =
            "{\"version\":8,\"transition\":{\"duration\":750,\"delay\":100},"
            + "\"sources\":{},\"layers\":[]}";
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 64, Height = 64 });

        // A map with no style yet reports no duration or delay. The placement flag always
        // reports, because MapLibre Native always holds a value for it.
        var empty = await map.GetStyleTransitionOptionsAsync();
        Assert.Null(empty.Duration);
        Assert.Null(empty.Delay);
        Assert.True(empty.EnablePlacementTransitions);

        // The style parser fills in its own 300ms duration for a style that declares no
        // transition.
        map.SetStyleJsonAsync("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());
        var parsed = await map.GetStyleTransitionOptionsAsync();
        Assert.Equal(300, parsed.Duration);
        Assert.Null(parsed.Delay);

        map.SetStyleJsonAsync(System.Text.Encoding.UTF8.GetBytes(transitionStyleJson));
        var declared = await map.GetStyleTransitionOptionsAsync();
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
        RuntimeEventTestHelpers.AssertCommitted(map.SetStyleTransitionOptionsAsync(options));
        Assert.Equal(options, await map.GetStyleTransitionOptionsAsync());

        // Loading a style replaces the override with what that style declares.
        map.SetStyleJsonAsync(System.Text.Encoding.UTF8.GetBytes(transitionStyleJson));
        Assert.Equal(declared, await map.GetStyleTransitionOptionsAsync());

        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.SetStyleTransitionOptionsAsync(new StyleTransitionOptions { Delay = -1 }),
            MaplibreStatus.InvalidArgument
        );
    }
}
