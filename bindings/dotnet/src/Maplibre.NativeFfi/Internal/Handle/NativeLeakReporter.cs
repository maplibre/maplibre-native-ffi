using Maplibre.NativeFfi.Internal.C;

namespace Maplibre.NativeFfi.Internal.Pointer;

internal enum NativeLeakReportKind
{
    LeakedHandle,
}

/// <param name="Handle">
/// The C API handle id the leak is about, or zero when the leaked resource is
/// binding-allocated memory. Backend-native addresses never appear here.
/// </param>
internal readonly record struct NativeLeakReport(
    NativeLeakReportKind Kind,
    string TypeName,
    ulong Handle,
    mln_status? Status,
    string Message
);

internal static class NativeLeakReporter
{
    /// <summary>Writes the report to standard error, where a host sees what it leaked.</summary>
    internal static void Report(NativeLeakReport report)
    {
        try
        {
            Console.Error.WriteLine($"Maplibre.NativeFfi {report.Kind}: {report.Message}");
        }
        catch
        {
            // Leak reporting must not throw from finalizers or best-effort Dispose paths.
        }
    }
}
