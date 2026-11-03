// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A resource load that a resource provider answers.
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
    /// Answers a resource request.
    /// </summary>
    /// <remarks>
    /// <para>A failed answer fails the request with
    /// <c>ResourceErrorReason.Other</c>, as
    /// <c>ResourceRequestHandle.Dispose()</c> does.</para>
    /// <para>See <c>mln_resource_request_complete</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidState</c> when the library cancelled
    /// the request.</description></item>
    /// </list>
    /// </remarks>
    public void Complete(ResourceResponse response)
    {
        try
        {
            using var scope = new NativeCallScope();
            var nativeResponse = scope.Value(NativeResourceResponse(response, scope));
            state.Answer(
                (live, diagnostic) =>
                    NativeMethods.mln_resource_request_complete(live, nativeResponse, diagnostic)
            );
        }
        catch
        {
            state.EndAnswer();
            throw;
        }
    }

    /// <summary>
    /// Reads whether the library cancelled a resource request.
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
    /// Hands a resource request back to the library to load as usual.
    /// </summary>
    /// <remarks>
    /// <para>A failed call fails the request with
    /// <c>ResourceErrorReason.Other</c>, as
    /// <c>ResourceRequestHandle.Dispose()</c> does.</para>
    /// <para>See <c>mln_resource_request_pass_through</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidState</c> when the library cancelled
    /// the request.</description></item>
    /// </list>
    /// </remarks>
    public void PassThrough()
    {
        try
        {
            using var scope = new NativeCallScope();
            state.Answer(
                (live, diagnostic) =>
                    NativeMethods.mln_resource_request_pass_through(live, diagnostic)
            );
        }
        catch
        {
            state.EndAnswer();
            throw;
        }
    }

    /// <summary>
    /// Calls a callback when the library cancels a resource request.
    /// </summary>
    /// <remarks>
    /// <para>The callback never runs after the request is answered. A request
    /// takes one cancel callback. When the library has already cancelled the
    /// request, the call reports that instead and the callback never
    /// runs.</para>
    /// <para>See <c>mln_resource_request_set_cancel_callback</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
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
