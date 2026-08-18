using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class CustomMvtVectorSourceTests
{
    private static readonly byte[] EmptyStyleJson =
        """{"version":8,"sources":{},"layers":[]}"""u8.ToArray();

    [BindingSpecTest("BND-121", "BND-124")]
    [Fact]
    public void CustomMvtVectorCallbacksCopyTileIdsAndSwallowExceptions()
    {
        CanonicalTileId? fetched = null;
        CanonicalTileId? cancelled = null;
        using var state = new CustomMvtVectorSourceState(
            new CustomMvtVectorSourceOptions
            {
                FetchTile = tileId => fetched = tileId,
                CancelTile = tileId => cancelled = tileId,
            }
        );
        var tile = new CanonicalTileId(1, 2, 3);

        state.FetchForTest(tile);
        state.CancelForTest(tile);

        Assert.Equal(tile, fetched);
        Assert.Equal(tile, cancelled);

        using var throwing = new CustomMvtVectorSourceState(
            new CustomMvtVectorSourceOptions
            {
                FetchTile = _ => throw new InvalidOperationException("boom"),
            }
        );
        throwing.FetchForTest(tile);
    }

    [BindingSpecTest("BND-025")]
    [Fact]
    public void CustomMvtVectorSourceRequiresFetchTileCallback()
    {
        var error = Assert.Throws<ArgumentException>(() =>
            new CustomMvtVectorSourceState(new CustomMvtVectorSourceOptions())
        );
        Assert.Equal("options", error.ParamName);
    }

    [BindingSpecTest("BND-124")]
    [Fact]
    public async Task CustomMvtVectorDisposeKeepsHandleAliveUntilActiveCallbackExits()
    {
        var entered = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var release = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var state = new CustomMvtVectorSourceState(
            new CustomMvtVectorSourceOptions
            {
                FetchTile = _ =>
                {
                    entered.SetResult();
                    release.Task.GetAwaiter().GetResult();
                },
            }
        );

        var worker = Task.Run(
            () => state.FetchForTest(new CanonicalTileId(1, 2, 3)),
            TestContext.Current.CancellationToken
        );
        await entered.Task.WaitAsync(
            TimeSpan.FromSeconds(5),
            TestContext.Current.CancellationToken
        );

        state.Dispose();

        Assert.True(state.IsHandleAllocatedForTest);
        release.SetResult();
        await worker.WaitAsync(TimeSpan.FromSeconds(5), TestContext.Current.CancellationToken);
        Assert.False(state.IsHandleAllocatedForTest);
    }

    [BindingSpecTest("BND-122")]
    [Fact]
    public unsafe void CustomMvtVectorSourceInstallFailureReleasesOnlyTheRejectedState()
    {
        var failInstall = false;
        using var install = MapHandle.UseCustomMvtVectorSourceInstallForTest(
            (_, _, _) =>
                failInstall ? mln_status.MLN_STATUS_INVALID_STATE : mln_status.MLN_STATUS_OK
        );
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        var installed = new CustomMvtVectorSourceState(
            new CustomMvtVectorSourceOptions { FetchTile = _ => { } }
        );
        map.AddCustomMvtVectorSource("custom-mvt", installed);

        failInstall = true;
        var rejected = new CustomMvtVectorSourceState(
            new CustomMvtVectorSourceOptions { FetchTile = _ => { } }
        );
        Assert.Throws<InvalidStateException>(() =>
            map.AddCustomMvtVectorSource("custom-mvt", rejected)
        );

        Assert.False(rejected.IsHandleAllocatedForTest);
        Assert.True(installed.IsHandleAllocatedForTest);

        installed.Dispose();
    }

    [BindingSpecTest("BND-105", "BND-124")]
    [Fact]
    public void CustomMvtVectorSourceApisAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson(EmptyStyleJson);
        var tile = new CanonicalTileId(0, 0, 0);

        map.AddCustomMvtVectorSource(
            "custom-mvt",
            new CustomMvtVectorSourceOptions
            {
                FetchTile = _ => { },
                CancelTile = _ => { },
                MinimumZoom = 0,
                MaximumZoom = 10,
            }
        );
        map.SetCustomMvtVectorSourceTileData("custom-mvt", tile, []);
        map.SetCustomMvtVectorSourceTileError("custom-mvt", tile, "tile missing");
        map.InvalidateCustomMvtVectorSourceTile("custom-mvt", tile);

        Assert.Equal(SourceType.CustomMvtVector, map.StyleSourceType("custom-mvt"));
        Assert.True(map.RemoveStyleSource("custom-mvt"));
    }

    [BindingSpecTest("BND-124")]
    [Fact]
    public void RemovingACustomMvtVectorSourceReleasesItsCallbackState()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson(EmptyStyleJson);
        var state = new CustomMvtVectorSourceState(
            new CustomMvtVectorSourceOptions { FetchTile = _ => { } }
        );
        map.AddCustomMvtVectorSource("custom-mvt", state);
        Assert.True(state.IsHandleAllocatedForTest);

        Assert.True(map.RemoveStyleSource("custom-mvt"));

        Assert.False(state.IsHandleAllocatedForTest);
    }

    [BindingSpecTest("BND-124")]
    [Fact]
    public void ClosingAMapReleasesItsCustomMvtVectorSourceCallbackState()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson(EmptyStyleJson);
        var state = new CustomMvtVectorSourceState(
            new CustomMvtVectorSourceOptions { FetchTile = _ => { } }
        );
        map.AddCustomMvtVectorSource("custom-mvt", state);

        map.Close();

        Assert.False(state.IsHandleAllocatedForTest);
    }

    [BindingSpecTest("BND-093", "BND-124")]
    [Fact]
    public void AStyleReplacementReleasesADroppedSourceWithoutStyleLoadedEvents()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(
            runtime,
            new MapOptions
            {
                Width = 512,
                Height = 512,
                EventMask = RuntimeEventMask.All & ~RuntimeEventMask.MapStyleLoaded,
            }
        );
        map.SetStyleJson(EmptyStyleJson);
        var state = new CustomMvtVectorSourceState(
            new CustomMvtVectorSourceOptions { FetchTile = _ => { } }
        );
        map.AddCustomMvtVectorSource("custom-mvt", state);

        map.SetStyleJson(EmptyStyleJson);
        var drained = new List<RuntimeEventType>();
        for (var attempt = 0; attempt < 1000 && state.IsHandleAllocatedForTest; attempt++)
        {
            runtime.Pump(TimeSpan.FromMilliseconds(1));
            drained.AddRange(runtime.DrainEvents().Events.Select(polled => polled.Type));
        }

        Assert.False(state.IsHandleAllocatedForTest);
        Assert.DoesNotContain(RuntimeEventType.MapStyleLoaded, drained);
        Assert.Equal(RuntimeEventMask.All & ~RuntimeEventMask.MapStyleLoaded, map.GetEventMask());
    }
}
