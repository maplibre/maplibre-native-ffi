using Maplibre.Native.Map;
using Maplibre.Native.Runtime;

namespace Maplibre.Native.Tests;

internal static class RuntimeEventTestHelpers
{
    internal static RuntimeEvent WaitForMapEvent(
        RuntimeHandle runtime,
        MapHandle map,
        RuntimeEventType eventType
    )
    {
        for (var attempt = 0; attempt < 1000; attempt++)
        {
            runtime.RunOnce();
            while (true)
            {
                var runtimeEvent = runtime.PollEvent();
                if (runtimeEvent is null)
                {
                    break;
                }

                if (
                    runtimeEvent.Type == eventType
                    && runtimeEvent.SourceType == RuntimeEventSourceType.Map
                    && ReferenceEquals(runtimeEvent.MapSource, map)
                )
                {
                    return runtimeEvent;
                }
            }

            Thread.Sleep(1);
        }

        throw new TimeoutException($"Timed out waiting for map event {eventType}.");
    }
}
