// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A GPU device that the library creates to render maps into textures.
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
    /// Creates a GPU device for rendering maps into textures.
    /// </summary>
    /// <remarks>
    /// <para>Render sessions on several maps can share one device.</para>
    /// <para>See <c>mln_graphics_device_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when this build's render
    /// backend can't create a device.</description></item>
    /// <item><description><c>Status.NativeError</c> when <c>adapter_index</c>
    /// names no GPU, or the platform can't create the
    /// device.</description></item>
    /// </list>
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
