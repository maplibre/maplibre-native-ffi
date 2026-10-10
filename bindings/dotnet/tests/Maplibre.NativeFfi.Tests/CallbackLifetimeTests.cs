using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Error;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

#pragma warning disable xUnit1031 // The capturing helpers must not be async, or a state machine roots what they drop.

public sealed class CallbackLifetimeTests
{
    [Fact]
    public async Task ACallbackIsRootedUntilNativeReleasesIt()
    {
        await using var fixture = await NativeFixture.WithEmptyStyleAsync();

        var (added, accepted) = AddCapturingSource(fixture.Map, new CustomGeometrySourceOptions());
        Assert.Equal(CommandDisposition.Committed, (await added).Disposition);
        Assert.True(Gc.IsAlive(accepted));

        // Removing the source is what makes native release the registration.
        await fixture.Map.RemoveStyleSourceAsync("custom", TestWaits.Token);
        Assert.Null(await fixture.Map.GetStyleSourceAsync("custom", TestWaits.Token));
        Assert.False(Gc.IsAlive(accepted));

        // Native validates the zoom range before it takes the registration.
        var (rejection, rejected) = AddRejectedCapturingSource(fixture.Map);
        Assert.IsType<InvalidArgumentException>(rejection);
        Assert.False(Gc.IsAlive(rejected));
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (Task<CommandCompletion> Added, WeakReference Captured) AddCapturingSource(
        MapHandle map,
        CustomGeometrySourceOptions options
    )
    {
        var captured = new object();
        var added = map.AddCustomGeometrySourceAsync(
            "custom",
            options with
            {
                FetchTile = _ => GC.KeepAlive(captured),
            },
            TestWaits.Token
        );
        return (added, new WeakReference(captured));
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (Exception? Rejection, WeakReference Captured) AddRejectedCapturingSource(
        MapHandle map
    )
    {
        var captured = new object();
        var rejection = Record.Exception(() =>
        {
            _ = map.AddCustomGeometrySourceAsync(
                "rejected",
                new CustomGeometrySourceOptions
                {
                    FetchTile = _ => GC.KeepAlive(captured),
                    MinZoom = 5,
                    MaxZoom = 1,
                },
                TestWaits.Token
            );
        });
        return (rejection, new WeakReference(captured));
    }

    // A camera command's end handler sits two records deep in its input. The binding roots it while
    // the ease runs, and native runs it once and releases it when its identity is cancelled.
    [Fact]
    public async Task ACameraEndHandlerIsRootedUntilItRuns()
    {
        await using var fixture = await NativeFixture.WithEmptyStyleAsync();
        var ends = new System.Collections.Concurrent.ConcurrentQueue<CameraTransitionEnd>();
        var (started, captured) = StartCapturingEase(fixture.Map, ends);
        Assert.Equal(CommandDisposition.Committed, (await started).Disposition);
        Assert.True(Gc.IsAlive(captured));

        // The handler runs before the cancellation completes.
        await fixture.Map.CancelCameraTransitionAsync(ulong.MaxValue, TestWaits.Token);
        var end = Assert.Single(ends);
        Assert.Equal(CameraTransitionOutcome.Cancelled, end.Outcome);
        Assert.NotEqual(0UL, end.Generation);
        Assert.False(Gc.IsAlive(captured));

        var (rejection, rejected) = ApplyRejectedCapturingDelta(fixture.Map);
        Assert.IsType<InvalidArgumentException>(rejection);
        Assert.False(Gc.IsAlive(rejected));
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (Task<CommandCompletion> Started, WeakReference Captured) StartCapturingEase(
        MapHandle map,
        System.Collections.Concurrent.ConcurrentQueue<CameraTransitionEnd> ends
    )
    {
        var captured = new object();
        var animation = AnimationOptions.Default;
        animation.DurationMs = 60000;
        animation.TransitionId = ulong.MaxValue;
        animation.EndHandler = new CameraTransitionHandler(end =>
        {
            GC.KeepAlive(captured);
            ends.Enqueue(end);
        });
        var started = map.UpdateCameraAsync(
            CameraUpdate.Default with
            {
                Mode = CameraUpdateMode.Ease,
                Camera = new CameraOptions { Zoom = 4 },
                Animation = animation,
            },
            TestWaits.Token
        );
        return (started, new WeakReference(captured));
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (Exception? Rejection, WeakReference Captured) ApplyRejectedCapturingDelta(
        MapHandle map
    )
    {
        var captured = new object();
        var animation = AnimationOptions.Default;
        animation.EndHandler = new CameraTransitionHandler(_ => GC.KeepAlive(captured));
        var delta = CameraDelta.Default;
        delta.Scale = -1;
        delta.Animation = animation;
        var rejection = Record.Exception(() =>
        {
            _ = map.ApplyCameraDeltaAsync(delta, TestWaits.Token);
        });
        return (rejection, new WeakReference(captured));
    }

    // A callback that captures the map it is registered on forms a cycle through the map's
    // callback roots, which the collector reclaims once nothing else holds the map.
    [Fact]
    public async Task ACallbackDoesNotKeepItsReceiverAlive()
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var map = RegisterCallbackCapturingItsMap(runtime);

        Gc.Collect();

        Assert.False(map.IsAlive);
        // The finalizer disposed the map, so the runtime has no live child to refuse its close.
        await runtime.CloseAsync();
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference RegisterCallbackCapturingItsMap(RuntimeHandle runtime)
    {
        var map = runtime.CreateMapAsync(NativeFixture.SmallMap).GetAwaiter().GetResult();
        map.SetStyleJsonAsync(NativeFixture.EmptyStyle).GetAwaiter().GetResult();
        var added = map.AddCustomGeometrySourceAsync(
                "captured-map",
                new CustomGeometrySourceOptions { FetchTile = _ => GC.KeepAlive(map) }
            )
            .GetAwaiter()
            .GetResult();
        Assert.Equal(CommandDisposition.Committed, added.Disposition);
        return new WeakReference(map);
    }
}

#pragma warning restore xUnit1031
