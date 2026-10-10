// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public sealed unsafe partial class BufferHandle : IDisposable, INativeOwner<MlnBuffer>
{
    private readonly NativeHandleState<MlnBuffer> state;

    internal BufferHandle(MlnBuffer handle)
    {
        state = new(handle, Abandon, nameof(BufferHandle), Abandon);
    }

    internal static BufferHandle Adopt(MlnBuffer handle) =>
        NativeHandleState<MlnBuffer>.Adopt(handle, () => new BufferHandle(handle), Abandon);

    private static mln_status Abandon(MlnBuffer live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_buffer_destroy(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnBuffer> INativeOwner<MlnBuffer>.State => state;
    internal MlnBuffer Handle => state.Handle;
    internal NativeCallbackOwner CallbackOwner => state.CallbackOwner;

    // Runtime events report their source by this identity.
    public ulong Id => state.IssuedHandle.Value;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_buffer_destroy");
        state.Retire();
    }

    /// <summary>
    /// Destroys an owned buffer. A null handle is a no-op.
    /// </summary>
    /// <remarks>
    /// See <c>mln_buffer_destroy</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
    /// </remarks>
    public void Close()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_buffer_destroy");
        state.Close();
    }

    /// <summary>
    /// Borrows the data stored by an owned buffer.
    /// </summary>
    /// <remarks>
    /// See <c>mln_buffer_get</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
    /// </remarks>
    public byte[] Get()
    {
        using var read = state.Read(this, "mln_buffer_get");
        var outView = default(mln_buffer_view);
        Check(NativeMethods.mln_buffer_get(read.Handle, &outView, Diagnostic));
        return ValueStructs.CopyBufferView(outView);
    }
}
