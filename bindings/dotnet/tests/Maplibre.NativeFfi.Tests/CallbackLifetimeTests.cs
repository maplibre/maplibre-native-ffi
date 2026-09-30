using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
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
        Assert.Null(await fixture.Map.GetStyleSourceInfoAsync("custom", TestWaits.Token));
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
        var map = runtime.MapCreateAsync(NativeFixture.SmallMap).GetAwaiter().GetResult();
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
