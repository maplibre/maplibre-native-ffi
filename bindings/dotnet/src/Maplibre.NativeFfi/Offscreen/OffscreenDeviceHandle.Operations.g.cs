// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A GPU device that the library creates to render maps offscreen.
/// </summary>
/// <remarks>
/// See <c>mln_offscreen_device</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class OffscreenDeviceHandle
    : IDisposable,
        INativeOwner<MlnOffscreenDevice>
{
    private readonly NativeHandleState<MlnOffscreenDevice> state;

    internal OffscreenDeviceHandle(MlnOffscreenDevice handle)
    {
        state = new(handle, Abandon, nameof(OffscreenDeviceHandle), Abandon);
    }

    internal static OffscreenDeviceHandle Adopt(MlnOffscreenDevice handle) =>
        NativeHandleState<MlnOffscreenDevice>.Adopt(
            handle,
            () => new OffscreenDeviceHandle(handle),
            Abandon
        );

    private static mln_status Abandon(MlnOffscreenDevice live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_offscreen_device_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnOffscreenDevice> INativeOwner<MlnOffscreenDevice>.State => state;
    internal MlnOffscreenDevice Handle => state.Handle;

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
    /// Creates a GPU device for rendering maps offscreen.
    /// </summary>
    /// <remarks>
    /// <para><c>MapHandle.AttachOffscreen()</c> creates a device of its own
    /// when it gets none. Create one to share it among the sessions on many
    /// maps, or to pick a GPU.</para>
    /// <para>In a browser WebGPU build, attach without a device instead.</para>
    /// <para>See <c>mln_offscreen_device_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> in a browser WebGPU build,
    /// or when the device can't render with
    /// <c>client_api</c>.</description></item>
    /// <item><description><c>Status.NativeError</c> when <c>adapter_index</c>
    /// names no GPU, or the platform can't create the
    /// device.</description></item>
    /// </list>
    /// </remarks>
    public static OffscreenDeviceHandle Create(OffscreenDeviceOptions? options = null)
    {
        using var call = Enter(null);
        var nativeOptions = options is null
            ? default(mln_offscreen_device_options)
            : NativeOffscreenDeviceOptions(options);
        MlnOffscreenDevice outDevice = default;
        Check(
            NativeMethods.mln_offscreen_device_create(
                options is null ? null : &nativeOptions,
                &outDevice,
                Diagnostic
            )
        );
        return OffscreenDeviceHandle.Adopt(outDevice);
    }

    /// <summary>
    /// Releases an offscreen device.
    /// </summary>
    /// <remarks>
    /// <para>Render sessions on the device keep working until they
    /// detach.</para>
    /// <para>See <c>mln_offscreen_device_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html">C API reference</see>.</para>
    /// </remarks>
    public void Release()
    {
        state.Close();
    }
}
