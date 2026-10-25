// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Synchronization payload kind for acquired texture frames.
/// </summary>
/// <remarks>
/// See <c>mln_gpu_sync_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public enum GpuSyncKind : uint
{
    /// <summary>
    /// The host needs no synchronization object. The work completed, or on
    /// WebGPU was submitted to the device's queue, before the frame became
    /// acquirable or before the release call.
    /// </summary>
    CpuComplete = 0,

    /// <summary>
    /// GLsync in the shared EGL or WGL context's share group, used only by a
    /// caller-graphics-thread driver. handle is nonzero.
    /// </summary>
    OpenGlFence = 1,
}
