// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Runtime and map event delivery. Set either callback or queue.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_handler</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct RuntimeEventHandler(Action<EventBatchView>? Callback);
