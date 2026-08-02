using Maplibre.NativeFfi.Internal.C;

namespace Maplibre.NativeFfi.Internal.Pointer;

internal enum NativeLeakReportKind
{
    LeakedHandle,
    DisposeFailed,
}

/// <param name="Handle">
/// The C API handle id the leak is about, or zero when the leaked resource is
/// binding-allocated memory rather than a C API handle. Backend-native
/// addresses never appear here; they belong to <see cref="NativePointer" />.
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
    private static readonly Lock Gate = new();
    private static Action<NativeLeakReport>? sink;

    internal static void Report(NativeLeakReport report)
    {
        Action<NativeLeakReport>? current;
        lock (Gate)
        {
            current = sink;
        }

        if (current is not null)
        {
            try
            {
                current(report);
            }
            catch
            {
                // Leak reporting must not throw from finalizers or best-effort Dispose paths.
            }
            return;
        }

        try
        {
            Console.Error.WriteLine($"Maplibre.NativeFfi {report.Kind}: {report.Message}");
        }
        catch
        {
            // Diagnostics are best-effort only.
        }
    }

    internal static IDisposable CaptureForTest(Action<NativeLeakReport> replacement)
    {
        ArgumentNullException.ThrowIfNull(replacement);
        lock (Gate)
        {
            var previous = sink;
            sink = replacement;
            return new CaptureScope(previous);
        }
    }

    private sealed class CaptureScope(Action<NativeLeakReport>? previous) : IDisposable
    {
        private bool disposed;

        public void Dispose()
        {
            lock (Gate)
            {
                if (disposed)
                {
                    return;
                }

                sink = previous;
                disposed = true;
            }
        }
    }
}
