using System.Diagnostics;

namespace Maplibre.NativeFfi.Internal.Pointer;

internal static class NativeLeakReporter
{
    /// <summary>
    /// Writes a leak report as a trace warning, which the host's trace listeners route. The
    /// default listener writes only to an attached debugger, so with no other listener the
    /// report goes to standard error instead.
    /// </summary>
    internal static void Report(string message)
    {
        try
        {
            var text = $"Maplibre.NativeFfi LeakedHandle: {message}";
            if (HasHostListener())
                Trace.TraceWarning(text);
            else
                Console.Error.WriteLine(text);
        }
        catch
        {
            // Leak reporting must not throw from finalizers or best-effort Dispose paths.
        }
    }

    /// <summary>
    /// Whether a host added a trace listener. The default listener alone does not count.
    /// </summary>
    internal static bool HasHostListener()
    {
        lock (Trace.Listeners)
        {
            foreach (TraceListener listener in Trace.Listeners)
                if (listener is not DefaultTraceListener)
                    return true;
        }
        return false;
    }
}
