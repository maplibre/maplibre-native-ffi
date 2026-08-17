using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RuntimeExecutorTests
{
    [BindingSpecTest("BND-088")]
    [Fact]
    public async Task NativeWorkProgressesAutonomously()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = await MapHandle.CreateAsync(
            runtime,
            new MapOptions { Width = 512, Height = 512 },
            TestContext.Current.CancellationToken
        );

        map.SetStyleUrl("unsupported://style.json");
        await runtime.BarrierAsync(TestContext.Current.CancellationToken);

        RuntimeEventTestHelpers.WaitForMapEvent(runtime, map, RuntimeEventType.MapLoadingFailed);
    }

    [BindingSpecTest("BND-089")]
    [Fact]
    public async Task CrossThreadCommandsProgressAutonomously()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = await MapHandle.CreateAsync(
            runtime,
            new MapOptions { Width = 512, Height = 512 },
            TestContext.Current.CancellationToken
        );

        var commandId = await Task.Run(map.RequestRepaint);
        await runtime.BarrierAsync(TestContext.Current.CancellationToken);

        Assert.NotEqual(0ul, commandId);
    }

    [Fact]
    public async Task TaskOnlyOperationWaitsDoNotAccumulateReadyEndpoints()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());

        for (var index = 0; index < 256; index++)
        {
            await runtime.BarrierAsync(TestContext.Current.CancellationToken);
        }

        Assert.DoesNotContain(
            runtime.DrainReadyEndpoints(),
            endpoint => endpoint.Kind == NotificationEndpointKind.Operation
        );
    }
}
