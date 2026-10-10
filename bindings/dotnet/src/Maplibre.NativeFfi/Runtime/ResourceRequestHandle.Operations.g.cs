// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public sealed unsafe partial class ResourceRequestHandle
    : IDisposable,
        INativeOwner<MlnResourceRequest>
{
    private readonly NativeHandleState<MlnResourceRequest> state;

    internal ResourceRequestHandle(MlnResourceRequest handle, bool pendingDecision = false)
    {
        state = new(handle, Abandon, nameof(ResourceRequestHandle), Abandon, pendingDecision);
    }

    internal static ResourceRequestHandle Adopt(MlnResourceRequest handle) =>
        NativeHandleState<MlnResourceRequest>.Adopt(
            handle,
            () => new ResourceRequestHandle(handle),
            Abandon
        );

    private static mln_status Abandon(MlnResourceRequest live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_resource_request_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnResourceRequest> INativeOwner<MlnResourceRequest>.State => state;
    internal MlnResourceRequest Handle => state.Handle;
    internal NativeCallbackOwner CallbackOwner => state.CallbackOwner;

    // Runtime events report their source by this identity.
    public ulong Id => state.IssuedHandle.Value;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_resource_request_release");
        state.Retire();
    }

    internal static ResourceRequestHandle BorrowDecision(MlnResourceRequest handle) =>
        new(handle, true);

    internal bool FinishDecision(bool accepted) => state.FinishDecision(accepted);

    public bool Cancelled()
    {
        using var read = state.Read(this, "mln_resource_request_cancelled");
        bool outCancelled = default;
        Check(NativeMethods.mln_resource_request_cancelled(read.Handle, &outCancelled, Diagnostic));
        return outCancelled;
    }

    public void Complete(ResourceResponse response)
    {
        using var scope = new NativeCallScope(this, "mln_resource_request_complete");
        var nativeResponse = NativeResourceResponse(response, scope);
        using var claim = state.BeginClaim();
        Check(NativeMethods.mln_resource_request_complete(Handle, &nativeResponse, Diagnostic));
        scope.Accept();
        claim.Accept();
    }

    public void Close()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_resource_request_release");
        state.Close();
    }

    public bool SetCancelCallback(Action? callback)
    {
        using var read = state.Read(this, "mln_resource_request_set_cancel_callback");
        using var scope = new NativeCallScope();
        bool outCancelled = default;
        Check(
            NativeMethods.mln_resource_request_set_cancel_callback(
                read.Handle,
                callback is null ? null : &InvokeResourceRequestCancelCallback,
                callback is null ? null : scope.Register(new NativeOwnedCallback(callback, this)),
                &NativeCallbackRoot.Release,
                &outCancelled,
                Diagnostic
            )
        );
        if (!outCancelled)
            scope.Accept(CallbackOwner);
        return outCancelled;
    }

    public void WaitUntilRetired()
    {
        using var call = Enter(this, "mln_resource_request_wait_until_retired");
        Check(
            NativeMethods.mln_resource_request_wait_until_retired(state.IssuedHandle, Diagnostic)
        );
    }
}
