// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Takes or returns the host's lock on the queue that a session shares with it.
/// Native code relies on the lock being held or released when the call returns.
/// The callback runs on the session's graphics thread or on the thread that
/// abandons the session.
/// </summary>
/// <remarks>
/// See <c>mln_queue_lock_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public delegate void QueueLockCallback();
