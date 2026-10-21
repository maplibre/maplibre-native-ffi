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

    // Runtime events report their source by this identity.
    public ulong Id => state.Handle.Value;

    public void Dispose()
    {
        state.Retire();
    }

    /// <summary>
    /// Answers a resource transform request with a replacement URL.
    /// </summary>
    /// <remarks>
    /// See <c>mln_resource_transform_request_complete</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public void Complete(string url)
    {
        using var scope = new NativeCallScope(this);
        Check(
            NativeMethods.mln_resource_transform_request_complete(
                Handle,
                scope.Utf8(url),
                Diagnostic
            )
        );
        scope.Accept();
    }

    /// <summary>
    /// Releases a resource transform request.
    /// </summary>
    /// <remarks>
    /// See <c>mln_resource_transform_request_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public void Close()
    {
        state.Close();
    }
}
