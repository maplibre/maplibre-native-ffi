using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Offline;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RuntimeOfflineOperationTests
{
    [Fact]
    public async Task AmbientCacheOperationsAreAwaitable()
    {
        using var runtime = TestHandles.CreateRuntime(
            new RuntimeOptions { CachePath = ":memory:" }
        );

        await runtime.RunAmbientCacheOperationAsync(AmbientCacheOperation.Invalidate);
        await runtime.SetMaximumAmbientCacheSizeAsync(8UL << 20);
    }

    [BindingSpecTest("BND-084", "BND-085")]
    [Fact]
    public async Task OfflineRegionsAreCreatedListedAndDeletedThroughTasks()
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

        var region = await runtime.CreateOfflineRegionAsync(definition, [1, 2, 3]);
        Assert.Equal<byte[]>([1, 2, 3], region.Metadata);
        Assert.Contains(await runtime.ListOfflineRegionsAsync(), listed => listed.Id == region.Id);

        await runtime.DeleteOfflineRegionAsync(region.Id);

        Assert.DoesNotContain(
            await runtime.ListOfflineRegionsAsync(),
            listed => listed.Id == region.Id
        );
    }
}
