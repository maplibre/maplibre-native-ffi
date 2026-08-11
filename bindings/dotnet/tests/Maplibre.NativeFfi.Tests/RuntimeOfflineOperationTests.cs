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
    public void AmbientCacheOperationCanBeStartedAndDiscarded()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());

        using var operation = runtime.StartAmbientCacheOperation(AmbientCacheOperation.Invalidate);

        Assert.NotEqual(0u, operation.Id);
        Assert.Equal(OfflineOperationKind.AmbientCache, operation.Kind);
        Assert.Equal(OfflineOperationResultKind.None, operation.ResultKind);
        Assert.False(operation.IsClosed);

        operation.Close();
        operation.Close();

        Assert.True(operation.IsClosed);
    }

    [Fact]
    public void SetMaximumAmbientCacheSizeCanBeStartedAndDiscarded()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions { CachePath = ":memory:" });

        using var operation = runtime.StartSetMaximumAmbientCacheSize(8UL << 20);

        Assert.NotEqual(0u, operation.Id);
        Assert.Equal(OfflineOperationKind.SetMaximumAmbientCacheSize, operation.Kind);
        Assert.Equal(OfflineOperationResultKind.None, operation.ResultKind);

        operation.Close();
        Assert.True(operation.IsClosed);
    }

    [Fact]
    public void OperationCloseAfterRuntimeCloseMarksOperationClosed()
    {
        var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var operation = runtime.StartAmbientCacheOperation(AmbientCacheOperation.Invalidate);

        runtime.Close();

        operation.Close();
        Assert.True(operation.IsClosed);
        operation.Close();
    }

    [BindingSpecTest("BND-084")]
    [Fact]
    public unsafe void FailedOfflineStatusTakeResultLeavesOperationLiveForRetry()
    {
        var calls = 0;
        using var take = RuntimeHandle.UseOfflineTakeResultMethodsForTest(
            (_, operationId, status) =>
            {
                Assert.Equal(77u, operationId);
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
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var operation = new OfflineOperationHandle(
            runtime,
            77,
            OfflineOperationKind.RegionGetStatus,
            OfflineOperationResultKind.RegionStatus
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
    public void OfflineRegionsAreCreatedListedAndDeletedThroughDrainedCompletions()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions { CachePath = ":memory:" });
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
        delete.Close();

        using var listAgain = runtime.StartOfflineRegions();
        CompleteOperation(runtime, listAgain);
        Assert.DoesNotContain(
            runtime.TakeOfflineRegionsResult(listAgain),
            listed => listed.Id == region.Id
        );
    }

    // Pumps and drains until the operation reports the completion event that makes its result
    // available, which is the only signal a host has for taking one.
    private static void CompleteOperation(RuntimeHandle runtime, OfflineOperationHandle operation)
    {
        for (var attempt = 0; attempt < 1000; attempt++)
        {
            runtime.Pump(TimeSpan.FromMilliseconds(1));
            foreach (var drained in runtime.DrainEvents().Events)
            {
                if (
                    drained.Payload is RuntimeEventPayload.OfflineOperationCompleted completed
                    && completed.OperationId == operation.Id
                )
                {
                    Assert.Equal((int)MaplibreStatus.Ok, drained.Code);
                    return;
                }
            }
        }

        throw new TimeoutException($"Offline operation {operation.Id} never completed.");
    }
}
