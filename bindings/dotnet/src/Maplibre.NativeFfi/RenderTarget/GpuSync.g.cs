// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A point in GPU work that the other side waits for before using a texture.
/// </summary>
/// <remarks>
/// See <c>mln_gpu_sync</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Handle">
/// The object to wait on, for the kinds that name one.
/// </param>
public readonly partial record struct GpuSync(GpuSyncKind Kind, ulong Handle, ulong Value);
