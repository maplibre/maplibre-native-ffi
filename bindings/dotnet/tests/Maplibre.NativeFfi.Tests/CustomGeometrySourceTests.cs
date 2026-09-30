using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class CustomGeometrySourceTests
{
    [Fact]
    public async Task CustomGeometrySourceApisAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
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
                MinZoom = 0,
                MaxZoom = 10,
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
            StyleSourceType.CustomVector,
            (await map.GetStyleSourceInfoAsync("custom", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleSourceAsync("custom", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.GetStyleSourceInfoAsync("custom", TestContext.Current.CancellationToken)
        );
    }

    [Fact]
    public async Task RemovingACustomGeometrySourceReleasesItsCallbackState()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var state = InstallCallbackProbe(map);
        Assert.True(Alive(state));

        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleSourceAsync("custom", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.GetStyleSourceInfoAsync("custom", TestContext.Current.CancellationToken)
        );

        Assert.False(Alive(state));
    }

    [Fact]
    public async Task ClosingAMapReleasesItsCustomGeometrySourceCallbackState()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var state = InstallCallbackProbe(map);

        map.Close();
        // The runtime runs the release callback while retiring the map, so a barrier that
        // observes the retirement observes the release too.
        await runtime.BarrierAsync(TestContext.Current.CancellationToken);

        Assert.False(Alive(state));
    }

    [Fact]
    public async Task AStyleReplacementReleasesADroppedSourceWithoutStyleLoadedEvents()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },

                EventMask = RuntimeEventMask.All & ~RuntimeEventMask.MapStyleLoaded,
            }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var state = InstallCallbackProbe(map);

        // The replacement style drops the source, and the C API reports that through the release
        // callback rather than through an event, so the host's cleared mask stays cleared.
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken)
        );
        await runtime.BarrierAsync(TestContext.Current.CancellationToken);
        var drained = runtime.DrainEventCopies().Select(polled => polled.Type).ToList();

        Assert.False(Alive(state));
        Assert.DoesNotContain(RuntimeEventType.MapStyleLoaded, drained);
        Assert.Equal(
            RuntimeEventMask.All & ~RuntimeEventMask.MapStyleLoaded,
            map.SnapshotGet().EventMask
        );
    }

    [Fact]
    public void CallbackCapturingMapAllowsAbandonedMapToRetire()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var owner = CreateCapturedMap(runtime);
        GC.Collect();
        GC.WaitForPendingFinalizers();
        // Native graph disposal may outlive a runtime barrier. A retained callback
        // can still reference the wrapper after finalization admitted its retirement.
        if (owner.Target is MapHandle map)
            Assert.True(map.IsClosed);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference CreateCapturedMap(RuntimeHandle runtime)
    {
        var map = TestHandles.CreateMap(runtime, MapOptions.Default);
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken)
        );
        Action<CanonicalTileId> callback = _ => GC.KeepAlive(map);
        RuntimeEventTestHelpers.AssertCommitted(
            map.AddCustomGeometrySourceAsync(
                "captured-map",
                new CustomGeometrySourceOptions { FetchTile = callback },
                TestContext.Current.CancellationToken
            )
        );
        return new WeakReference(map, trackResurrection: true);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference InstallCallbackProbe(MapHandle map)
    {
        var target = new CallbackProbe();
        RuntimeEventTestHelpers.AssertCommitted(
            map.AddCustomGeometrySourceAsync(
                "custom",
                new CustomGeometrySourceOptions { FetchTile = target.Fetch },
                TestContext.Current.CancellationToken
            )
        );
        return new WeakReference(target);
    }

    private sealed class CallbackProbe
    {
        public void Fetch(CanonicalTileId _) { }
    }

    private static bool Alive(WeakReference value)
    {
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        return value.IsAlive;
    }
}
