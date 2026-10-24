namespace Maplibre.NativeFfi.Internal.Pointer;

internal static class NativeLeakReporter
{
    /// <summary>
    /// Reports a leaked handle to each <see cref="Maplibre.LeakReported" /> handler, or to
    /// standard error when there is none.
    /// </summary>
    internal static void Report(string typeName, string message)
    {
        // Leak reporting must not throw from finalizers or best-effort Dispose paths.
        var handlers = Maplibre.LeakReportedHandlers;
        if (handlers is null)
        {
            try
            {
                Console.Error.WriteLine($"Maplibre.NativeFfi LeakedHandle: {message}");
            }
            catch { }
            return;
        }
        var args = new LeakReportedEventArgs(typeName, message);
        foreach (var handler in handlers.GetInvocationList())
        {
            try
            {
                ((EventHandler<LeakReportedEventArgs>)handler)(null, args);
            }
            catch
            {
                // One handler's failure must neither escape the finalizer nor skip the others.
            }
        }
    }
}
