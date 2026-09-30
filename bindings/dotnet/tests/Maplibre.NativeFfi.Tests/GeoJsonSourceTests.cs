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

    [Fact]
    public async Task PreparedGeoJsonSourceDataAddsAndUpdatesThroughNativeMap()
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

        using var initial = GeoJsonSourceDataHandle.Create(EmptyFeatureCollection, null);
        using var updated = GeoJsonSourceDataHandle.Create(
            """{"type":"Point","coordinates":[2,1]}"""u8.ToArray(),
            null
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.AddGeojsonSourceDataAsync(
                "geo-data",
                initial,
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetGeojsonSourceDataAsync(
                "geo-data",
                updated,
                TestContext.Current.CancellationToken
            )
        );

        Assert.Equal(
            StyleSourceType.Geojson,
            (await map.GetStyleSourceInfoAsync("geo-data", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );
    }

    [Fact]
    public async Task ClusteredGeojsonSourceOptionsValidateDuringPreparation()
    {
        using var prepared = GeoJsonSourceDataHandle.Create(NearbyPoints, ClusterOptions());

        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        _ = map.AddGeojsonSourceDataAsync(
            "clustered",
            prepared,
            TestContext.Current.CancellationToken
        );

        Assert.Equal(
            StyleSourceType.Geojson,
            (await map.GetStyleSourceInfoAsync("clustered", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );

        // Cluster-expression validation happens at preparation, before any map exists.
        var options = ClusterOptions();
        options.ClusterProperties = """{"weight_sum":"not-an-expression"}"""u8.ToArray();

        var error = Assert.Throws<InvalidArgumentException>(() =>
            GeoJsonSourceDataHandle.Create(NearbyPoints, options)
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
    }

    [Fact]
    public async Task PreparedDataInstallsOnManySourcesAndOutlivesRelease()
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

        var prepared = GeoJsonSourceDataHandle.Create(NearbyPoints, null);

        _ = map.AddGeojsonSourceDataAsync("geo-a", prepared, TestContext.Current.CancellationToken);
        _ = map.AddGeojsonSourceDataAsync("geo-b", prepared, TestContext.Current.CancellationToken);
        var setCommand = map.SetGeojsonSourceDataAsync(
            "geo-a",
            prepared,
            TestContext.Current.CancellationToken
        );

        // Install calls borrow the handle; releasing it right after submitting the
        // commands never invalidates the sources.
        prepared.Close();
        Assert.True(prepared.IsClosed);

        RuntimeEventTestHelpers.AssertCommitted(setCommand);
        Assert.Equal(
            StyleSourceType.Geojson,
            (await map.GetStyleSourceInfoAsync("geo-a", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );
        Assert.Equal(
            StyleSourceType.Geojson,
            (await map.GetStyleSourceInfoAsync("geo-b", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );
    }

    [Fact]
    public async Task SetRejectsDataPreparedWithDifferentOptions()
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

        using var clustered = GeoJsonSourceDataHandle.Create(NearbyPoints, ClusterOptions());
        RuntimeEventTestHelpers.AssertCommitted(
            map.AddGeojsonSourceDataAsync(
                "clustered",
                clustered,
                TestContext.Current.CancellationToken
            )
        );

        // The options match happens on the map thread, so a mismatch surfaces as an
        // asynchronous command failure rather than a synchronous throw.
        using var unclustered = GeoJsonSourceDataHandle.Create(NearbyPoints, null);
        RuntimeEventTestHelpers.AssertFailed(
            map.SetGeojsonSourceDataAsync(
                "clustered",
                unclustered,
                TestContext.Current.CancellationToken
            ),
            MaplibreStatus.InvalidArgument
        );

        // Cluster aggregations are part of the options match, so data
        // prepared with different cluster properties is rejected too.
        var reaggregated = ClusterOptions();
        reaggregated.ClusterProperties = """{"weight_max":["max",["get","weight"]]}"""u8.ToArray();
        using var mismatched = GeoJsonSourceDataHandle.Create(NearbyPoints, reaggregated);
        RuntimeEventTestHelpers.AssertFailed(
            map.SetGeojsonSourceDataAsync(
                "clustered",
                mismatched,
                TestContext.Current.CancellationToken
            ),
            MaplibreStatus.InvalidArgument
        );

        Assert.Equal(
            StyleSourceType.Geojson,
            (await map.GetStyleSourceInfoAsync("clustered", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );
    }

    [Fact]
    public async Task ClosedPreparedDataRejectsUseAndCloseAgainNoOps()
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

        var prepared = GeoJsonSourceDataHandle.Create(EmptyFeatureCollection, null);
        prepared.Close();
        prepared.Close();
        prepared.Dispose();
        Assert.True(prepared.IsClosed);

        var error = Assert.Throws<InvalidStateException>(() =>
            map.AddGeojsonSourceDataAsync(
                    "geo-closed",
                    prepared,
                    TestContext.Current.CancellationToken
                )
                .GetAwaiter()
                .GetResult()
        );
        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Null(
            await map.GetStyleSourceInfoAsync("geo-closed", TestContext.Current.CancellationToken)
        );
    }

    [Fact]
    public async Task PreparationRunsOffThreadAndInstallsOnMapThread()
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

        _ = map.AddGeojsonSourceDataAsync(
            "geo-worker",
            prepared,
            TestContext.Current.CancellationToken
        );
        Assert.Equal(
            StyleSourceType.Geojson,
            (await map.GetStyleSourceInfoAsync("geo-worker", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );
    }

    [Fact]
    public void SynchronousTilingOverrideAppliesToExistingSourceOnly()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        using var prepared = GeoJsonSourceDataHandle.Create(NearbyPoints, null);
        map.AddGeojsonSourceDataAsync("geo-sync", prepared, TestContext.Current.CancellationToken);

        RuntimeEventTestHelpers.AssertCommitted(
            map.SetGeojsonSourceSynchronousTilingAsync(
                "geo-sync",
                true,
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetGeojsonSourceDataAsync(
                "geo-sync",
                prepared,
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetGeojsonSourceSynchronousTilingAsync(
                "geo-sync",
                false,
                TestContext.Current.CancellationToken
            )
        );

        // The missing-source check runs on the map thread, so it surfaces as an
        // asynchronous command failure.
        RuntimeEventTestHelpers.AssertFailed(
            map.SetGeojsonSourceSynchronousTilingAsync(
                "geo-missing",
                true,
                TestContext.Current.CancellationToken
            ),
            MaplibreStatus.NotFound
        );
    }

    private static GeojsonSourceOptions ClusterOptions() =>
        new()
        {
            Cluster = true,
            ClusterRadius = 60,
            ClusterMinPoints = 2,
            ClusterMaxZoom = 17,
            ClusterProperties = """{"weight_sum":["+",["get","weight"]]}"""u8.ToArray(),
        };
}
