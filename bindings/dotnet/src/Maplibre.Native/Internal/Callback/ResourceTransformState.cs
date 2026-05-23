using System.Runtime.InteropServices;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Resource;

namespace Maplibre.Native.Internal.Callback;

internal sealed unsafe class ResourceTransformState : IDisposable
{
    private readonly ResourceTransformCallback callback;
    private readonly GCHandle handle;
    private readonly ThreadLocal<NativeUtf8String?> responseUrls = new(trackAllValues: true);

    internal ResourceTransformState(ResourceTransformCallback callback)
    {
        this.callback = callback ?? throw new ArgumentNullException(nameof(callback));
        handle = GCHandle.Alloc(this);
    }

    internal mln_resource_transform Descriptor => new()
    {
        size = (uint)sizeof(mln_resource_transform),
        callback = &OnTransform,
        user_data = (void*)GCHandle.ToIntPtr(handle),
    };

    internal string? TransformForTest(ResourceKind kind, string url)
    {
        using var nativeUrl = NativeUtf8String.FromNullableString(url, nameof(url));
        mln_resource_transform_response response = default;
        var status = Invoke(this, (uint)kind, nativeUrl.Pointer, &response);
        if (status != mln_status.MLN_STATUS_OK)
        {
            return null;
        }

        return response.url is null ? null : Marshal.PtrToStringUTF8((nint)response.url);
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
    private static mln_status OnTransform(void* userData, uint kind, sbyte* url, mln_resource_transform_response* outResponse)
    {
        try
        {
            var state = (ResourceTransformState?)GCHandle.FromIntPtr((nint)userData).Target;
            return Invoke(state, kind, url, outResponse);
        }
        catch
        {
            return mln_status.MLN_STATUS_NATIVE_ERROR;
        }
    }

    private static mln_status Invoke(ResourceTransformState? state, uint kind, sbyte* url, mln_resource_transform_response* outResponse)
    {
        if (state is null || outResponse is null)
        {
            return mln_status.MLN_STATUS_INVALID_ARGUMENT;
        }

        try
        {
            var requestUrl = url is null ? string.Empty : Marshal.PtrToStringUTF8((nint)url) ?? string.Empty;
            var replacement = state.callback(new ResourceTransformRequest((ResourceKind)kind, requestUrl));

            var previous = state.responseUrls.Value;
            var responseUrl = NativeUtf8String.FromNullableString(replacement, nameof(replacement));
            state.responseUrls.Value = responseUrl;
            previous?.Dispose();

            *outResponse = new mln_resource_transform_response
            {
                size = (uint)sizeof(mln_resource_transform_response),
                url = responseUrl.Pointer,
            };
            return mln_status.MLN_STATUS_OK;
        }
        catch
        {
            return mln_status.MLN_STATUS_NATIVE_ERROR;
        }
    }

    public void Dispose()
    {
        foreach (var responseUrl in responseUrls.Values)
        {
            responseUrl?.Dispose();
        }

        responseUrls.Dispose();
        if (handle.IsAllocated)
        {
            handle.Free();
        }
    }
}
