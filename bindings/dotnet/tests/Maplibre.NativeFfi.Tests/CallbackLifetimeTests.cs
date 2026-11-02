using System.Runtime.CompilerServices;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

#pragma warning disable xUnit1031 // The capturing helpers must not be async, or a state machine roots what they drop.

public sealed class CallbackLifetimeTests
{
    // A camera command's end handler sits two records deep in its input. Nothing but the binding
    // references it while the ease runs, so it must survive a collection to run when cancelled.
    [Fact]
    public async Task ACameraEndHandlerSurvivesACollectionUntilItRuns()
    {
        await using var fixture = await NativeFixture.WithEmptyStyleAsync();
        var ends = new System.Collections.Concurrent.ConcurrentQueue<CameraTransitionEnd>();
        await StartEase(fixture.Map, ends);
        Gc.Collect();

        // The handler runs before the cancellation completes.
        await fixture.Map.CancelCameraTransitionAsync(ulong.MaxValue);
        var end = Assert.Single(ends);
        Assert.Equal(CameraTransitionOutcome.Cancelled, end.Outcome);
        Assert.NotEqual(0UL, end.Generation);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static Task<CommandCompletion> StartEase(
        MapHandle map,
        System.Collections.Concurrent.ConcurrentQueue<CameraTransitionEnd> ends
    ) =>
        map.UpdateCameraAsync(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Ease,
                Camera = new CameraOptions { Zoom = 4 },
                Animation = new AnimationOptions
                {
                    DurationMs = 60000,
                    TransitionId = ulong.MaxValue,
                    EndHandler = new CameraTransitionHandler(ends.Enqueue),
                },
            }
        );

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
