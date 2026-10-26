// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A GPU device that native code creates for texture render targets.
/// </summary>
/// <remarks>
/// See <c>mln_graphics_device</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class GraphicsDeviceHandle
    : IDisposable,
        INativeOwner<MlnGraphicsDevice>
{
    private readonly NativeHandleState<MlnGraphicsDevice> state;

    internal GraphicsDeviceHandle(MlnGraphicsDevice handle)
    {
        state = new(handle, Abandon, nameof(GraphicsDeviceHandle), Abandon);
    }

    internal static GraphicsDeviceHandle Adopt(MlnGraphicsDevice handle) =>
        NativeHandleState<MlnGraphicsDevice>.Adopt(
            handle,
            () => new GraphicsDeviceHandle(handle),
            Abandon
        );

    private static mln_status Abandon(MlnGraphicsDevice live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_graphics_device_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnGraphicsDevice> INativeOwner<MlnGraphicsDevice>.State => state;
    internal MlnGraphicsDevice Handle => state.Handle;

    /// <summary>
    /// The native handle value. It stays readable after close.
    /// </summary>
    public ulong Id => state.Handle.Value;

    /// <summary>
    /// Whether a release, dispose, close, or discard has ended the handle.
    /// </summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>
    /// Names the handle's type, its <see cref="Id" />, and whether it is closed.
    /// </summary>
    public override string ToString() => state.ToString();

    public void Dispose()
    {
        state.Retire();
    }

    /// <summary>
    /// Creates a graphics device for this build's render backend.
    /// </summary>
    /// <remarks>
    /// See <c>mln_graphics_device_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html">C API reference</see>.
    /// </remarks>
    public static GraphicsDeviceHandle Create(GraphicsDeviceOptions? options = null)
    {
        using var call = Enter(null);
        var nativeOptions = options is null
            ? default(mln_graphics_device_options)
            : NativeGraphicsDeviceOptions(options);
        MlnGraphicsDevice outDevice = default;
        Check(
            NativeMethods.mln_graphics_device_create(
                options is null ? null : &nativeOptions,
                &outDevice,
                Diagnostic
            )
        );
        return GraphicsDeviceHandle.Adopt(outDevice);
    }
}
