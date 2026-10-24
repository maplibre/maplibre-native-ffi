using System.Collections.Concurrent;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Pointer;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

#pragma warning disable xUnit1031 // The abandoning helpers must not be async, or a state machine roots what they drop.

/// <summary>What the collector does with abandoned handles, and where it reports them.</summary>
[Collection(nameof(GlobalState))]
public sealed class LeakReportTests
{
    // An abandoned handle retires through its native disposal on the finalizer thread, and the
    // finalizer reports the leak once, whether or not that disposal succeeds.
    // A handle whose disposal fails is reported and destroyed by nothing.
    [Fact]
    public void AnAbandonedHandleIsDisposedAndReportedAsALeak()
    {
        using var reports = new LeakCapture();
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var (map, mapId, undisposable) = Abandon(runtime);

        Gc.Collect();

        Assert.False(map.IsAlive);
        Assert.False(undisposable.IsAlive);
        var report = $"Leaked MapHandle native handle 0x{mapId:x}; it was disposed when collected.";
        Assert.Contains(report, reports.Text, StringComparison.Ordinal);
        Assert.Equal(1, Occurrences(reports.Text, report));
        Assert.Equal(
            1,
            Occurrences(
                reports.Text,
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
        using var reports = new LeakCapture();
        var (runtime, runtimeId, map, mapId) = CreateUnreachableRuntimeAndMap();

        Gc.Collect();
        // The map's finalizer lets the runtime finish disposing, so a second pass collects it.
        Gc.Collect();

        Assert.False(runtime.IsAlive);
        Assert.False(map.IsAlive);
        Assert.Equal(
            1,
            Occurrences(reports.Text, $"Leaked RuntimeHandle native handle 0x{runtimeId:x};")
        );
        Assert.Equal(1, Occurrences(reports.Text, $"Leaked MapHandle native handle 0x{mapId:x};"));
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

    // The creation's completion roots the map until native releases it on the runtime worker.
    // An ordered query on the map runs after that release.
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static MapHandle CreateMapReleased(RuntimeHandle runtime)
    {
        var map = runtime.CreateMap(NativeFixture.SmallMap);
        map.Ready.GetAwaiter().GetResult();
        map.GetStyleUrlAsync().GetAwaiter().GetResult();
        return map;
    }

    // A host that added no trace listener has only the default one, which writes to an attached
    // debugger alone, so the report goes to standard error instead.
    [Fact]
    public void WithoutAHostTraceListenerTheReportGoesToStandardError()
    {
        using var standardError = new StandardErrorCapture();
        var installed = Trace.Listeners.Cast<TraceListener>().ToArray();
        Trace.Listeners.Clear();
        Trace.Listeners.Add(new DefaultTraceListener());
        try
        {
            var leaked = AbandonDisposable();
            Gc.Collect();
            Assert.False(leaked.IsAlive);
        }
        finally
        {
            Trace.Listeners.Clear();
            Trace.Listeners.AddRange(installed);
        }

        Assert.Contains(
            $"Maplibre.NativeFfi LeakedHandle: Leaked RuntimeHandle native handle 0x{SyntheticHandles.Runtime(9012).Value:x}; it was disposed when collected.",
            standardError.Text,
            StringComparison.Ordinal
        );
    }

    /// <summary>Records the leak warnings that reach the trace listeners until disposed.</summary>
    private sealed class LeakCapture : TraceListener
    {
        private readonly ConcurrentQueue<string> warnings = new();

        internal LeakCapture() => Trace.Listeners.Add(this);

        internal string Text => string.Join('\n', warnings);

        public override void TraceEvent(
            TraceEventCache? eventCache,
            string source,
            TraceEventType eventType,
            int id,
            string? message
        )
        {
            if (eventType == TraceEventType.Warning && message is not null)
                warnings.Enqueue(message);
        }

        public override void Write(string? message) { }

        public override void WriteLine(string? message) { }

        protected override void Dispose(bool disposing)
        {
            Trace.Listeners.Remove(this);
            base.Dispose(disposing);
        }
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static unsafe WeakReference AbandonDisposable() =>
        new(
            new NativeHandleState<MlnRuntime>(
                SyntheticHandles.Runtime(9012),
                (_, _) => mln_status.MLN_STATUS_OK,
                "RuntimeHandle",
                (_, _) => mln_status.MLN_STATUS_OK
            )
        );

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
