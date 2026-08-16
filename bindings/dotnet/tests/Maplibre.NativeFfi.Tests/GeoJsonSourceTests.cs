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
    public async Task PreparedGeoJsonSourceDataAddsAndUpdatesThroughNativeMap()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        using var initial = GeoJsonSourceDataHandle.Create(EmptyFeatureCollection, null);
        using var updated = GeoJsonSourceDataHandle.Create(
            """{"type":"Point","coordinates":[2,1]}"""u8.ToArray(),
            null
        );

        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.AddGeoJsonSourceData("geo-data", initial),
            MaplibreStatus.Ok
        );
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.SetGeoJsonSourceData("geo-data", updated),
            MaplibreStatus.Ok
        );

        Assert.Equal(SourceType.GeoJson, (await map.StyleSourceInfoAsync("geo-data"))?.Type);
    }

    [BindingSpecTest("BND-060", "BND-105")]
    [Fact]
    public async Task ClusteredGeoJsonSourceOptionsValidateDuringPreparation()
    {
        using var prepared = GeoJsonSourceDataHandle.Create(NearbyPoints, ClusterOptions());

        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        map.AddGeoJsonSourceData("clustered", prepared);

        Assert.Equal(SourceType.GeoJson, (await map.StyleSourceInfoAsync("clustered"))?.Type);

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
    public async Task PreparedDataInstallsOnManySourcesAndOutlivesRelease()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        var prepared = GeoJsonSourceDataHandle.Create(NearbyPoints, null);

        map.AddGeoJsonSourceData("geo-a", prepared);
        map.AddGeoJsonSourceData("geo-b", prepared);
        var setCommand = map.SetGeoJsonSourceData("geo-a", prepared);

        // Install calls borrow the handle; releasing it right after submitting the
        // commands never invalidates the sources.
        prepared.Close();
        Assert.True(prepared.IsClosed);

        RuntimeEventTestHelpers.AssertCommandFinishes(runtime, setCommand, MaplibreStatus.Ok);
        Assert.Equal(SourceType.GeoJson, (await map.StyleSourceInfoAsync("geo-a"))?.Type);
        Assert.Equal(SourceType.GeoJson, (await map.StyleSourceInfoAsync("geo-b"))?.Type);
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task SetRejectsDataPreparedWithDifferentOptions()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        using var clustered = GeoJsonSourceDataHandle.Create(NearbyPoints, ClusterOptions());
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.AddGeoJsonSourceData("clustered", clustered),
            MaplibreStatus.Ok
        );

        // The options match happens on the map thread, so a mismatch surfaces as an
        // asynchronous command failure rather than a synchronous throw.
        using var unclustered = GeoJsonSourceDataHandle.Create(NearbyPoints, null);
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.SetGeoJsonSourceData("clustered", unclustered),
            MaplibreStatus.InvalidArgument
        );

        // Cluster aggregations are part of the options match, so data
        // prepared with different cluster properties is rejected too.
        var reaggregated = ClusterOptions();
        reaggregated.ClusterProperties = """{"weight_max":["max",["get","weight"]]}"""u8.ToArray();
        using var mismatched = GeoJsonSourceDataHandle.Create(NearbyPoints, reaggregated);
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.SetGeoJsonSourceData("clustered", mismatched),
            MaplibreStatus.InvalidArgument
        );

        Assert.Equal(SourceType.GeoJson, (await map.StyleSourceInfoAsync("clustered"))?.Type);
    }

    [BindingSpecTest("BND-023", "BND-040")]
    [Fact]
    public async Task ClosedPreparedDataRejectsUseAndCloseAgainNoOps()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
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
        Assert.Null(await map.StyleSourceInfoAsync("geo-closed"));
    }

    [BindingSpecTest("BND-065", "BND-105")]
    [Fact]
    public async Task PreparationRunsOffThreadAndInstallsOnMapThread()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        // Preparation is legal from any thread, so it runs on a dedicated worker.
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
        Assert.Equal(SourceType.GeoJson, (await map.StyleSourceInfoAsync("geo-worker"))?.Type);
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public void SynchronousTilingOverrideAppliesToExistingSourceOnly()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        using var prepared = GeoJsonSourceDataHandle.Create(NearbyPoints, null);
        map.AddGeoJsonSourceData("geo-sync", prepared);

        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.SetGeoJsonSourceSynchronousTiling("geo-sync", true),
            MaplibreStatus.Ok
        );
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.SetGeoJsonSourceData("geo-sync", prepared),
            MaplibreStatus.Ok
        );
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.SetGeoJsonSourceSynchronousTiling("geo-sync", false),
            MaplibreStatus.Ok
        );

        // The missing-source check runs on the map thread, so it surfaces as an
        // asynchronous command failure.
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.SetGeoJsonSourceSynchronousTiling("geo-missing", true),
            MaplibreStatus.InvalidArgument
        );
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
