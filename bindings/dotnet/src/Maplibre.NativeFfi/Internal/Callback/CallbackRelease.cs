using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.Callback;

/// <summary>Shared body for a callback descriptor's release-user-data stub.</summary>
internal static unsafe class CallbackRelease
{
    /// <summary>Disposes the state one callback descriptor's user data points at.</summary>
    /// <remarks>
    /// The C API runs this from native teardown, so nothing here may throw across the boundary. A
    /// state that cannot dispose leaves its handle to the finalizer, which reports the leak.
    /// </remarks>
    internal static void Dispose<T>(void* userData)
        where T : class, IDisposable
    {
        try
        {
            ((T?)GCHandle.FromIntPtr((nint)userData).Target)?.Dispose();
        }
        catch { }
    }
}
