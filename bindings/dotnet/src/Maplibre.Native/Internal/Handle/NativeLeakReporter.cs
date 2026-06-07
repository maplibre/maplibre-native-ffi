using Maplibre.Native.Internal.C;

namespace Maplibre.Native.Internal.Handle;

internal enum NativeLeakReportKind
{
    LeakedHandle,
    DisposeFailed,
}

internal readonly record struct NativeLeakReport(
    NativeLeakReportKind Kind,
    string TypeName,
    nint Address,
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
            Console.Error.WriteLine($"Maplibre.Native {report.Kind}: {report.Message}");
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
