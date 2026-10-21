using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;

namespace Maplibre.NativeFfi.Internal.Status;

internal static unsafe class NativeStatus
{
    internal static void Check(mln_status status, mln_diagnostic* diagnostic)
    {
        if (status != mln_status.MLN_STATUS_OK)
            throw new MaplibreException(
                (global::Maplibre.NativeFfi.Status)status,
                NativeDiagnostic.Message(diagnostic)
            );
    }

    internal static void Check(int rawStatus, string diagnostic)
    {
        if (rawStatus != 0)
            throw new MaplibreException((global::Maplibre.NativeFfi.Status)rawStatus, diagnostic);
    }
}
