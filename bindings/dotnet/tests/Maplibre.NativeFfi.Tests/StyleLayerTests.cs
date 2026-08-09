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
    public void DemAndLocationLayerHelpersAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
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

        Assert.True(map.StyleLayerExists("hillshade"));
        Assert.Equal("hillshade", map.StyleLayerType("hillshade"));
        Assert.True(map.StyleLayerExists("relief"));
        Assert.Equal("color-relief", map.StyleLayerType("relief"));
        Assert.True(map.StyleLayerExists("location"));
        Assert.Equal("location-indicator", map.StyleLayerType("location"));
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public void LayerBaseAccessorsRoundTripThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 64, Height = 64 });
        map.SetStyleJson(
            System.Text.Encoding.UTF8.GetBytes(
                "{\"version\":8,\"sources\":{\"geo\":{\"type\":\"geojson\",\"data\":"
                    + "{\"type\":\"FeatureCollection\",\"features\":[]}}},\"layers\":["
                    + "{\"id\":\"bg\",\"type\":\"background\"},"
                    + "{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"geo\"}]}"
            )
        );

        Assert.Equal(string.Empty, map.GetLayerSourceLayer("fill"));
        map.SetLayerSourceLayer("fill", "roads");
        Assert.Equal("roads", map.GetLayerSourceLayer("fill"));
        Assert.Equal("geo", map.GetLayerSourceId("fill"));

        // A layer type that takes no source is rejected rather than silently ignored.
        Assert.Throws<InvalidArgumentException>(() => map.SetLayerSourceLayer("bg", "roads"));
        Assert.Equal(string.Empty, map.GetLayerSourceId("bg"));

        // An unset zoom range crosses the boundary as infinities.
        Assert.Equal(double.NegativeInfinity, map.GetLayerMinZoom("fill"));
        Assert.Equal(double.PositiveInfinity, map.GetLayerMaxZoom("fill"));
        map.SetLayerMinZoom("fill", 4);
        map.SetLayerMaxZoom("fill", 12.5);
        Assert.Equal(4, map.GetLayerMinZoom("fill"));
        Assert.Equal(12.5, map.GetLayerMaxZoom("fill"));

        Assert.Equal(StyleLayerVisibility.Visible, map.GetLayerVisibility("fill"));
        map.SetLayerVisibility("fill", StyleLayerVisibility.None);
        Assert.Equal(StyleLayerVisibility.None, map.GetLayerVisibility("fill"));

        // An unknown raw visibility passes through to C, which rejects it.
        Assert.Throws<InvalidArgumentException>(() =>
            map.SetLayerVisibility("fill", (StyleLayerVisibility)900)
        );
        Assert.Throws<InvalidArgumentException>(() => map.GetLayerMinZoom("missing"));
    }

    [BindingSpecTest("BND-061")]
    [Fact]
    public void StyleTransitionOptionsRoundTripThroughNativeMap()
    {
        const string transitionStyleJson =
            "{\"version\":8,\"transition\":{\"duration\":750,\"delay\":100},"
            + "\"sources\":{},\"layers\":[]}";
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 64, Height = 64 });

        // A map with no style yet reports no duration or delay. The placement flag always
        // reports, because MapLibre Native always holds a value for it.
        var empty = map.GetStyleTransitionOptions();
        Assert.Null(empty.Duration);
        Assert.Null(empty.Delay);
        Assert.True(empty.EnablePlacementTransitions);

        // The style parser fills in its own 300ms duration for a style that declares no
        // transition.
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());
        var parsed = map.GetStyleTransitionOptions();
        Assert.Equal(300, parsed.Duration);
        Assert.Null(parsed.Delay);

        map.SetStyleJson(System.Text.Encoding.UTF8.GetBytes(transitionStyleJson));
        var declared = map.GetStyleTransitionOptions();
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
        map.SetStyleTransitionOptions(options);
        Assert.Equal(options, map.GetStyleTransitionOptions());

        // Loading a style replaces the override with what that style declares.
        map.SetStyleJson(System.Text.Encoding.UTF8.GetBytes(transitionStyleJson));
        Assert.Equal(declared, map.GetStyleTransitionOptions());

        Assert.Throws<InvalidArgumentException>(() =>
            map.SetStyleTransitionOptions(new StyleTransitionOptions { Delay = -1 })
        );
    }
}
