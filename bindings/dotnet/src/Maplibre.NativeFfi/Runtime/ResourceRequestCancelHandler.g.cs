// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Cancel callback state for one handled resource request.
/// </summary>
/// <remarks>
/// See <c>mln_resource_request_cancel_handler</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct ResourceRequestCancelHandler(Action? Callback);
