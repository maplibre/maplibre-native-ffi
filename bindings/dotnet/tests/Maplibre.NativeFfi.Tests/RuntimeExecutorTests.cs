using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RuntimeExecutorTests
{
    [BindingSpecTest("")]
    [Fact]
    public async Task NativeWorkProgressesAutonomously()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = await runtime.MapCreateAsync(
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        _ = map.SetStyleUrlAsync("unsupported://style.json", TestContext.Current.CancellationToken);
        await runtime.BarrierAsync(TestContext.Current.CancellationToken);

        RuntimeEventTestHelpers.WaitForMapEvent(runtime, map, RuntimeEventType.MapLoadingFailed);
    }

    // A completion is delivered by the runtime itself, so a host that never drains events still
    // observes every barrier it awaits.
    [Fact]
    public async Task RepeatedCompletionWaitsNeedNoEventDrain()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);

        for (var index = 0; index < 256; index++)
        {
            await runtime.BarrierAsync(TestContext.Current.CancellationToken);
        }

        Assert.Empty(runtime.DrainEventCopies());
    }
}
