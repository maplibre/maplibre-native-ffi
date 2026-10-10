using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;

namespace Maplibre.NativeFfi.Internal.Status;

internal static unsafe class NativeStatus
{
    /// <summary>
    /// Throws for a failed status, with the message the call wrote to diagnostic.
    /// </summary>
    internal static void Check(mln_status status, mln_diagnostic* diagnostic)
    {
        if (status != mln_status.MLN_STATUS_OK)
        {
            throw CreateException((int)status, NativeDiagnostic.Message(diagnostic));
        }
    }

    internal static void Check(int rawStatus, string diagnostic)
    {
        if (rawStatus != (int)MaplibreStatus.Ok)
        {
            throw CreateException(rawStatus, diagnostic);
        }
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

    internal static MaplibreStatus StatusFromRaw(int rawStatus) =>
        rawStatus switch
        {
            0 => MaplibreStatus.Ok,
            -1 => MaplibreStatus.InvalidArgument,
            -2 => MaplibreStatus.InvalidState,
            -3 => MaplibreStatus.WrongThread,
            -4 => MaplibreStatus.Unsupported,
            -5 => MaplibreStatus.NativeError,
            -6 => MaplibreStatus.Cancelled,
            -7 => MaplibreStatus.Busy,
            -8 => MaplibreStatus.TargetLost,
            -9 => MaplibreStatus.NotReady,
            -10 => MaplibreStatus.NotFound,
            _ => MaplibreStatus.Unknown,
        };
}
