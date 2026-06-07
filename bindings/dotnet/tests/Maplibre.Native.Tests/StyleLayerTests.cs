using Maplibre.Native.Geo;
using Maplibre.Native.Map;
using Maplibre.Native.Runtime;
using Maplibre.Native.Style;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class StyleLayerTests
{
    [Fact]
    public void DemAndLocationLayerHelpersAdaptThroughNativeMap()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");
        map.AddRasterDemSourceTiles("dem", ["https://example.test/dem/{z}/{x}/{y}.png"]);

        map.AddHillshadeLayer("hillshade", "dem");
        map.AddColorReliefLayer("relief", "dem");
        map.AddLocationIndicatorLayer("location");
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
}
