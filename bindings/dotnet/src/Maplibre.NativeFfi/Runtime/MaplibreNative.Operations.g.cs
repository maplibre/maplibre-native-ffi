// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class MaplibreNative
{
    /// <summary>
    /// Reads whether the library may use the network.
    /// </summary>
    /// <remarks>
    /// See <c>mln_network_get_status</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public static NetworkStatus GetNetworkStatus()
    {
        using var call = Enter(null);
        mln_network_status outStatus = default;
        Check(NativeMethods.mln_network_get_status(&outStatus, Diagnostic));
        return (NetworkStatus)outStatus;
    }

    /// <summary>
    /// Tells the library whether it may use the network.
    /// </summary>
    /// <remarks>
    /// <para>While the status is <c>NetworkStatus.Offline</c>, the library
    /// starts no network requests. The requests that wait start when the status
    /// returns to <c>NetworkStatus.Online</c>. The status starts as online and
    /// applies to every runtime in the process.</para>
    /// <para>See <c>mln_network_set_status</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// </remarks>
    public static void SetNetworkStatus(NetworkStatus status)
    {
        using var call = Enter(null);
        Check(NativeMethods.mln_network_set_status((mln_network_status)status, Diagnostic));
    }
}
