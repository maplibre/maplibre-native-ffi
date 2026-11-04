// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A URL rewrite that a resource transform answers with the URL to load.
/// </summary>
/// <remarks>
/// See <c>mln_resource_transform_request</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class ResourceTransformRequestHandle
    : IDisposable,
        INativeOwner<MlnResourceTransformRequest>
{
    private readonly NativeHandleState<MlnResourceTransformRequest> state;

    internal ResourceTransformRequestHandle(MlnResourceTransformRequest handle)
    {
        state = new(handle, Abandon, nameof(ResourceTransformRequestHandle), Abandon);
    }

    internal static ResourceTransformRequestHandle Adopt(MlnResourceTransformRequest handle) =>
        NativeHandleState<MlnResourceTransformRequest>.Adopt(
            handle,
            () => new ResourceTransformRequestHandle(handle),
            Abandon
        );

    private static mln_status Abandon(MlnResourceTransformRequest live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_resource_transform_request_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnResourceTransformRequest> INativeOwner<MlnResourceTransformRequest>.State =>
        state;
    internal MlnResourceTransformRequest Handle => state.Handle;

    /// <summary>
    /// The native handle value. It stays readable after release.
    /// </summary>
    public ulong Id => state.Handle.Value;

    /// <summary>
    /// Whether a release, dispose, or discard has ended the handle.
    /// </summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>
    /// Names the handle's type, its <see cref="Id" />, and whether it is closed.
    /// </summary>
    public override string ToString() => state.ToString();

    void IDisposable.Dispose()
    {
        state.Close();
    }

    /// <summary>
    /// Answers a resource transform request with the URL to load instead.
    /// </summary>
    /// <remarks>
    /// <para><c>url</c> must not be empty. A failed answer fails the request
    /// with <c>ResourceErrorReason.Other</c>, as
    /// <c>ResourceTransformRequestHandle.Release()</c> does.</para>
    /// <para>See <c>mln_resource_transform_request_complete</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// </remarks>
    public void Complete(string url)
    {
        try
        {
            using var scope = new NativeCallScope();
            var nativeUrl = scope.Utf8(url);
            state.Answer(
                (live, diagnostic) =>
                    NativeMethods.mln_resource_transform_request_complete(
                        live,
                        nativeUrl,
                        diagnostic
                    )
            );
        }
        catch
        {
            state.EndAnswer();
            throw;
        }
    }

    /// <summary>
    /// Fails a resource transform request without sending it.
    /// </summary>
    /// <remarks>
    /// <para>The library retries a request that failed for a reason it retries,
    /// such as <c>ResourceErrorReason.Connection</c>, and the retry calls the
    /// transform again. When this call fails, the request fails with
    /// <c>ResourceErrorReason.Other</c>, as
    /// <c>ResourceTransformRequestHandle.Release()</c> does.</para>
    /// <para>See <c>mln_resource_transform_request_fail</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// </remarks>
    public void Fail(ResourceTransformError error)
    {
        try
        {
            using var scope = new NativeCallScope();
            var nativeError = scope.Value(NativeResourceTransformError(error, scope));
            state.Answer(
                (live, diagnostic) =>
                    NativeMethods.mln_resource_transform_request_fail(live, nativeError, diagnostic)
            );
        }
        catch
        {
            state.EndAnswer();
            throw;
        }
    }

    /// <summary>
    /// Releases a resource transform request.
    /// </summary>
    /// <remarks>
    /// <para>Releasing a request without an answer fails it with
    /// <c>ResourceErrorReason.Other</c>.</para>
    /// <para>See <c>mln_resource_transform_request_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// </remarks>
    public void Release()
    {
        state.Close();
    }
}
