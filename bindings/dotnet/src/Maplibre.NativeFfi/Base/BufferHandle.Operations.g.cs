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

    public void Close()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_buffer_destroy");
        state.Close();
    }

    public byte[] Get()
    {
        using var read = state.Read(this, "mln_buffer_get");
        var outView = default(mln_buffer_view);
        Check(NativeMethods.mln_buffer_get(read.Handle, &outView, Diagnostic));
        return ValueStructs.CopyBufferView(outView);
    }
}
