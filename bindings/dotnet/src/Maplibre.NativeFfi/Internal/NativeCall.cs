using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Loader;
using Maplibre.NativeFfi.Internal.Status;

namespace Maplibre.NativeFfi.Internal;

internal static unsafe class NativeCall
{
    /// <summary>
    /// Keeps the receiver reachable until the returned scope is disposed.
    /// An operation without a receiver then loads the native library.
    /// </summary>
    internal static Entry Enter(object? owner)
    {
        if (owner is null)
            NativeLibraryLoader.EnsureLoaded();
        return new Entry(owner);
    }

    internal static mln_diagnostic* Diagnostic => NativeDiagnostic.Current;

    internal static void Check(mln_status status)
    {
        if (status != mln_status.MLN_STATUS_OK)
            NativeStatus.Check(status, NativeDiagnostic.Current);
    }

    /// <summary>
    /// Checks <paramref name="status"/> like <see cref="Check"/>, except that
    /// <paramref name="absent"/> reports that native published no output.
    /// Returns whether the output holds a value.
    /// </summary>
    internal static bool Present(mln_status status, mln_status absent)
    {
        if (status == absent)
            return false;
        Check(status);
        return true;
    }

    internal readonly ref struct Entry(object? owner)
    {
        public void Dispose() => GC.KeepAlive(owner);
    }
}
