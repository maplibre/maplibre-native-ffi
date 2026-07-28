using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Handle;
using Maplibre.Native.Internal.Status;

namespace Maplibre.Native.Runtime;

/// <summary>
/// Releases a runtime owner thread parked in <see cref="RuntimeHandle.Wait" />.
/// </summary>
/// <remarks>
/// Unlike the other handles here, a wake source is usable from any thread: it is
/// what a host's task submission or shutdown path calls. It stays usable after its
/// runtime closes, and signalling it then does nothing.
/// </remarks>
public sealed unsafe class WakeSource : IDisposable
{
    private readonly NativeHandleState<mln_wake_source> state;

    internal WakeSource(mln_wake_source* source)
    {
        state = new NativeHandleState<mln_wake_source>(
            source,
            static handle =>
            {
                NativeMethods.mln_wake_source_destroy(handle);
                return mln_status.MLN_STATUS_OK;
            },
            nameof(WakeSource)
        );
    }

    /// <summary>Whether this wrapper has released its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Latches a wake and releases the parked owner thread.</summary>
    /// <remarks>
    /// A signal raised while the owner thread runs is latched, so the next
    /// <see cref="RuntimeHandle.Wait" /> consumes it without blocking. Signalling
    /// after the runtime closes succeeds and does nothing.
    /// </remarks>
    public void Signal()
    {
        NativeStatus.Check(NativeMethods.mln_wake_source_signal(state.Pointer));
    }

    /// <summary>Releases the wake source.</summary>
    public void Close()
    {
        state.Close();
    }

    /// <inheritdoc />
    public void Dispose()
    {
        state.TryClose();
    }
}
