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

    /// <summary>Throws a completion's status, with the generation a failed command published.</summary>
    internal static void Check(mln_status status, string diagnostic, ulong generation)
    {
        if (status != mln_status.MLN_STATUS_OK)
            throw new MaplibreException((global::Maplibre.NativeFfi.Status)status, diagnostic)
            {
                Generation = generation,
            };
    }
}
