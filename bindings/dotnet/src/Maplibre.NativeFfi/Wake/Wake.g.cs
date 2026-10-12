// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Receiver wake callback copied by a successful owning call. The callback
/// schedules service by the receiver and returns without waiting for another
/// thread.
/// </summary>
/// <remarks>
/// See <c>mln_wake</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct Wake(Action? Callback);
