// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Host lock on the graphics queue that a session shares with its host, copied
/// by a successful attach.
/// </summary>
/// <remarks>
/// See <c>mln_queue_lock</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct QueueLock(Action? Lock, Action? Unlock);
