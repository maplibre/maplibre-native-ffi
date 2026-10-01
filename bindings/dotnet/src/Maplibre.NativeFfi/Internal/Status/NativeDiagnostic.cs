using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using Maplibre.NativeFfi.Internal.C;

// Each status-returning call declares an mln_diagnostic on its stack, and native
// writes the message before returning, so zeroing the 4 KB buffer on every call
// buys nothing. Code that needs zeroed stack memory initializes it explicitly.
[module: SkipLocalsInit]

namespace Maplibre.NativeFfi.Internal.Status;

internal static unsafe class NativeDiagnostic
{
    // Native code writes the message as a call returns, and the binding reads it
    // before the thread makes another call, so one pinned buffer serves every
    // call a thread makes, including calls from reentrant synchronous callbacks.
    [ThreadStatic]
    private static mln_diagnostic[]? current;

    /// <summary>The calling thread's diagnostic, prepared for one call.</summary>
    internal static mln_diagnostic* Current
    {
        get
        {
            current ??= GC.AllocateArray<mln_diagnostic>(1, pinned: true);
            return Prepare((mln_diagnostic*)Unsafe.AsPointer(ref current[0]));
        }
    }

    /// <summary>Sets the size of an uninitialized diagnostic and returns it for the call.</summary>
    internal static mln_diagnostic* Prepare(mln_diagnostic* diagnostic)
    {
        diagnostic->size = (uint)sizeof(mln_diagnostic);
        return diagnostic;
    }

    /// <summary>Copies the message a returned call wrote to its diagnostic.</summary>
    internal static string Message(mln_diagnostic* diagnostic)
    {
        ReadOnlySpan<sbyte> message = diagnostic->message;
        var bytes = MemoryMarshal.AsBytes(message);
        var length = bytes.IndexOf((byte)0);
        return Encoding.UTF8.GetString(length < 0 ? bytes : bytes[..length]);
    }
}
