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
            (_, _, _, _) =>
                failInstall ? mln_status.MLN_STATUS_INVALID_STATE : mln_status.MLN_STATUS_OK
        );
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        var installed = new CustomMvtVectorSourceState(
            new CustomMvtVectorSourceOptions { FetchTile = _ => { } }
        );
        map.AddCustomMvtVectorSourceAsync(
            "custom-mvt",
            installed,
            TestContext.Current.CancellationToken
        );

        failInstall = true;
        var rejected = new CustomMvtVectorSourceState(
            new CustomMvtVectorSourceOptions { FetchTile = _ => { } }
        );
        Assert.Throws<InvalidStateException>(() =>
            map.AddCustomMvtVectorSourceAsync(
                    "custom-mvt",
                    rejected,
                    TestContext.Current.CancellationToken
                )
                .GetAwaiter()
                .GetResult()
        );

        // A rejected add owes no release callback, so the binding frees that state itself and
        // leaves the state the accepted add handed over untouched.
        Assert.False(rejected.IsHandleAllocatedForTest);
        Assert.True(installed.IsHandleAllocatedForTest);

        // The faked install never handed the state to MapLibre, so no release is owed for it.
        installed.Dispose();
    }

    [BindingSpecTest("BND-105", "BND-124")]
    [Fact]
    public async Task CustomMvtVectorSourceApisAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var tile = new CanonicalTileId(0, 0, 0);

        _ = map.AddCustomMvtVectorSourceAsync(
            "custom-mvt",
            new CustomMvtVectorSourceOptions
            {
                FetchTile = _ => { },
                CancelTile = _ => { },
                MinimumZoom = 0,
                MaximumZoom = 10,
            },
            TestContext.Current.CancellationToken
        );
        _ = map.SetCustomMvtVectorSourceTileDataAsync(
            "custom-mvt",
            tile,
            [],
            TestContext.Current.CancellationToken
        );
        _ = map.SetCustomMvtVectorSourceTileErrorAsync(
            "custom-mvt",
            tile,
            "tile missing",
            TestContext.Current.CancellationToken
        );
        _ = map.InvalidateCustomMvtVectorSourceTileAsync(
            "custom-mvt",
            tile,
            TestContext.Current.CancellationToken
        );

        Assert.Equal(
            SourceType.CustomMvtVector,
            (
                await map.StyleSourceInfoAsync("custom-mvt", TestContext.Current.CancellationToken)
            )?.Type
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleSourceAsync("custom-mvt", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.StyleSourceInfoAsync("custom-mvt", TestContext.Current.CancellationToken)
        );
    }

    [BindingSpecTest("BND-124")]
    [Fact]
    public async Task RemovingACustomMvtVectorSourceReleasesItsCallbackState()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var state = new CustomMvtVectorSourceState(
            new CustomMvtVectorSourceOptions { FetchTile = _ => { } }
        );
        _ = map.AddCustomMvtVectorSourceAsync(
            "custom-mvt",
            state,
            TestContext.Current.CancellationToken
        );
        Assert.True(state.IsHandleAllocatedForTest);

        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleSourceAsync("custom-mvt", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.StyleSourceInfoAsync("custom-mvt", TestContext.Current.CancellationToken)
        );

        Assert.False(state.IsHandleAllocatedForTest);
    }

    [BindingSpecTest("BND-124")]
    [Fact]
    public async Task ClosingAMapReleasesItsCustomMvtVectorSourceCallbackState()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 512, Height = 512 });
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var state = new CustomMvtVectorSourceState(
            new CustomMvtVectorSourceOptions { FetchTile = _ => { } }
        );
        _ = map.AddCustomMvtVectorSourceAsync(
            "custom-mvt",
            state,
            TestContext.Current.CancellationToken
        );

        map.Close();
        // The runtime runs the release callback while retiring the map, so a barrier that
        // observes the retirement observes the release too.
        await runtime.BarrierAsync(TestContext.Current.CancellationToken);

        Assert.False(state.IsHandleAllocatedForTest);
    }

    [BindingSpecTest("BND-093", "BND-124")]
    [Fact]
    public void AStyleReplacementReleasesADroppedSourceWithoutStyleLoadedEvents()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions
            {
                Width = 512,
                Height = 512,
                EventMask = RuntimeEventMask.All & ~RuntimeEventMask.MapStyleLoaded,
            }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var state = new CustomMvtVectorSourceState(
            new CustomMvtVectorSourceOptions { FetchTile = _ => { } }
        );
        _ = map.AddCustomMvtVectorSourceAsync(
            "custom-mvt",
            state,
            TestContext.Current.CancellationToken
        );

        // The replacement style drops the source, and the C API reports that through the release
        // callback rather than through an event, so the host's cleared mask stays cleared.
        map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var drained = new List<RuntimeEventType>();
        for (var attempt = 0; attempt < 1000 && state.IsHandleAllocatedForTest; attempt++)
        {
            Thread.Sleep(1);
            drained.AddRange(runtime.DrainEvents().Select(polled => polled.Type));
        }

        Assert.False(state.IsHandleAllocatedForTest);
        Assert.DoesNotContain(RuntimeEventType.MapStyleLoaded, drained);
        Assert.Equal(
            RuntimeEventMask.All & ~RuntimeEventMask.MapStyleLoaded,
            map.GetSnapshot().EventMask
        );
    }
}
