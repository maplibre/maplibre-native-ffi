// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Schedules service by the receiver that owns a queue or driver. Native code
/// may invoke the callback from any thread, and calls may coalesce or overlap.
/// </summary>
/// <remarks>
/// See <c>mln_wake_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html">C API reference</see>.
/// </remarks>
public delegate void WakeCallback();
