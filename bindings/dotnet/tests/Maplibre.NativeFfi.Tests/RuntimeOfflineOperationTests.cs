using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Offline;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RuntimeOfflineOperationTests
{
    [Fact]
    public void AmbientCacheOperationCanBeStartedAndReleased()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());

        using var operation = runtime.StartAmbientCacheOperation(AmbientCacheOperation.Invalidate);

        Assert.False(operation.IsClosed);

        operation.Close();
        operation.Close();

        Assert.True(operation.IsClosed);
    }

    [Fact]
    public void SetMaximumAmbientCacheSizeCanBeStartedAndReleased()
    {
        using var runtime = TestHandles.CreateRuntime(
            new RuntimeOptions { CachePath = ":memory:" }
        );

        using var operation = runtime.StartSetMaximumAmbientCacheSize(8UL << 20);

        operation.Close();
        Assert.True(operation.IsClosed);
    }

    [Fact]
    public void OperationCanBeReleasedAfterRuntimeClose()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var operation = runtime.StartAmbientCacheOperation(AmbientCacheOperation.Invalidate);

        TestHandles.Close(runtime);
        Assert.True(runtime.IsClosed);

        operation.Close();
        Assert.True(operation.IsClosed);
    }

    [BindingSpecTest("BND-084")]
    [Fact]
    public unsafe void FailedOfflineStatusTakeResultLeavesOperationLiveForRetry()
    {
        var calls = 0;
        using var take = RuntimeHandle.UseOfflineTakeResultMethodsForTest(
            (_, operationId, status) =>
            {
                Assert.Equal(77u, operationId.Value);
                calls++;
                if (calls == 1)
                {
                    return mln_status.MLN_STATUS_INVALID_STATE;
                }

                *status = new mln_offline_region_status
                {
                    download_state = (uint)
                        mln_offline_region_download_state.MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE,
                    completed_resource_count = 1,
                    completed_resource_size = 2,
                    completed_tile_count = 3,
                    required_tile_count = 4,
                    completed_tile_size = 5,
                    required_resource_count = 6,
                    required_resource_count_is_precise = 1,
                    complete = 1,
                };
                return mln_status.MLN_STATUS_OK;
            }
        );
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var operation = new OperationHandle(
            runtime,
            new MlnOperation(77),
            OperationResultKind.RegionStatus
        );

        var error = Assert.Throws<InvalidStateException>(() =>
            runtime.TakeOfflineRegionStatusResult(operation)
        );

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.False(operation.IsClosed);

        var status = runtime.TakeOfflineRegionStatusResult(operation);

        Assert.True(operation.IsClosed);
        Assert.Equal(2, calls);
        Assert.Equal(OfflineRegionDownloadState.Active, status.DownloadState);
        Assert.Equal(6u, status.RequiredResourceCount);
        Assert.True(status.RequiredResourceCountIsPrecise);
        Assert.True(status.Complete);
    }

    [BindingSpecTest("BND-084", "BND-085")]
    [Fact]
    public void OfflineRegionsAreCreatedListedAndDeletedThroughOperationHandles()
    {
        using var runtime = TestHandles.CreateRuntime(
            new RuntimeOptions { CachePath = ":memory:" }
        );
        var definition = new OfflineRegionDefinition.TilePyramid(
            "custom://offline-style.json",
            new LatLngBounds(new LatLng(0, 0), new LatLng(1, 1)),
            0,
            1,
            1,
            true
        );
        using var create = runtime.StartCreateOfflineRegion(definition, [1, 2, 3]);
        CompleteOperation(runtime, create);
        var region = runtime.TakeCreateOfflineRegionResult(create);
        Assert.Equal<byte[]>([1, 2, 3], region.Metadata);

        using var list = runtime.StartOfflineRegions();
        CompleteOperation(runtime, list);
        Assert.Contains(runtime.TakeOfflineRegionsResult(list), listed => listed.Id == region.Id);

        using var delete = runtime.StartDeleteOfflineRegion(region.Id);
        CompleteOperation(runtime, delete);
        delete.Finish();
        Assert.True(delete.IsClosed);

        using var listAgain = runtime.StartOfflineRegions();
        CompleteOperation(runtime, listAgain);
        Assert.DoesNotContain(
            runtime.TakeOfflineRegionsResult(listAgain),
            listed => listed.Id == region.Id
        );
    }

    private static void CompleteOperation(RuntimeHandle runtime, OperationHandle operation)
    {
        operation.WaitAsync(TestContext.Current.CancellationToken).GetAwaiter().GetResult();
        var completion = operation.GetCompletion();
        Assert.Equal(MaplibreStatus.Ok, completion.Status);
        Assert.Equal((int)MaplibreStatus.Ok, completion.RawStatus);
    }
}
