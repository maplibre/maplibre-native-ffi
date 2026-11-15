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
    /// Creates a frame pacer that follows the platform's display.
    /// </summary>
    /// <remarks>
    /// <para>On iOS, macOS, and tvOS, the pacer follows the display through a
    /// <c>CADisplayLink</c>. You can call it on any thread, and the pacer needs
    /// no run loop of yours. On macOS, it follows the main display, the one
    /// with the menu bar. With <c>FramePacerOptions.MaximumFramesPerSecond</c>,
    /// the pacer also asks the system for that refresh rate. On iPhone, a rate
    /// above 60 needs <c>CADisableMinimumFrameDurationOnPhone</c> in the app's
    /// <c>Info.plist</c>.</para>
    /// <para>On Android, the pacer follows the display through the calling
    /// thread's <c>AChoreographer</c>. Call it on a thread with an
    /// <c>ALooper</c>, such as the app's main thread, and keep that thread
    /// running its looper while sessions use the pacer. <c>AChoreographer</c>
    /// can stop while the phone's display is off, even while a Surface on
    /// another display, such as an Android Auto projection, still shows the
    /// map. The pacer then keeps demanding frames from a timer on the looper,
    /// within the frame rate limit, until vsyncs arrive again.</para>
    /// <para>On OpenHarmony, the pacer follows the display through
    /// <c>OH_NativeVSync</c>. You can call it on any thread, and the pacer
    /// needs no event loop of yours.</para>
    /// <para>See <c>mln_frame_pacer_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> on the other platforms,
    /// such as visionOS, Linux, and Windows, where the library has no source of
    /// display refreshes.</description></item>
    /// <item><description><c>Status.InvalidState</c> on Android when the
    /// calling thread has no <c>ALooper</c>.</description></item>
    /// <item><description><c>Status.NativeError</c> on iOS, macOS, and tvOS
    /// when the system has no display to follow, and on OpenHarmony when the
    /// system can't report the display's refreshes to the
    /// process.</description></item>
    /// </list>
    /// </remarks>
    public static FramePacerHandle Create(FramePacerOptions? options = null)
    {
        using var call = Enter(null);
        var nativeOptions = options is null
            ? default(mln_frame_pacer_options)
            : NativeFramePacerOptions(options);
        MlnFramePacer outPacer = default;
        Check(
            NativeMethods.mln_frame_pacer_create(
                options is null ? null : &nativeOptions,
                &outPacer,
                Diagnostic
            )
        );
        return FramePacerHandle.Adopt(outPacer);
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

    /// <summary>
    /// Changes a frame pacer's settings.
    /// </summary>
    /// <remarks>
    /// <para>Settings that <c>options</c> doesn't set keep their current
    /// values. To remove the frame rate limit, set
    /// <c>FramePacerOptions.UnlimitedFramesPerSecond</c>. You can call it on
    /// any thread.</para>
    /// <para>The sessions that the pacer serves follow the new settings from
    /// the next display refresh, including a refresh that they already wait
    /// for. The pacer counts a new limit from the last frames it demanded, so a
    /// higher limit can demand the next frames sooner. On iOS, macOS, and tvOS,
    /// the pacer also asks the system for the new limit's refresh rate, or for
    /// the display's own rate once the limit is removed.</para>
    /// <para>See <c>mln_frame_pacer_set_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidState</c> when you released
    /// <c>pacer</c>, even while sessions still use it.</description></item>
    /// </list>
    /// </remarks>
    public void SetOptions(FramePacerOptions options)
    {
        using var call = Enter(this);
        var nativeOptions = NativeFramePacerOptions(options);
        Check(NativeMethods.mln_frame_pacer_set_options(Handle, &nativeOptions, Diagnostic));
    }
}
