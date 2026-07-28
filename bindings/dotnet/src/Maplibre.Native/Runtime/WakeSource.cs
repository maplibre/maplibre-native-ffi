using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Handle;
using Maplibre.Native.Internal.Status;

namespace Maplibre.Native.Runtime;

/// <summary>
/// Releases a runtime owner thread parked in <see cref="RuntimeHandle.Pump" />.
/// </summary>
/// <remarks>
/// A wake source is usable from any thread, which a host's task submission and
/// shutdown paths rely on. It stays usable after its runtime closes, and signalling
/// it then does nothing.
/// </remarks>
public sealed unsafe class WakeSource : IDisposable
{
    // Held across the native signal and across close, so a close on another
    // thread cannot destroy the source between the pointer read and the call.
    private readonly Lock nativeCallGate = new();
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

    /// <summary>Sets the runtime's wake flag and releases the parked owner thread.</summary>
    /// <remarks>
    /// A signal raised while the owner thread is running sets the wake flag, so the
    /// next <see cref="RuntimeHandle.Pump" /> returns without parking. Signalling
    /// after the runtime closes succeeds and does nothing.
    /// </remarks>
    public void Signal()
    {
        lock (nativeCallGate)
        {
            NativeStatus.Check(NativeMethods.mln_wake_source_signal(state.Pointer));
        }
    }

    /// <summary>Releases the wake source.</summary>
    public void Close()
    {
        lock (nativeCallGate)
        {
            state.Close();
        }
    }

    /// <inheritdoc />
    public void Dispose()
    {
        lock (nativeCallGate)
        {
            state.TryClose();
        }
    }
}
