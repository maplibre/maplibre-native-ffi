// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Backend synchronization copied by frame access and release calls.
/// </summary>
/// <remarks>
/// See <c>mln_gpu_sync</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Handle">
/// Bit pattern of the GLsync pointer.
/// </param>
public readonly partial record struct GpuSync(GpuSyncKind Kind, ulong Handle);
