// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A resource request that a resource provider handles.
/// </summary>
/// <remarks>
/// See <c>mln_resource_request_handle</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
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

    /// <summary>
    /// Reports whether MapLibre has cancelled a C API resource provider
    /// request.
    /// </summary>
    /// <remarks>
    /// See <c>mln_resource_request_cancelled</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public bool Cancelled()
    {
        using var read = state.Read(this, "mln_resource_request_cancelled");
        bool outCancelled = default;
        Check(NativeMethods.mln_resource_request_cancelled(read.Handle, &outCancelled, Diagnostic));
        return outCancelled;
    }

    /// <summary>
    /// Completes a C API resource provider request.
    /// </summary>
    /// <remarks>
    /// See <c>mln_resource_request_complete</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public void Complete(ResourceResponse response)
    {
        using var scope = new NativeCallScope(this, "mln_resource_request_complete");
        var nativeResponse = NativeResourceResponse(response, scope);
        using var claim = state.BeginClaim();
        Check(NativeMethods.mln_resource_request_complete(Handle, &nativeResponse, Diagnostic));
        scope.Accept();
        claim.Accept();
    }

    /// <summary>
    /// Releases the provider's reference to a resource request handle.
    /// </summary>
    /// <remarks>
    /// See <c>mln_resource_request_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public void Close()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_resource_request_release");
        state.Close();
    }

    /// <summary>
    /// Registers a callback that runs when MapLibre cancels a C API resource
    /// provider request.
    /// </summary>
    /// <remarks>
    /// See <c>mln_resource_request_set_cancel_callback</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public bool SetCancelCallback(ResourceRequestCancelHandler handler)
    {
        using var read = state.Read(this, "mln_resource_request_set_cancel_callback");
        using var scope = new NativeCallScope() { Receiver = this };
        var nativeHandler = NativeResourceRequestCancelHandler(handler, scope);
        bool outCancelled = default;
        Check(
            NativeMethods.mln_resource_request_set_cancel_callback(
                read.Handle,
                &nativeHandler,
                &outCancelled,
                Diagnostic
            )
        );
        if (!outCancelled)
            scope.Accept(CallbackOwner);
        return outCancelled;
    }

    /// <summary>
    /// Blocks until a resource request is released and its cancel callback
    /// registration has retired: the callback, if it ran, and release_user_data
    /// have both returned. Completing a request does not release its owner.
    /// </summary>
    /// <remarks>
    /// See <c>mln_resource_request_wait_until_retired</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public void WaitUntilRetired()
    {
        using var call = Enter(this, "mln_resource_request_wait_until_retired");
        Check(
            NativeMethods.mln_resource_request_wait_until_retired(state.IssuedHandle, Diagnostic)
        );
    }
}
