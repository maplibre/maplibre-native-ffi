// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Locks or unlocks the Vulkan queue that you share with a render session.
/// </summary>
/// <remarks>
/// <para>It can run on a library thread, and it can't call the library.</para>
/// <para>See <c>mln_queue_lock_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public delegate void QueueLockCallback();
