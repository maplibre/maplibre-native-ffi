using System.Diagnostics;
using Maplibre.Native.Map;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class RuntimeWakeTests
{
    private static readonly TimeSpan ParkTimeout = TimeSpan.FromSeconds(10);

    // Well below ParkTimeout, and far above the scheduling noise a loaded CI machine
    // adds to a condition-variable wake.
    private static readonly TimeSpan PromptReturn = TimeSpan.FromSeconds(5);

    // Pumps until the runtime is idle, so a park that follows is released by the
    // signal the test raises.
    private static void Quiesce(RuntimeHandle runtime)
    {
        for (var attempt = 0; attempt < 100; attempt++)
        {
            runtime.Pump(TimeSpan.Zero);
            var drained = false;
            while (runtime.PollEvent() is not null)
            {
                drained = true;
            }

            if (!drained)
            {
                return;
            }
        }

        Assert.Fail("The runtime kept producing events while idle.");
    }

    [BindingSpecTest("BND-088")]
    [Fact]
    public void ParkedOwnerThreadWakesForNativeWorkAndForAWakeSource()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        Quiesce(runtime);

        // The style is malformed, so native reports the failure from its own
        // threads and the failure reaches the parked owner thread.
        map.SetStyleUrl("unsupported://style.json");
        var loadingFailed = false;
        var loadStarted = Stopwatch.StartNew();
        for (var attempt = 0; attempt < 20 && !loadingFailed; attempt++)
        {
            runtime.Pump(ParkTimeout);
            Assert.True(
                loadStarted.Elapsed < PromptReturn,
                "Parks sat out their timeouts while loading was pending."
            );
            while (runtime.PollEvent() is { } polled)
            {
                if (polled.Type == RuntimeEventType.MapLoadingFailed)
                {
                    loadingFailed = true;
                }
            }
        }

        Assert.True(loadingFailed);

        // A source signalled from another thread matches a host's submission
        // path, and the park it releases has no other work to end it.
        var source = runtime.AcquireWakeSource();
        Quiesce(runtime);
        var signaller = new Thread(() =>
        {
            Thread.Sleep(TimeSpan.FromMilliseconds(20));
            source.Signal();
        });
        signaller.Start();

        var parkStarted = Stopwatch.StartNew();
        runtime.Pump(ParkTimeout);
        Assert.True(
            parkStarted.Elapsed < PromptReturn,
            "The parked owner thread timed out instead of taking the signal."
        );
        signaller.Join();

        // A wake source stays usable after its runtime closes, so hosts tear
        // the two down in either order.
        map.Close();
        runtime.Close();
        source.Signal();
        source.Close();
        Assert.True(source.IsClosed);
    }

    [BindingSpecTest("BND-089")]
    [Fact]
    public void PumpClearsTheWakeFlagItReturnsOn()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var source = runtime.AcquireWakeSource();
        Quiesce(runtime);

        source.Signal();
        var signalledStarted = Stopwatch.StartNew();
        runtime.Pump(ParkTimeout);
        Assert.True(
            signalledStarted.Elapsed < PromptReturn,
            "A pump waited even though the wake flag was set."
        );

        // The pump above cleared the wake flag, so this one waits its full timeout.
        var idleStarted = Stopwatch.StartNew();
        runtime.Pump(TimeSpan.FromMilliseconds(200));
        Assert.True(
            idleStarted.Elapsed >= TimeSpan.FromMilliseconds(100),
            "The first pump left the wake flag set."
        );
    }
}
