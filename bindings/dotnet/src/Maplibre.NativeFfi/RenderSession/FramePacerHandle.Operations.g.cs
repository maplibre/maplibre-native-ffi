// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// Demands render sessions' frames at a display's refreshes.
/// </summary>
/// <remarks>
/// See <c>mln_frame_pacer</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class FramePacerHandle : IDisposable, INativeOwner<MlnFramePacer>
{
    private readonly NativeHandleState<MlnFramePacer> state;

    internal FramePacerHandle(MlnFramePacer handle)
    {
        state = new(handle, Abandon, nameof(FramePacerHandle), Abandon);
    }

    internal static FramePacerHandle Adopt(MlnFramePacer handle) =>
        NativeHandleState<MlnFramePacer>.Adopt(handle, () => new FramePacerHandle(handle), Abandon);

    private static mln_status Abandon(MlnFramePacer live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_frame_pacer_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnFramePacer> INativeOwner<MlnFramePacer>.State => state;
    internal MlnFramePacer Handle => state.Handle;

    /// <summary>
    /// The native handle value. It stays readable after release.
    /// </summary>
    public ulong Id => state.Handle.Value;

    /// <summary>
    /// Whether a release, dispose, or discard has ended the handle.
    /// </summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>
    /// Names the handle's type, its <see cref="Id" />, and whether it is closed.
    /// </summary>
    public override string ToString() => state.ToString();

    void IDisposable.Dispose()
    {
        state.Close();
    }

    /// <summary>
    /// Releases a frame pacer.
    /// </summary>
    /// <remarks>
    /// <para>Render sessions that use the pacer stay paced until you release
    /// them or set another pacer.</para>
    /// <para>See <c>mln_frame_pacer_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// </remarks>
    public void Release()
    {
        state.Close();
    }
}
