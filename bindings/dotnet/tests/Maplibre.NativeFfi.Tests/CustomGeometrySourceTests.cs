using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class CustomGeometrySourceTests
{
    [BindingSpecTest("BND-121", "BND-124")]
    [Fact]
    public void CustomGeometryCallbacksCopyTileIdsAndSwallowExceptions()
    {
        CanonicalTileId? fetched = null;
        CanonicalTileId? cancelled = null;
        using var state = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions
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

        using var throwing = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions
            {
                FetchTile = _ => throw new InvalidOperationException("boom"),
            }
        );
        throwing.FetchForTest(tile);
    }

    [BindingSpecTest("BND-025")]
    [Fact]
    public void CustomGeometrySourceRequiresFetchTileCallback()
    {
        var error = Assert.Throws<ArgumentException>(() =>
            new CustomGeometrySourceState(new CustomGeometrySourceOptions())
        );
        Assert.Equal("options", error.ParamName);
    }

    [BindingSpecTest("BND-025")]
    [Fact]
    public void CustomGeometrySourceRejectsNegativeBuffer()
    {
        using var state = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions { FetchTile = _ => { }, Buffer = -1 }
        );

        var error = Assert.Throws<InvalidArgumentException>(() => state.Descriptor);

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
    }

    [Fact]
    public void CustomGeometrySourceRejectsFractionalNegativeBuffer()
    {
        using var state = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions { FetchTile = _ => { }, Buffer = -0.5 }
        );

        // Cast-safety: -0.5 truncates to 0u and would silently pass the C-side
        // unsigned-domain check. The binding rejects before the cast.
        Assert.Throws<InvalidArgumentException>(() => state.Descriptor);
    }

    [BindingSpecTest("BND-124")]
    [Fact]
    public async Task CustomGeometryDisposeKeepsHandleAliveUntilActiveCallbackExits()
    {
        var entered = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var release = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var state = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions
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
    public unsafe void CustomGeometrySourceInstallFailureReleasesOnlyTheRejectedState()
    {
        var failInstall = false;
        using var install = MapHandle.UseCustomGeometrySourceInstallForTest(
            (_, _, _, _) =>
                failInstall ? mln_status.MLN_STATUS_INVALID_STATE : mln_status.MLN_STATUS_OK
        );
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        var installed = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions { FetchTile = _ => { } }
        );
        map.AddCustomGeometrySourceAsync(
            "custom",
            installed,
            TestContext.Current.CancellationToken
        );

        failInstall = true;
        var rejected = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions { FetchTile = _ => { } }
        );
        Assert.Throws<InvalidStateException>(() =>
            map.AddCustomGeometrySourceAsync(
                    "custom",
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
    public async Task CustomGeometrySourceApisAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var tile = new CanonicalTileId(0, 0, 0);

        _ = map.AddCustomGeometrySourceAsync(
            "custom",
            new CustomGeometrySourceOptions
            {
                FetchTile = _ => { },
                CancelTile = _ => { },
                TileSize = 512,
                MinimumZoom = 0,
                MaximumZoom = 10,
                Tolerance = 0.375,
                Buffer = 128,
                Clip = true,
                Wrap = false,
            },
            TestContext.Current.CancellationToken
        );
        _ = map.SetCustomGeometrySourceTileDataAsync(
            "custom",
            tile,
            """{"type":"FeatureCollection","features":[]}"""u8.ToArray(),
            TestContext.Current.CancellationToken
        );
        _ = map.InvalidateCustomGeometrySourceTileAsync(
            "custom",
            tile,
            TestContext.Current.CancellationToken
        );
        _ = map.InvalidateCustomGeometrySourceRegionAsync(
            "custom",
            new LatLngBounds(new LatLng(-1, -1), new LatLng(1, 1)),
            TestContext.Current.CancellationToken
        );

        Assert.Equal(
            SourceType.CustomVector,
            (await map.StyleSourceInfoAsync("custom", TestContext.Current.CancellationToken))?.Type
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleSourceAsync("custom", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.StyleSourceInfoAsync("custom", TestContext.Current.CancellationToken)
        );
    }

    [BindingSpecTest("BND-124")]
    [Fact]
    public async Task RemovingACustomGeometrySourceReleasesItsCallbackState()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var state = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions { FetchTile = _ => { } }
        );
        _ = map.AddCustomGeometrySourceAsync(
            "custom",
            state,
            TestContext.Current.CancellationToken
        );
        Assert.True(state.IsHandleAllocatedForTest);

        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleSourceAsync("custom", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.StyleSourceInfoAsync("custom", TestContext.Current.CancellationToken)
        );

        Assert.False(state.IsHandleAllocatedForTest);
    }

    [BindingSpecTest("BND-124")]
    [Fact]
    public async Task ClosingAMapReleasesItsCustomGeometrySourceCallbackState()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 512, Height = 512 });
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var state = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions { FetchTile = _ => { } }
        );
        _ = map.AddCustomGeometrySourceAsync(
            "custom",
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
        var state = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions { FetchTile = _ => { } }
        );
        map.AddCustomGeometrySourceAsync("custom", state, TestContext.Current.CancellationToken);

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
