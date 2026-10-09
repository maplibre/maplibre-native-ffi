namespace Maplibre.NativeFfi.Internal.Pointer;

internal static class NativeLeakReporter
{
    /// <summary>Writes a leak report to standard error, where a host sees what it leaked.</summary>
    internal static void Report(string message)
    {
        try
        {
            Console.Error.WriteLine($"Maplibre.NativeFfi LeakedHandle: {message}");
        }
        catch
        {
            // Leak reporting must not throw from finalizers or best-effort Dispose paths.
        }
    }
}
