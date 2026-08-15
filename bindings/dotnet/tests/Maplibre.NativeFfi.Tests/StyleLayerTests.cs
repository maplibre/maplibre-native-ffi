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
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());
        map.AddRasterDemSourceTiles("dem", ["https://example.test/dem/{z}/{x}/{y}.png"], null);

        map.AddHillshadeLayer("hillshade", "dem", "");
        map.AddColorReliefLayer("relief", "dem", "");
        map.AddLocationIndicatorLayer("location", "");
        map.SetLocationIndicatorLocation("location", new LatLng(12.5, 34.25), 100);
        map.SetLocationIndicatorBearing("location", 45);
        map.SetLocationIndicatorAccuracyRadius("location", 12);
        map.SetLocationIndicatorImageName(
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
        map.SetStyleJson(
            System.Text.Encoding.UTF8.GetBytes(
                "{\"version\":8,\"sources\":{\"geo\":{\"type\":\"geojson\",\"data\":"
                    + "{\"type\":\"FeatureCollection\",\"features\":[]}}},\"layers\":["
                    + "{\"id\":\"bg\",\"type\":\"background\"},"
                    + "{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"geo\"}]}"
            )
        );

        Assert.Equal(string.Empty, await map.GetLayerSourceLayerAsync("fill"));
        Assert.NotEqual(0ul, map.SetLayerSourceLayer("fill", "roads"));
        Assert.Equal("roads", await map.GetLayerSourceLayerAsync("fill"));
        Assert.Equal("geo", await map.GetLayerSourceIdAsync("fill"));

        // A layer type that takes no source preserves its empty source ID.
        Assert.NotEqual(0ul, map.SetLayerSourceLayer("bg", "roads"));
        Assert.Equal(string.Empty, await map.GetLayerSourceIdAsync("bg"));

        // An unset zoom range crosses the boundary as infinities.
        var unset = Assert.IsType<LayerInfo>(await map.StyleLayerInfoAsync("fill"));
        Assert.Equal(double.NegativeInfinity, unset.MinZoom);
        Assert.Equal(double.PositiveInfinity, unset.MaxZoom);
        Assert.NotEqual(0ul, map.SetLayerMinZoom("fill", 4));
        Assert.NotEqual(0ul, map.SetLayerMaxZoom("fill", 12.5));

        Assert.Equal(StyleLayerVisibility.Visible, unset.Visibility);
        Assert.NotEqual(0ul, map.SetLayerVisibility("fill", StyleLayerVisibility.None));

        // The layer-info aggregate reports everything at once, and its source ID and
        // source layer match the dedicated copy operations.
        var info = Assert.IsType<LayerInfo>(await map.StyleLayerInfoAsync("fill"));
        Assert.Equal("fill", info.Id);
        Assert.Equal("fill", info.Type);
        Assert.Equal(4, info.MinZoom);
        Assert.Equal(12.5, info.MaxZoom);
        Assert.Equal(StyleLayerVisibility.None, info.Visibility);
        Assert.Equal((uint)StyleLayerVisibility.None, info.RawVisibility);
        Assert.Equal("geo", info.SourceId);
        Assert.Equal("roads", info.SourceLayer);
        Assert.Equal(await map.GetLayerSourceIdAsync("fill"), info.SourceId);
        Assert.Equal(await map.GetLayerSourceLayerAsync("fill"), info.SourceLayer);

        // An unknown raw visibility is accepted as a command, then leaves the value unchanged.
        Assert.NotEqual(0ul, map.SetLayerVisibility("fill", (StyleLayerVisibility)900));
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
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());
        var parsed = await map.GetStyleTransitionOptionsAsync();
        Assert.Equal(300, parsed.Duration);
        Assert.Null(parsed.Delay);

        map.SetStyleJson(System.Text.Encoding.UTF8.GetBytes(transitionStyleJson));
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
        Assert.NotEqual(0ul, map.SetStyleTransitionOptions(options));
        Assert.Equal(options, await map.GetStyleTransitionOptionsAsync());

        // Loading a style replaces the override with what that style declares.
        map.SetStyleJson(System.Text.Encoding.UTF8.GetBytes(transitionStyleJson));
        Assert.Equal(declared, await map.GetStyleTransitionOptionsAsync());

        var rejected = map.SetStyleTransitionOptions(new StyleTransitionOptions { Delay = -1 });
        var failure = RuntimeEventTestHelpers.WaitForCommand(runtime, rejected);
        var completion = Assert.IsType<RuntimeEventPayload.CommandFinished>(failure.Payload);
        Assert.Equal(CommandDisposition.Failed, completion.Disposition);
        Assert.Equal((int)MaplibreStatus.InvalidArgument, failure.Code);
        Assert.NotEmpty(failure.Message);
    }
}
