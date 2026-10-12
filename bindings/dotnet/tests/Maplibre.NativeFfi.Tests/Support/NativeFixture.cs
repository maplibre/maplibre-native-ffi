using System.Diagnostics;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>A runtime and one map for a test, torn down when the test disposes the fixture.</summary>
/// <remarks>
/// The runtime answers every resource request with an error unless the test installs its own
/// provider, so no test reaches the network. The runtime's event wake releases a semaphore, and
/// <see cref="WaitForMapEventAsync" /> blocks on it between drains.
/// </remarks>
internal sealed class NativeFixture : IAsyncDisposable
{
    internal static byte[] EmptyStyle => """{"version":8,"sources":{},"layers":[]}"""u8.ToArray();

    internal static MapOptions SmallMap =>
        MapOptions.Default with
        {
            InitialExtent = new LogicalExtent(64, 64, 1),
        };

    /// <summary>Completes every request with a not-found error, so nothing leaves the process.</summary>
    internal static ResourceProvider DenyingProvider =>
        new(
            (_, request) =>
            {
                request.Complete(
                    new ResourceResponse
                    {
                        Status = ResourceResponseStatus.Error,
                        ErrorReason = ResourceErrorReason.NotFound,
                        ErrorMessage = "The test fixture serves no resources.",
                    }
                );
                request.Close();
                return ResourceProviderDecision.Handle;
            }
        );

    private readonly SemaphoreSlim events;

    private NativeFixture(RuntimeHandle runtime, MapHandle map, SemaphoreSlim events)
    {
        Runtime = runtime;
        Map = map;
        this.events = events;
    }

    internal RuntimeHandle Runtime { get; }

    internal MapHandle Map { get; }

    internal static async Task<NativeFixture> CreateAsync(
        MapOptions? options = null,
        ResourceProvider? provider = null
    )
    {
        var events = new SemaphoreSlim(0);
        var runtime = RuntimeHandle.Create(
            RuntimeOptions.Default with
            {
                EventWake = new Wake(() => events.Release()),
            }
        );
        try
        {
            await runtime.SetResourceProviderAsync(provider ?? DenyingProvider, TestWaits.Token);
            var map = await runtime
                .CreateMapAsync(options ?? SmallMap)
                .WaitAsync(TestWaits.Deadline, TestWaits.Token);
            return new NativeFixture(runtime, map, events);
        }
        catch
        {
            await runtime.CloseAsync();
            events.Dispose();
            throw;
        }
    }

    /// <summary>Creates a fixture whose map has loaded an empty style.</summary>
    internal static async Task<NativeFixture> WithEmptyStyleAsync(MapOptions? options = null)
    {
        var fixture = await CreateAsync(options);
        try
        {
            await fixture.Map.SetStyleJsonAsync(EmptyStyle, TestWaits.Token);
            return fixture;
        }
        catch
        {
            await fixture.DisposeAsync();
            throw;
        }
    }

    /// <summary>Drains the runtime's events until the map reports one of the given type.</summary>
    internal async Task<RuntimeEvent> WaitForMapEventAsync(RuntimeEventType type)
    {
        var elapsed = Stopwatch.StartNew();
        while (true)
        {
            using (var batch = Runtime.DrainEvents())
            {
                foreach (var runtimeEvent in batch?.Get().Events ?? [])
                {
                    if (
                        runtimeEvent.Type == type
                        && runtimeEvent.SourceType == RuntimeEventSourceType.Map
                        && runtimeEvent.Source == Map.Id
                    )
                        return runtimeEvent;
                }
            }
            await TestWaits.WaitAsync(events, elapsed, $"map event {type}");
        }
    }

    public async ValueTask DisposeAsync()
    {
        try
        {
            if (!Map.IsClosed)
                await Map.CloseAsync();
        }
        finally
        {
            if (!Runtime.IsClosed)
                await Runtime.CloseAsync();
            // Runtime teardown releases the event wake, so nothing signals the semaphore after it.
            events.Dispose();
        }
    }
}
