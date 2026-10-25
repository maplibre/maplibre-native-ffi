// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One nonblocking request for a frame.
/// </summary>
/// <remarks>
/// See <c>mln_frame_demand</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Flags">
/// A bitwise OR of <c>mln_frame_demand_flag</c> values. Zero renders only when
/// a newer map update exists, without presenting.
/// </param>
/// <param name="Token">
/// Host identity returned with the terminal frame result. Any value except
/// <c>MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE</c>, the maximum uint64_t value.
/// </param>
public readonly partial record struct FrameDemand(FrameDemandFlag Flags, ulong Token);
