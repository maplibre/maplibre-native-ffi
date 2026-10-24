// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class Maplibre
{
    /// <summary>
    /// Reads MapLibre Native's process-global network status.
    /// </summary>
    /// <remarks>
    /// See <c>mln_network_get_status</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public static NetworkStatus NetworkGetStatus()
    {
        using var call = Enter(null);
        uint outStatus = default;
        Check(NativeMethods.mln_network_get_status(&outStatus, Diagnostic));
        return (NetworkStatus)outStatus;
    }

    /// <summary>
    /// Sets MapLibre Native's process-global network status.
    /// </summary>
    /// <remarks>
    /// See <c>mln_network_set_status</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public static void NetworkSetStatus(NetworkStatus status)
    {
        using var call = Enter(null);
        Check(NativeMethods.mln_network_set_status((uint)status, Diagnostic));
    }
}
