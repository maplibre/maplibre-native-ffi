using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class CustomMvtVectorSourceTests
{
    [BindingSpecTest("", "")]
    [Fact]
    public async Task CustomMvtVectorSourceApisAdaptThroughNativeMap()
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

        _ = map.AddCustomMvtVectorSourceAsync(
            "custom-mvt",
            new CustomMvtVectorSourceOptions
            {
                FetchTile = _ => { },
                CancelTile = _ => { },
                MinZoom = 0,
                MaxZoom = 10,
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
            StyleSourceType.CustomMvtVector,
            (await map.GetStyleSourceInfoAsync("custom-mvt", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleSourceAsync("custom-mvt", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.GetStyleSourceInfoAsync("custom-mvt", TestContext.Current.CancellationToken)
        );
    }

    [BindingSpecTest("")]
    [Fact]
    public async Task RemovingACustomMvtVectorSourceReleasesItsCallbackState()
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
            map.RemoveStyleSourceAsync("custom-mvt", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.GetStyleSourceInfoAsync("custom-mvt", TestContext.Current.CancellationToken)
        );

        Assert.False(Alive(state));
    }

    [BindingSpecTest("")]
    [Fact]
    public async Task ClosingAMapReleasesItsCustomMvtVectorSourceCallbackState()
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

    [BindingSpecTest("", "")]
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

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference InstallCallbackProbe(MapHandle map)
    {
        var target = new CallbackProbe();
        RuntimeEventTestHelpers.AssertCommitted(
            map.AddCustomMvtVectorSourceAsync(
                "custom-mvt",
                new CustomMvtVectorSourceOptions { FetchTile = target.Fetch },
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
