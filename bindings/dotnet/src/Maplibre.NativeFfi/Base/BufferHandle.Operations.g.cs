// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi.Base;

public sealed unsafe partial class BufferHandle : IDisposable
{
    private readonly NativeHandleState<MlnBuffer> state;
    private readonly ulong nativeId;
    public ulong Id => nativeId;

    internal BufferHandle(MlnBuffer handle)
    {
        nativeId = handle.Value;
        state = new NativeHandleState<MlnBuffer>(
            handle,
            static (live, _) =>
            {
                NativeMethods.mln_buffer_destroy(live);
                return mln_status.MLN_STATUS_OK;
            },
            nameof(BufferHandle),
            static (live, _) =>
            {
                NativeMethods.mln_buffer_destroy(live);
                return mln_status.MLN_STATUS_OK;
            }
        );
    }

    internal static BufferHandle Adopt(MlnBuffer handle)
    {
        BufferHandle? owner = null;
        try
        {
            owner = new BufferHandle(handle);
            return owner;
        }
        catch
        {
            if (owner is null)
                NativeMethods.mln_buffer_destroy(handle);
            else
                owner.state.Retire();
            throw;
        }
    }

    internal MlnBuffer Handle => state.Handle;

    internal NativeHandleState<MlnBuffer>.ReadScope Borrow() => state.Borrow();

    internal global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackOwner CallbackOwner =>
        state.CallbackOwner;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_buffer_destroy"
        );
        state.Retire();
    }

    public void Close()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_buffer_destroy"
        );
        state.Close();
    }

    public byte[] Get()
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_buffer_get"
        );
        var outView = new mln_buffer_view { size = (uint)sizeof(mln_buffer_view) };
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_buffer_get(
                read.Handle,
                &outView,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        return ValueStructs.CopyBufferView(outView);
    }
}
