using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Internal.Loader;
using Maplibre.NativeFfi.Internal.Status;

namespace Maplibre.NativeFfi.Internal;

/// <summary>
/// The steps every generated synchronous operation takes around its C call.
/// </summary>
internal static unsafe class NativeCall
{
    /// <summary>
    /// Checks that the calling thread may enter <paramref name="operation"/>
    /// and keeps its receiver reachable until the returned scope is disposed.
    /// An operation without a receiver then loads the native library.
    /// </summary>
    internal static Entry Enter(object? owner, string operation)
    {
        NativeCallbackGuard.EnsureAllowed(owner, operation);
        if (owner is null)
            NativeLibraryLoader.EnsureLoaded();
        return new Entry(owner);
    }

    /// <summary>The diagnostic to pass a status-returning C call from this thread.</summary>
    internal static mln_diagnostic* Diagnostic => NativeDiagnostic.Current;

    /// <summary>Throws for a failed status with the message the call wrote to <see cref="Diagnostic"/>.</summary>
    internal static void Check(mln_status status)
    {
        if (status != mln_status.MLN_STATUS_OK)
            NativeStatus.Check(status, NativeDiagnostic.Current);
    }

    internal readonly ref struct Entry(object? owner)
    {
        public void Dispose() => GC.KeepAlive(owner);
    }
}
