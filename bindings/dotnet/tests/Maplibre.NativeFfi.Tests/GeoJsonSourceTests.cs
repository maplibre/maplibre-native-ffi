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
    public void PreparedGeoJsonSourceDataAddsAndUpdatesThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        using var initial = GeoJsonSourceDataHandle.Create(EmptyFeatureCollection, null);
        using var updated = GeoJsonSourceDataHandle.Create(
            """{"type":"Point","coordinates":[2,1]}"""u8.ToArray(),
            null
        );

        map.AddGeoJsonSourceData("geo-data", initial);
        map.SetGeoJsonSourceData("geo-data", updated);

        Assert.True(map.StyleSourceExists("geo-data"));
        Assert.Equal(SourceType.GeoJson, map.StyleSourceType("geo-data"));
    }

    [BindingSpecTest("BND-060", "BND-105")]
    [Fact]
    public void ClusteredGeoJsonSourceOptionsValidateDuringPreparation()
    {
        using var prepared = GeoJsonSourceDataHandle.Create(NearbyPoints, ClusterOptions());

        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        map.AddGeoJsonSourceData("clustered", prepared);

        Assert.True(map.StyleSourceExists("clustered"));
        Assert.Equal(SourceType.GeoJson, map.StyleSourceType("clustered"));

        // Cluster-expression validation happens at preparation, before any map exists.
        var options = ClusterOptions();
        options.ClusterProperties = """{"weight_sum":"not-an-expression"}"""u8.ToArray();

        var error = Assert.Throws<InvalidArgumentException>(() =>
            GeoJsonSourceDataHandle.Create(NearbyPoints, options)
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public void PreparedDataInstallsOnManySourcesAndOutlivesRelease()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        var prepared = GeoJsonSourceDataHandle.Create(NearbyPoints, null);

        map.AddGeoJsonSourceData("geo-a", prepared);
        map.AddGeoJsonSourceData("geo-b", prepared);
        map.SetGeoJsonSourceData("geo-a", prepared);

        // Install calls borrow the handle; releasing it never invalidates the sources.
        prepared.Close();
        Assert.True(prepared.IsClosed);

        Assert.True(map.StyleSourceExists("geo-a"));
        Assert.True(map.StyleSourceExists("geo-b"));
        Assert.Equal(SourceType.GeoJson, map.StyleSourceType("geo-a"));
        Assert.Equal(SourceType.GeoJson, map.StyleSourceType("geo-b"));
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public void SetRejectsDataPreparedWithDifferentOptions()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        using var clustered = GeoJsonSourceDataHandle.Create(NearbyPoints, ClusterOptions());
        map.AddGeoJsonSourceData("clustered", clustered);

        using var unclustered = GeoJsonSourceDataHandle.Create(NearbyPoints, null);
        var error = Assert.Throws<InvalidArgumentException>(() =>
            map.SetGeoJsonSourceData("clustered", unclustered)
        );
        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);

        // Cluster aggregations are part of the options match, so data
        // prepared with different cluster properties is rejected too.
        var reaggregated = ClusterOptions();
        reaggregated.ClusterProperties = """{"weight_max":["max",["get","weight"]]}"""u8.ToArray();
        using var mismatched = GeoJsonSourceDataHandle.Create(NearbyPoints, reaggregated);
        var propertiesError = Assert.Throws<InvalidArgumentException>(() =>
            map.SetGeoJsonSourceData("clustered", mismatched)
        );
        Assert.Equal(MaplibreStatus.InvalidArgument, propertiesError.Status);
    }

    [BindingSpecTest("BND-023", "BND-040")]
    [Fact]
    public void ClosedPreparedDataRejectsUseAndCloseAgainNoOps()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        var prepared = GeoJsonSourceDataHandle.Create(EmptyFeatureCollection, null);
        prepared.Close();
        prepared.Close();
        prepared.Dispose();
        Assert.True(prepared.IsClosed);

        var error = Assert.Throws<InvalidStateException>(() =>
            map.AddGeoJsonSourceData("geo-closed", prepared)
        );
        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.False(map.StyleSourceExists("geo-closed"));
    }

    [BindingSpecTest("BND-065", "BND-105")]
    [Fact]
    public void PreparationRunsOffThreadAndInstallsOnMapThread()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        // The map stays on its owner thread, so it joins a dedicated worker instead of awaiting.
        GeoJsonSourceDataHandle? worked = null;
        Exception? failure = null;
        var worker = new Thread(() =>
        {
            try
            {
                worked = GeoJsonSourceDataHandle.Create(NearbyPoints, ClusterOptions());
            }
            catch (Exception exception)
            {
                failure = exception;
            }
        });
        worker.Start();
        worker.Join();
        Assert.Null(failure);
        using var prepared = Assert.IsType<GeoJsonSourceDataHandle>(worked);

        map.AddGeoJsonSourceData("geo-worker", prepared);
        Assert.True(map.StyleSourceExists("geo-worker"));
        Assert.Equal(SourceType.GeoJson, map.StyleSourceType("geo-worker"));
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public void SynchronousTilingOverrideAppliesToExistingSourceOnly()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        using var prepared = GeoJsonSourceDataHandle.Create(NearbyPoints, null);
        map.AddGeoJsonSourceData("geo-sync", prepared);

        map.SetGeoJsonSourceSynchronousTiling("geo-sync", true);
        map.SetGeoJsonSourceData("geo-sync", prepared);
        map.SetGeoJsonSourceSynchronousTiling("geo-sync", false);

        var error = Assert.Throws<InvalidArgumentException>(() =>
            map.SetGeoJsonSourceSynchronousTiling("geo-missing", true)
        );
        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
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
