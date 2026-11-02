// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A request for one frame.
/// </summary>
/// <remarks>
/// See <c>mln_frame_demand</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Token">
/// A value of your choice that the request's frame result carries.
/// </param>
public readonly partial record struct FrameDemand(FrameDemandFlag Flags, ulong Token);
