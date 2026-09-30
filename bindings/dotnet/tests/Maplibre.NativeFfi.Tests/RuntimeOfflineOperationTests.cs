using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RuntimeOfflineOperationTests
{
    // Ambient-cache maintenance takes no argument the host can observe afterwards, so the
    // observable contract is that a lowered budget still leaves the database usable.
    [Fact]
    public async Task AmbientCacheOperationsCompleteAndLeaveTheDatabaseUsable()
    {
        using var runtime = RuntimeHandle.Create(
            RuntimeOptions.Default with
            {
                CachePath = ":memory:",
            }
        );

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

        Assert.Empty(await runtime.OfflineRegionsListAsync(TestContext.Current.CancellationToken));
    }

    [Fact]
    public async Task RegionMetadataStatusAndDownloadStateRoundTripThroughTasks()
    {
        using var runtime = RuntimeHandle.Create(
            RuntimeOptions.Default with
            {
                CachePath = ":memory:",
            }
        );
        var region = await runtime.OfflineRegionCreateAsync(
            Definition(),
            [7],
            TestContext.Current.CancellationToken
        );

        var updated = await runtime.OfflineRegionUpdateMetadataAsync(
            region.Id,
            [8, 9],
            TestContext.Current.CancellationToken
        );
        Assert.Equal(region.Id, updated.Id);
        Assert.Equal<byte[]>([8, 9], updated.Metadata);
        Assert.Equal<byte[]>(
            [8, 9],
            (await runtime.OfflineRegionGetAsync(region.Id, TestContext.Current.CancellationToken))!
                .Value
                .Metadata
        );

        await runtime.OfflineRegionSetObservedAsync(
            region.Id,
            true,
            TestContext.Current.CancellationToken
        );
        await runtime.OfflineRegionSetDownloadStateAsync(
            region.Id,
            OfflineRegionDownloadState.Inactive,
            TestContext.Current.CancellationToken
        );
        var status = await runtime.OfflineRegionGetStatusAsync(
            region.Id,
            TestContext.Current.CancellationToken
        );
        Assert.Equal(OfflineRegionDownloadState.Inactive, status.DownloadState);

        await runtime.OfflineRegionInvalidateAsync(
            region.Id,
            TestContext.Current.CancellationToken
        );
        await runtime.OfflineRegionDeleteAsync(region.Id, TestContext.Current.CancellationToken);
    }

    // A lookup of a missing region is not an error; every other operation reports not found.
    [Fact]
    public async Task OperationsOnAMissingRegionReportNotFound()
    {
        using var runtime = RuntimeHandle.Create(
            RuntimeOptions.Default with
            {
                CachePath = ":memory:",
            }
        );
        const long missing = 987654;

        Assert.Null(
            await runtime.OfflineRegionGetAsync(missing, TestContext.Current.CancellationToken)
        );

        foreach (
            var operation in new Func<Task>[]
            {
                () => runtime.OfflineRegionUpdateMetadataAsync(missing, [1]),
                () => runtime.OfflineRegionGetStatusAsync(missing),
                () => runtime.OfflineRegionSetObservedAsync(missing, true),
                () =>
                    runtime.OfflineRegionSetDownloadStateAsync(
                        missing,
                        OfflineRegionDownloadState.Active
                    ),
                () => runtime.OfflineRegionInvalidateAsync(missing),
                () => runtime.OfflineRegionDeleteAsync(missing),
            }
        )
        {
            var error = await Assert.ThrowsAsync<MaplibreException>(operation);
            Assert.Equal(MaplibreStatus.NotFound, error.Status);
        }
    }

    private static OfflineRegionDefinition Definition() =>
        new OfflineRegionDefinition.TilePyramid(
            new OfflineTilePyramidRegionDefinition(
                "custom://offline-style.json",
                new LatLngBounds(new LatLng(0, 0), new LatLng(1, 1)),
                0,
                1,
                1,
                true
            )
        );

    [Fact]
    public async Task OfflineRegionsAreCreatedListedAndDeletedThroughTasks()
    {
        using var runtime = RuntimeHandle.Create(
            RuntimeOptions.Default with
            {
                CachePath = ":memory:",
            }
        );
        var region = await runtime.OfflineRegionCreateAsync(
            Definition(),
            [1, 2, 3],
            TestContext.Current.CancellationToken
        );
        Assert.Equal<byte[]>([1, 2, 3], region.Metadata);
        Assert.Contains(
            await runtime.OfflineRegionsListAsync(TestContext.Current.CancellationToken),
            listed => listed.Id == region.Id
        );

        await runtime.OfflineRegionDeleteAsync(region.Id, TestContext.Current.CancellationToken);

        Assert.DoesNotContain(
            await runtime.OfflineRegionsListAsync(TestContext.Current.CancellationToken),
            listed => listed.Id == region.Id
        );
    }
}
