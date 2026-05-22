using System.Runtime.InteropServices;

namespace Maplibre.Native.Internal.C;

internal static unsafe partial class NativeMethods
{
    internal const string LibraryName = "maplibre-native-c";

    [LibraryImport(LibraryName, EntryPoint = "mln_c_version")]
    internal static partial uint MlnCVersion();

    [LibraryImport(LibraryName, EntryPoint = "mln_supported_render_backend_mask")]
    internal static partial uint MlnSupportedRenderBackendMask();

    [LibraryImport(LibraryName, EntryPoint = "mln_network_status_get")]
    internal static partial int MlnNetworkStatusGet(out uint status);

    [LibraryImport(LibraryName, EntryPoint = "mln_network_status_set")]
    internal static partial int MlnNetworkStatusSet(uint status);

    [LibraryImport(LibraryName, EntryPoint = "mln_thread_last_error_message")]
    internal static partial byte* MlnThreadLastErrorMessage();
}
