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

namespace Maplibre.NativeFfi.Runtime;

public sealed unsafe partial class ResourceRequestHandle : IDisposable
{
    private readonly NativeHandleState<MlnResourceRequest> state;
    private readonly ulong nativeId;
    public ulong Id => nativeId;

    internal ResourceRequestHandle(MlnResourceRequest handle, bool pendingDecision = false)
    {
        nativeId = handle.Value;
        state = new NativeHandleState<MlnResourceRequest>(
            handle,
            static (live, _) =>
            {
                NativeMethods.mln_resource_request_release(live);
                return mln_status.MLN_STATUS_OK;
            },
            nameof(ResourceRequestHandle),
            static (live, _) =>
            {
                NativeMethods.mln_resource_request_release(live);
                return mln_status.MLN_STATUS_OK;
            },
            pendingDecision
        );
    }

    internal static ResourceRequestHandle Adopt(MlnResourceRequest handle)
    {
        ResourceRequestHandle? owner = null;
        try
        {
            owner = new ResourceRequestHandle(handle);
            return owner;
        }
        catch
        {
            if (owner is null)
                NativeMethods.mln_resource_request_release(handle);
            else
                owner.state.Retire();
            throw;
        }
    }

    internal MlnResourceRequest Handle => state.Handle;

    internal NativeHandleState<MlnResourceRequest>.ReadScope Borrow() => state.Borrow();

    internal global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackOwner CallbackOwner =>
        state.CallbackOwner;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_resource_request_release"
        );
        state.Retire();
    }

    internal static ResourceRequestHandle BorrowDecision(MlnResourceRequest handle) =>
        new(handle, true);

    internal bool FinishDecision(bool accepted) => state.FinishDecision(accepted);

    public bool Cancelled()
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_resource_request_cancelled"
        );
        bool outCancelled = default;
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_resource_request_cancelled(
                read.Handle,
                &outCancelled,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        return outCancelled;
    }

    public void Complete(ResourceResponse response)
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_resource_request_complete"
        );
        var nativeResponse = NativeResourceResponse(response, scope);
        using var claim = state.BeginClaim();
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_resource_request_complete(
                Handle,
                &nativeResponse,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        scope.Accept();
        claim.Accept();
    }

    public void Close()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_resource_request_release"
        );
        state.Close();
    }

    public bool SetCancelCallback(Action? callback)
    {
        using var read = state.Borrow();
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_resource_request_set_cancel_callback"
        );
        var rootCallback = callback is null
            ? null
            : scope.Register(
                new global::Maplibre.NativeFfi.Internal.Callback.NativeOwnedCallback(callback, this)
            );
        bool outCancelled = default;
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_resource_request_set_cancel_callback(
                read.Handle,
                callback is null ? null : &InvokeResourceRequestCancelCallback,
                rootCallback,
                &global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackRoot.Release,
                &outCancelled,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        if (!outCancelled)
            scope.Accept(this.CallbackOwner);
        return outCancelled;
    }

    public void WaitUntilRetired()
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_resource_request_wait_until_retired"
        );
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_resource_request_wait_until_retired(
                state.IssuedHandle,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
    }
}
