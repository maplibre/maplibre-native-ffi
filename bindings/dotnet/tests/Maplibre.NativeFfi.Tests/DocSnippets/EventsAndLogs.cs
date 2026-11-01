// Snippets for the docs site's Events and logs page. They compile with the
// tests, which never run them.

namespace Maplibre.NativeFfi.Tests.DocSnippets;

internal static class EventsAndLogs
{
    private static MapHandle? map;

    private static void OnStyleLoaded() { }

    private static void OnLoadingFailed(string message) { }

    private static void WriteLog(LogSeverity severity, string message) { }

    // #region handler
    // Runs on a library thread.
    private static void OnEvents(RuntimeEvent[] events)
    {
        foreach (var e in events)
        {
            if (e.Source != map?.Id)
                continue;

            switch (e.Type)
            {
                case RuntimeEventType.MapStyleLoaded:
                    OnStyleLoaded();
                    break;
                case RuntimeEventType.MapLoadingFailed:
                    OnLoadingFailed(e.Message);
                    break;
            }
        }
    }

    // #endregion handler

    internal static async Task Run(RuntimeHandle runtime)
    {
        // #region handler
        runtime.Discarding.SetEventHandler(new RuntimeEventHandler(OnEvents));
        // #endregion handler

        // #region mask
        map = runtime.CreateMap(
            new MapOptions
            {
                InitialExtent = new LogicalExtent(512, 512, 1),
                EventMask = RuntimeEventFlag.MapStyleLoaded | RuntimeEventFlag.MapLoadingFailed,
            }
        );
        // #endregion mask
        await map.Ready;

        // #region logs
        // Runs on a library thread, and can't call the library.
        MaplibreNative.SetLogCallback(
            new LogHandler(
                (severity, _, _, message) =>
                {
                    WriteLog(severity, message);
                    // Consume the record. Return false to let the platform
                    // logger print it too.
                    return true;
                }
            )
        );
        // #endregion logs
    }
}
