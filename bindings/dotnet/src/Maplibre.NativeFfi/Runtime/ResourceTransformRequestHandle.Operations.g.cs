// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A resource URL transform that waits for its host's answer.
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
    /// Answers a resource transform request with a replacement URL and consumes
    /// its handle.
    /// </summary>
    /// <remarks>
    /// See <c>mln_resource_transform_request_complete</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public void Complete(string url)
    {
        using var scope = new NativeCallScope();
        var nativeUrl = scope.Utf8(url);
        state.Release(
            (live, diagnostic) =>
                NativeMethods.mln_resource_transform_request_complete(live, nativeUrl, diagnostic),
            idempotent: false
        );
    }
}
