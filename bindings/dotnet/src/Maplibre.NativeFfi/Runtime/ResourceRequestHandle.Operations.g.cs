// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A resource request that a resource provider handles.
/// </summary>
/// <remarks>
/// See <c>mln_resource_request</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class ResourceRequestHandle
    : IDisposable,
        INativeOwner<MlnResourceRequest>
{
    private readonly NativeHandleState<MlnResourceRequest> state;

    internal ResourceRequestHandle(MlnResourceRequest handle)
    {
        state = new(handle, Abandon, nameof(ResourceRequestHandle), Abandon);
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

    // Runtime events report their source by this identity.
    public ulong Id => state.Handle.Value;

    public void Dispose()
    {
        state.Retire();
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
        using var scope = new NativeCallScope(this);
        var nativeResponse = NativeResourceResponse(response, scope);
        Check(NativeMethods.mln_resource_request_complete(Handle, &nativeResponse, Diagnostic));
        scope.Accept();
    }

    /// <summary>
    /// Reports whether MapLibre has cancelled a C API resource provider
    /// request.
    /// </summary>
    /// <remarks>
    /// See <c>mln_resource_request_is_cancelled</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public bool IsCancelled()
    {
        using var call = Enter(this);
        bool outCancelled = default;
        Check(NativeMethods.mln_resource_request_is_cancelled(Handle, &outCancelled, Diagnostic));
        return outCancelled;
    }

    /// <summary>
    /// Hands an outstanding provider request to the built-in network loader.
    /// </summary>
    /// <remarks>
    /// See <c>mln_resource_request_pass_through</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public void PassThrough()
    {
        using var call = Enter(this);
        Check(NativeMethods.mln_resource_request_pass_through(Handle, Diagnostic));
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
        using var scope = new NativeCallScope(this);
        var nativeHandler = NativeResourceRequestCancelHandler(handler, scope);
        bool outCancelled = default;
        Check(
            NativeMethods.mln_resource_request_set_cancel_callback(
                Handle,
                &nativeHandler,
                &outCancelled,
                Diagnostic
            )
        );
        if (!outCancelled)
            scope.Accept();
        return outCancelled;
    }
}
