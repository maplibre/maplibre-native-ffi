using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Error;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

#pragma warning disable xUnit1031 // The abandoning helpers must not be async, or a state machine roots what they drop.

public sealed class HandleLifecycleTests
{
    [Fact]
    public async Task ClosingAHandleTwiceClosesItOnce()
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var map = await runtime.MapCreateAsync(NativeFixture.SmallMap);

        map.Close();
        map.Close();
        await map.DisposeAsync();
        Assert.True(map.IsClosed);

        var teardown = runtime.CloseAsync();
        var again = runtime.CloseAsync();
        Assert.True(runtime.IsClosed);
        await teardown;
        await again;
        runtime.Dispose();
    }

    [Fact]
    public async Task ARefusedCloseLeavesTheHandleUsableAndARetrySucceeds()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = await runtime.MapCreateAsync(NativeFixture.SmallMap);

        var error = Assert.Throws<InvalidStateException>(runtime.Close);
        Assert.Equal((int)MaplibreStatus.InvalidState, error.RawStatus);
        Assert.Contains("live or pending children", error.Diagnostic, StringComparison.Ordinal);
        Assert.False(runtime.IsClosed);
        await runtime.BarrierAsync(TestWaits.Token);

        await map.CloseAsync();
        await runtime.CloseAsync();
        Assert.True(runtime.IsClosed);
    }

    [Fact]
    public async Task AClosedHandleRejectsCallsBeforeTheyReachNative()
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var map = await runtime.MapCreateAsync(NativeFixture.SmallMap);
        await map.CloseAsync();
        await runtime.CloseAsync();

        var error = Assert.Throws<InvalidStateException>(() =>
        {
            _ = map.RequestRepaintAsync(TestWaits.Token);
        });

        Assert.Null(error.RawStatus);
        Assert.Equal("MapHandle is closed", error.Diagnostic);
    }

    [Fact]
    public async Task AChildKeepsItsParentAlive()
    {
        var (map, runtime) = CreateMapOfUnreferencedRuntime();

        Gc.Collect();

        Assert.True(runtime.IsAlive);
        var completion = await map.SetStyleJsonAsync(NativeFixture.EmptyStyle, TestWaits.Token);
        Assert.Equal(CommandDisposition.Committed, completion.Disposition);
        await map.CloseAsync();
        await ((RuntimeHandle)runtime.Target!).CloseAsync();
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (MapHandle Map, WeakReference Runtime) CreateMapOfUnreferencedRuntime()
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var map = runtime.MapCreateAsync(NativeFixture.SmallMap).GetAwaiter().GetResult();
        return (map, new WeakReference(runtime));
    }

    [Fact]
    public async Task ADroppedCreationTaskRetiresTheMapItCreates()
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var creation = DropCreation(runtime);
        // The creation completes before the barrier, so the dropped task holds the map.
        await runtime.BarrierAsync(TestWaits.Token);

        Gc.Collect();

        Assert.False(creation.IsAlive);
        // A live map would make the runtime refuse to close.
        await runtime.CloseAsync();
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference DropCreation(RuntimeHandle runtime) =>
        new(runtime.MapCreateAsync(NativeFixture.SmallMap));
}

#pragma warning restore xUnit1031
