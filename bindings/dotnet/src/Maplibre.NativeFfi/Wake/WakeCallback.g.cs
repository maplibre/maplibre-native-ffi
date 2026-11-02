// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Tells the caller that work is ready for it.
/// </summary>
/// <remarks>
/// <para>One call can stand for several pieces of ready work, so take all of it
/// each time.</para>
/// <para>Can run on any thread, including the caller's own thread during a
/// library call. Return quickly and don't block.</para>
/// <para>See <c>mln_wake_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html">C API reference</see>.</para>
/// </remarks>
public delegate void WakeCallback();
