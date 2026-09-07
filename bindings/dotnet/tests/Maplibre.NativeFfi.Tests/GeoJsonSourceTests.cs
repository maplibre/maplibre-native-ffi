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
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        using var initial = GeoJsonSourceDataHandle.Create(EmptyFeatureCollection, null);
        using var updated = GeoJsonSourceDataHandle.Create(
            """{"type":"Point","coordinates":[2,1]}"""u8.ToArray(),
            null
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.AddGeoJsonSourceDataAsync(
                "geo-data",
                initial,
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetGeoJsonSourceDataAsync(
                "geo-data",
                updated,
                TestContext.Current.CancellationToken
            )
        );

        Assert.Equal(
            SourceType.GeoJson,
            (
                await map.StyleSourceInfoAsync("geo-data", TestContext.Current.CancellationToken)
            )?.Type
        );
    }

    [BindingSpecTest("BND-060", "BND-105")]
    [Fact]
    public async Task ClusteredGeoJsonSourceOptionsValidateDuringPreparation()
    {
        using var prepared = GeoJsonSourceDataHandle.Create(NearbyPoints, ClusterOptions());

        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        _ = map.AddGeoJsonSourceDataAsync(
            "clustered",
            prepared,
            TestContext.Current.CancellationToken
        );

        Assert.Equal(
            SourceType.GeoJson,
            (
                await map.StyleSourceInfoAsync("clustered", TestContext.Current.CancellationToken)
            )?.Type
        );

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
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        var prepared = GeoJsonSourceDataHandle.Create(NearbyPoints, null);

        _ = map.AddGeoJsonSourceDataAsync("geo-a", prepared, TestContext.Current.CancellationToken);
        _ = map.AddGeoJsonSourceDataAsync("geo-b", prepared, TestContext.Current.CancellationToken);
        var setCommand = map.SetGeoJsonSourceDataAsync(
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
            SourceType.GeoJson,
            (await map.StyleSourceInfoAsync("geo-a", TestContext.Current.CancellationToken))?.Type
        );
        Assert.Equal(
            SourceType.GeoJson,
            (await map.StyleSourceInfoAsync("geo-b", TestContext.Current.CancellationToken))?.Type
        );
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task SetRejectsDataPreparedWithDifferentOptions()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        using var clustered = GeoJsonSourceDataHandle.Create(NearbyPoints, ClusterOptions());
        RuntimeEventTestHelpers.AssertCommitted(
            map.AddGeoJsonSourceDataAsync(
                "clustered",
                clustered,
                TestContext.Current.CancellationToken
            )
        );

        // The options match happens on the map thread, so a mismatch surfaces as an
        // asynchronous command failure rather than a synchronous throw.
        using var unclustered = GeoJsonSourceDataHandle.Create(NearbyPoints, null);
        RuntimeEventTestHelpers.AssertFailed(
            map.SetGeoJsonSourceDataAsync(
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
            map.SetGeoJsonSourceDataAsync(
                "clustered",
                mismatched,
                TestContext.Current.CancellationToken
            ),
            MaplibreStatus.InvalidArgument
        );

        Assert.Equal(
            SourceType.GeoJson,
            (
                await map.StyleSourceInfoAsync("clustered", TestContext.Current.CancellationToken)
            )?.Type
        );
    }

    [BindingSpecTest("BND-023", "BND-040")]
    [Fact]
    public async Task ClosedPreparedDataRejectsUseAndCloseAgainNoOps()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        var prepared = GeoJsonSourceDataHandle.Create(EmptyFeatureCollection, null);
        prepared.Close();
        prepared.Close();
        prepared.Dispose();
        Assert.True(prepared.IsClosed);

        var error = Assert.Throws<InvalidStateException>(() =>
            map.AddGeoJsonSourceDataAsync(
                    "geo-closed",
                    prepared,
                    TestContext.Current.CancellationToken
                )
                .GetAwaiter()
                .GetResult()
        );
        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Null(
            await map.StyleSourceInfoAsync("geo-closed", TestContext.Current.CancellationToken)
        );
    }

    [BindingSpecTest("BND-065", "BND-105")]
    [Fact]
    public async Task PreparationRunsOffThreadAndInstallsOnMapThread()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
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

        _ = map.AddGeoJsonSourceDataAsync(
            "geo-worker",
            prepared,
            TestContext.Current.CancellationToken
        );
        Assert.Equal(
            SourceType.GeoJson,
            (
                await map.StyleSourceInfoAsync("geo-worker", TestContext.Current.CancellationToken)
            )?.Type
        );
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public void SynchronousTilingOverrideAppliesToExistingSourceOnly()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        using var prepared = GeoJsonSourceDataHandle.Create(NearbyPoints, null);
        map.AddGeoJsonSourceDataAsync("geo-sync", prepared, TestContext.Current.CancellationToken);

        RuntimeEventTestHelpers.AssertCommitted(
            map.SetGeoJsonSourceSynchronousTilingAsync(
                "geo-sync",
                true,
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetGeoJsonSourceDataAsync(
                "geo-sync",
                prepared,
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetGeoJsonSourceSynchronousTilingAsync(
                "geo-sync",
                false,
                TestContext.Current.CancellationToken
            )
        );

        // The missing-source check runs on the map thread, so it surfaces as an
        // asynchronous command failure.
        RuntimeEventTestHelpers.AssertFailed(
            map.SetGeoJsonSourceSynchronousTilingAsync(
                "geo-missing",
                true,
                TestContext.Current.CancellationToken
            ),
            MaplibreStatus.NotFound
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
