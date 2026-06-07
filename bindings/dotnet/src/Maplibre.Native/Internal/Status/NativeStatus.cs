using System.Runtime.InteropServices;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;

namespace Maplibre.Native.Internal.Status;

internal static unsafe class NativeStatus
{
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
                diagnostic
            ),
            MaplibreStatus.InvalidState => new InvalidStateException(status, rawStatus, diagnostic),
            MaplibreStatus.WrongThread => new WrongThreadException(status, rawStatus, diagnostic),
            MaplibreStatus.Unsupported => new UnsupportedFeatureException(
                status,
                rawStatus,
                diagnostic
            ),
            MaplibreStatus.NativeError => new NativeErrorException(status, rawStatus, diagnostic),
            _ => new MaplibreException(status, rawStatus, diagnostic),
        };
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
}
