using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Offline;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class RuntimeOfflineOperationTests
{
    // Support invariant for offline operation handles: operations with no result
    // data can be started, discarded, and closed idempotently.
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

    [BindingSpecTest("BND-042")]
    [Fact]
    public void RuntimeCloseFailsWhileOfflineOperationIsLiveAndCanRetryAfterDiscard()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var operation = runtime.StartAmbientCacheOperation(AmbientCacheOperation.Invalidate);

        var error = Assert.Throws<InvalidStateException>(runtime.Close);

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Null(error.RawStatus);
        Assert.False(runtime.IsClosed);
        Assert.False(operation.IsClosed);
        Assert.Contains("live child handle", error.Diagnostic, StringComparison.OrdinalIgnoreCase);

        operation.Close();
        runtime.Close();

        Assert.True(runtime.IsClosed);
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
}
