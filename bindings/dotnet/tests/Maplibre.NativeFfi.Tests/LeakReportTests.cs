using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Pointer;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

#pragma warning disable xUnit1031 // The abandoning helpers must not be async, or a state machine roots what they drop.

/// <summary>What the collector does with abandoned handles, read from standard error.</summary>
[Collection(nameof(GlobalState))]
public sealed class LeakReportTests
{
    // An abandoned handle retires through its native disposal on the finalizer thread, and the
    // finalizer reports the leak on standard error once, whether or not that disposal succeeds.
    // A handle whose disposal fails is reported and destroyed by nothing.
    [Fact]
    public void AnAbandonedHandleIsDisposedAndReportedAsALeak()
    {
        using var standardError = new StandardErrorCapture();
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var (map, mapId, undisposable) = Abandon(runtime);

        Gc.Collect();

        Assert.False(map.IsAlive);
        Assert.False(undisposable.IsAlive);
        var report = $"Leaked MapHandle native handle 0x{mapId:x}; it was disposed when collected.";
        Assert.Contains(report, standardError.Text, StringComparison.Ordinal);
        Assert.Equal(1, Occurrences(standardError.Text, report));
        Assert.Equal(
            1,
            Occurrences(
                standardError.Text,
                $"Leaked RuntimeHandle native handle 0x{SyntheticHandles.Runtime(5678).Value:x}; native disposal failed"
            )
        );
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static unsafe (WeakReference Map, ulong MapId, WeakReference Undisposable) Abandon(
        RuntimeHandle runtime
    )
    {
        var map = CreateMapReleased(runtime);
        // A state whose disposal throws stands in for a native disposal that fails.
        var undisposable = new NativeHandleState<MlnRuntime>(
            SyntheticHandles.Runtime(5678),
            (_, _) => throw new InvalidOperationException("A leaked handle is never destroyed."),
            "RuntimeHandle",
            (_, _) => throw new InvalidOperationException("The disposal fails.")
        );
        return (new WeakReference(map), map.Id, new WeakReference(undisposable));
    }

    // A runtime and its map that become unreachable together are both reclaimed, each reported
    // once: the map's finalizer lets the runtime finish disposing.
    [Fact]
    public void UnreachableHandlesAreReclaimedAndReported()
    {
        using var standardError = new StandardErrorCapture();
        var (runtime, runtimeId, map, mapId) = CreateUnreachableRuntimeAndMap();

        Gc.Collect();
        // The map's finalizer lets the runtime finish disposing, so a second pass collects it.
        Gc.Collect();

        Assert.False(runtime.IsAlive);
        Assert.False(map.IsAlive);
        Assert.Equal(
            1,
            Occurrences(standardError.Text, $"Leaked RuntimeHandle native handle 0x{runtimeId:x};")
        );
        Assert.Equal(
            1,
            Occurrences(standardError.Text, $"Leaked MapHandle native handle 0x{mapId:x};")
        );
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (
        WeakReference Runtime,
        ulong RuntimeId,
        WeakReference Map,
        ulong MapId
    ) CreateUnreachableRuntimeAndMap()
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var map = CreateMapReleased(runtime);
        return (new WeakReference(runtime), runtime.Id, new WeakReference(map), map.Id);
    }

    // The creation's completion roots its result until native releases it on the runtime
    // worker. An ordered query on the map runs after that release.
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static MapHandle CreateMapReleased(RuntimeHandle runtime)
    {
        var map = runtime.CreateMapAsync(NativeFixture.SmallMap).GetAwaiter().GetResult();
        map.GetStyleUrlAsync(TestWaits.Token).GetAwaiter().GetResult();
        return map;
    }

    private static int Occurrences(string text, string value)
    {
        var count = 0;
        for (
            var index = text.IndexOf(value, StringComparison.Ordinal);
            index >= 0;
            index = text.IndexOf(value, index + value.Length, StringComparison.Ordinal)
        )
            count++;
        return count;
    }
}

#pragma warning restore xUnit1031
