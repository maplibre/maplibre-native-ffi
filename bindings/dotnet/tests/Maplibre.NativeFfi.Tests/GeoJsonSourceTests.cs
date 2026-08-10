using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class GeoJsonSourceTests
{
    private static readonly byte[] EmptyFeatureCollection =
        """{"type":"FeatureCollection","features":[]}"""u8.ToArray();

    private static readonly byte[] NearbyPoints =
        """{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},"properties":{"weight":1}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},"properties":{"weight":2}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},"properties":{"weight":3}}]}"""u8.ToArray();

    [BindingSpecTest("BND-065", "BND-105")]
    [Fact]
    public void GeoJsonSourceDataAdaptsThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        map.AddGeoJsonSourceData("geo-data", EmptyFeatureCollection, null);
        map.SetGeoJsonSourceData(
            "geo-data",
            """{"type":"Point","coordinates":[2,1]}"""u8.ToArray()
        );

        Assert.True(map.StyleSourceExists("geo-data"));
        Assert.Equal(SourceType.GeoJson, map.StyleSourceType("geo-data"));
    }

    [BindingSpecTest("BND-060", "BND-105")]
    [Fact]
    public void ClusteredGeoJsonSourceOptionsParseThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        map.AddGeoJsonSourceData("clustered", NearbyPoints, ClusterOptions());

        Assert.True(map.StyleSourceExists("clustered"));
        Assert.Equal(SourceType.GeoJson, map.StyleSourceType("clustered"));

        var options = ClusterOptions();
        options.ClusterProperties = """{"weight_sum":"not-an-expression"}"""u8.ToArray();

        var error = Assert.Throws<InvalidArgumentException>(() =>
            map.AddGeoJsonSourceData("clustered-invalid", NearbyPoints, options)
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.False(map.StyleSourceExists("clustered-invalid"));
    }

    private static GeoJsonSourceOptions ClusterOptions() =>
        new()
        {
            Cluster = true,
            ClusterRadius = 60,
            ClusterMinimumPoints = 2,
            ClusterMaximumZoom = 17,
            ClusterProperties = """{"weight_sum":["+",["get","weight"]]}"""u8.ToArray(),
        };
}
