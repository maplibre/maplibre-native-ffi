using System.Runtime.CompilerServices;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

#pragma warning disable xUnit1031 // The capturing helpers must not be async, or a state machine roots what they drop.

public sealed class CallbackLifetimeTests
{
    [Fact]
    public async Task ACallbackIsRootedUntilNativeReleasesIt()
    {
        await using var fixture = await NativeFixture.WithEmptyStyleAsync();

        var (added, accepted) = AddCapturingSource(fixture.Map);
        await added;
        Assert.True(Gc.IsAlive(accepted));

        // Removing the source is what makes native release the registration.
        await fixture.Map.RemoveStyleSourceAsync("custom");
        Assert.Null(await fixture.Map.GetStyleSourceAsync("custom"));
        Assert.False(Gc.IsAlive(accepted));

        // Native validates the zoom range before it takes the registration.
        var (rejection, rejected) = AddRejectedCapturingSource(fixture.Map);
        Assert.IsType<MaplibreException>(rejection);
        Assert.False(Gc.IsAlive(rejected));
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (Task<CommandCompletion> Added, WeakReference Captured) AddCapturingSource(
        MapHandle map
    )
    {
        var captured = new object();
        var added = map.AddCustomGeometrySourceAsync(
            "custom",
            new CustomGeometrySourceOptions { FetchTile = _ => GC.KeepAlive(captured) }
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
                }
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
        await started;
        Assert.True(Gc.IsAlive(captured));

        // The handler runs before the cancellation completes.
        await fixture.Map.CancelCameraTransitionAsync(ulong.MaxValue);
        var end = Assert.Single(ends);
        Assert.Equal(CameraTransitionOutcome.Cancelled, end.Outcome);
        Assert.NotEqual(0UL, end.Generation);
        Assert.False(Gc.IsAlive(captured));

        var (rejection, rejected) = ApplyRejectedCapturingDelta(fixture.Map);
        Assert.IsType<MaplibreException>(rejection);
        Assert.False(Gc.IsAlive(rejected));
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (Task<CommandCompletion> Started, WeakReference Captured) StartCapturingEase(
        MapHandle map,
        System.Collections.Concurrent.ConcurrentQueue<CameraTransitionEnd> ends
    )
    {
        var captured = new object();
        var animation = new AnimationOptions
        {
            DurationMs = 60000,
            TransitionId = ulong.MaxValue,
            EndHandler = new CameraTransitionHandler(end =>
            {
                GC.KeepAlive(captured);
                ends.Enqueue(end);
            }),
        };
        var started = map.UpdateCameraAsync(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Ease,
                Camera = new CameraOptions { Zoom = 4 },
                Animation = animation,
            }
        );
        return (started, new WeakReference(captured));
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (Exception? Rejection, WeakReference Captured) ApplyRejectedCapturingDelta(
        MapHandle map
    )
    {
        var captured = new object();
        var delta = new CameraDelta
        {
            Scale = -1,
            Animation = new AnimationOptions
            {
                EndHandler = new CameraTransitionHandler(_ => GC.KeepAlive(captured)),
            },
        };
        var rejection = Record.Exception(() =>
        {
            _ = map.ApplyCameraDeltaAsync(delta);
        });
        return (rejection, new WeakReference(captured));
    }

    // Native holds a strong root until the registration is released.
    [Fact]
    public async Task ACallbackKeepsItsReceiverAliveUntilExplicitClose()
    {
        var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = RegisterCallbackCapturingItsMap(runtime);

        Gc.Collect();

        Assert.True(map.IsAlive);
        await ((MapHandle)map.Target!).CloseAsync();
        // The finalizer disposed the map, so the runtime has no live child to refuse its close.
        await runtime.CloseAsync();
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference RegisterCallbackCapturingItsMap(RuntimeHandle runtime)
    {
        var map = runtime.CreateMap(NativeFixture.SmallMap);
        map.Ready.GetAwaiter().GetResult();
        map.SetStyleJsonAsync(NativeFixture.EmptyStyle).GetAwaiter().GetResult();
        map.AddCustomGeometrySourceAsync(
                "captured-map",
                new CustomGeometrySourceOptions { FetchTile = _ => GC.KeepAlive(map) }
            )
            .GetAwaiter()
            .GetResult();
        return new WeakReference(map);
    }
}

#pragma warning restore xUnit1031
