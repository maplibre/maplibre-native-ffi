using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Offline;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RuntimeOfflineOperationTests
{
    // Ambient-cache maintenance takes no argument the host can observe afterwards, so the
    // observable contract is that a lowered budget still leaves the database usable.
    [BindingSpecTest("BND-086")]
    [Fact]
    public async Task AmbientCacheOperationsCompleteAndLeaveTheDatabaseUsable()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions { CachePath = ":memory:" });

        await runtime.RunAmbientCacheOperationAsync(
            AmbientCacheOperation.Invalidate,
            TestContext.Current.CancellationToken
        );
        await runtime.SetMaximumAmbientCacheSizeAsync(
            8UL << 20,
            TestContext.Current.CancellationToken
        );
        await runtime.RunAmbientCacheOperationAsync(
            AmbientCacheOperation.Clear,
            TestContext.Current.CancellationToken
        );

        Assert.Empty(await runtime.ListOfflineRegionsAsync(TestContext.Current.CancellationToken));
    }

    [BindingSpecTest("BND-084", "BND-085")]
    [Fact]
    public async Task RegionMetadataStatusAndDownloadStateRoundTripThroughTasks()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions { CachePath = ":memory:" });
        var region = await runtime.CreateOfflineRegionAsync(
            Definition(),
            [7],
            TestContext.Current.CancellationToken
        );

        var updated = await runtime.UpdateOfflineRegionMetadataAsync(
            region.Id,
            [8, 9],
            TestContext.Current.CancellationToken
        );
        Assert.Equal(region.Id, updated.Id);
        Assert.Equal<byte[]>([8, 9], updated.Metadata);
        Assert.Equal<byte[]>(
            [8, 9],
            (
                await runtime.GetOfflineRegionAsync(
                    region.Id,
                    TestContext.Current.CancellationToken
                )
            )!.Metadata
        );

        await runtime.SetOfflineRegionObservedAsync(
            region.Id,
            true,
            TestContext.Current.CancellationToken
        );
        await runtime.SetOfflineRegionDownloadStateAsync(
            region.Id,
            OfflineRegionDownloadState.Inactive,
            TestContext.Current.CancellationToken
        );
        var status = await runtime.GetOfflineRegionStatusAsync(
            region.Id,
            TestContext.Current.CancellationToken
        );
        Assert.Equal(OfflineRegionDownloadState.Inactive, status.DownloadState);

        await runtime.InvalidateOfflineRegionAsync(
            region.Id,
            TestContext.Current.CancellationToken
        );
        await runtime.DeleteOfflineRegionAsync(region.Id, TestContext.Current.CancellationToken);
    }

    // A lookup of a missing region is not an error; every other operation reports not found.
    [BindingSpecTest("BND-085")]
    [Fact]
    public async Task OperationsOnAMissingRegionReportNotFound()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions { CachePath = ":memory:" });
        const long missing = 987654;

        Assert.Null(
            await runtime.GetOfflineRegionAsync(missing, TestContext.Current.CancellationToken)
        );

        foreach (
            var operation in new Func<Task>[]
            {
                () => runtime.UpdateOfflineRegionMetadataAsync(missing, [1]),
                () => runtime.GetOfflineRegionStatusAsync(missing),
                () => runtime.SetOfflineRegionObservedAsync(missing, true),
                () =>
                    runtime.SetOfflineRegionDownloadStateAsync(
                        missing,
                        OfflineRegionDownloadState.Active
                    ),
                () => runtime.InvalidateOfflineRegionAsync(missing),
                () => runtime.DeleteOfflineRegionAsync(missing),
            }
        )
        {
            var error = await Assert.ThrowsAsync<MaplibreException>(operation);
            Assert.Equal(MaplibreStatus.NotFound, error.Status);
        }
    }

    private static OfflineRegionDefinition Definition() =>
        new OfflineRegionDefinition.TilePyramid(
            "custom://offline-style.json",
            new LatLngBounds(new LatLng(0, 0), new LatLng(1, 1)),
            0,
            1,
            1,
            true
        );

    [BindingSpecTest("BND-084", "BND-085")]
    [Fact]
    public async Task OfflineRegionsAreCreatedListedAndDeletedThroughTasks()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions { CachePath = ":memory:" });
        var region = await runtime.CreateOfflineRegionAsync(
            Definition(),
            [1, 2, 3],
            TestContext.Current.CancellationToken
        );
        Assert.Equal<byte[]>([1, 2, 3], region.Metadata);
        Assert.Contains(
            await runtime.ListOfflineRegionsAsync(TestContext.Current.CancellationToken),
            listed => listed.Id == region.Id
        );

        await runtime.DeleteOfflineRegionAsync(region.Id, TestContext.Current.CancellationToken);

        Assert.DoesNotContain(
            await runtime.ListOfflineRegionsAsync(TestContext.Current.CancellationToken),
            listed => listed.Id == region.Id
        );
    }
}
