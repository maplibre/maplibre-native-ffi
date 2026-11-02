// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Receives a batch of events.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public delegate void RuntimeEventCallback(RuntimeEvent[] events);
