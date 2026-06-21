using System.Runtime.InteropServices;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;

namespace Maplibre.Native.Internal.Status;

internal static unsafe class NativeStatus
{
    [ThreadStatic]
    private static Func<string>? diagnosticProviderForTest;

    internal static void Check(mln_status status)
    {
        Check((int)status);
    }

    internal static void Check(int rawStatus)
    {
        if (rawStatus == (int)MaplibreStatus.Ok)
        {
            return;
        }

        throw CreateException(rawStatus, CaptureDiagnostic());
    }

    private static MaplibreException CreateException(int rawStatus, string diagnostic)
    {
        var status = StatusFromRaw(rawStatus);
        return status switch
        {
            MaplibreStatus.InvalidArgument => new InvalidArgumentException(
                status,
                rawStatus,
                diagnostic,
                null
            ),
            MaplibreStatus.InvalidState => new InvalidStateException(
                status,
                rawStatus,
                diagnostic,
                null
            ),
            MaplibreStatus.WrongThread => new WrongThreadException(
                status,
                rawStatus,
                diagnostic,
                null
            ),
            MaplibreStatus.Unsupported => new UnsupportedFeatureException(
                status,
                rawStatus,
                diagnostic,
                null
            ),
            MaplibreStatus.NativeError => new NativeErrorException(
                status,
                rawStatus,
                diagnostic,
                null
            ),
            _ => new MaplibreException(status, rawStatus, diagnostic, null),
        };
    }

    internal static IDisposable UseDiagnosticProviderForTest(Func<string> provider)
    {
        var previous = diagnosticProviderForTest;
        diagnosticProviderForTest = provider;
        return new RestoreDiagnosticProvider(previous);
    }

    private static MaplibreStatus StatusFromRaw(int rawStatus) =>
        rawStatus switch
        {
            0 => MaplibreStatus.Ok,
            -1 => MaplibreStatus.InvalidArgument,
            -2 => MaplibreStatus.InvalidState,
            -3 => MaplibreStatus.WrongThread,
            -4 => MaplibreStatus.Unsupported,
            -5 => MaplibreStatus.NativeError,
            _ => MaplibreStatus.Unknown,
        };

    private static string CaptureDiagnostic()
    {
        if (diagnosticProviderForTest is { } provider)
        {
            return provider();
        }

        return CaptureNativeDiagnostic();
    }

    private static string CaptureNativeDiagnostic()
    {
        try
        {
            var message = NativeMethods.mln_thread_last_error_message();
            return message is null
                ? string.Empty
                : Marshal.PtrToStringUTF8((nint)message) ?? string.Empty;
        }
        catch (DllNotFoundException)
        {
            return string.Empty;
        }
    }

    private sealed class RestoreDiagnosticProvider(Func<string>? previous) : IDisposable
    {
        public void Dispose()
        {
            diagnosticProviderForTest = previous;
        }
    }
}
