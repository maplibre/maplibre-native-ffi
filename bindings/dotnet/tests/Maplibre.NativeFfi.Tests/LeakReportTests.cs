using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

#pragma warning disable xUnit1031 // The abandoning helpers must not be async, or a state machine roots what they drop.

/// <summary>What the collector does with abandoned handles, read from standard error.</summary>
[Collection(nameof(GlobalState))]
public sealed class LeakReportTests
{
    // An abandoned handle retires through its native disposal on the finalizer thread. When that
    // disposal fails, or the handle has none, the finalizer reports the leak on standard error
    // and destroys nothing.
    [Fact]
    public async Task AnAbandonedHandleIsDisposedOrReportedAsALeak()
    {
        using var standardError = new StandardErrorCapture();
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var (map, undisposable) = Abandon(runtime);

        Gc.Collect();

        Assert.False(map.IsAlive);
        Assert.False(undisposable.IsAlive);
        // The finalizer disposed the map, so the runtime has no live child to refuse its close.
        await runtime.CloseAsync();
        Assert.Contains(
            $"Leaked RuntimeHandle native handle 0x{SyntheticHandles.Runtime(5678).Value:x}",
            standardError.Text,
            StringComparison.Ordinal
        );
        Assert.DoesNotContain("Leaked MapHandle", standardError.Text, StringComparison.Ordinal);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static unsafe (WeakReference Map, WeakReference Undisposable) Abandon(
        RuntimeHandle runtime
    )
    {
        var map = runtime.MapCreateAsync(NativeFixture.SmallMap).GetAwaiter().GetResult();
        // Every generated handle has a native disposal, so a state without one stands in for a
        // disposal that fails.
        var undisposable = new NativeHandleState<MlnRuntime>(
            SyntheticHandles.Runtime(5678),
            (_, _) => throw new InvalidOperationException("A leaked handle is never destroyed."),
            "RuntimeHandle"
        );
        return (new WeakReference(map), new WeakReference(undisposable));
    }

    [Fact]
    public void UnreachableHandlesAreReclaimedWithoutLeaks()
    {
        using var standardError = new StandardErrorCapture();
        var (runtime, map) = CreateUnreachableRuntimeAndMap();

        Gc.Collect();
        // The map's finalizer lets the runtime finish disposing, so a second pass collects it.
        Gc.Collect();

        Assert.False(runtime.IsAlive);
        Assert.False(map.IsAlive);
        Assert.DoesNotContain("Leaked", standardError.Text, StringComparison.Ordinal);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (WeakReference Runtime, WeakReference Map) CreateUnreachableRuntimeAndMap()
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var map = runtime.MapCreateAsync(NativeFixture.SmallMap).GetAwaiter().GetResult();
        runtime.BarrierAsync().GetAwaiter().GetResult();
        return (new WeakReference(runtime), new WeakReference(map));
    }
}

#pragma warning restore xUnit1031
