using System.Runtime.InteropServices;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Resource;

namespace Maplibre.Native.Internal.Callback;

internal sealed unsafe class ResourceTransformState : IDisposable
{
    private readonly ResourceTransformCallback callback;
    private nint handle;

    internal ResourceTransformState(ResourceTransformCallback callback)
    {
        this.callback = callback ?? throw new ArgumentNullException(nameof(callback));
        handle = GCHandle.ToIntPtr(GCHandle.Alloc(this));
    }

    internal mln_resource_transform Descriptor =>
        new()
        {
            size = (uint)sizeof(mln_resource_transform),
            callback = &OnTransform,
            user_data = (void*)handle,
        };

    internal mln_status TransformForTest(ResourceKind kind, string? url, out string? replacementUrl)
    {
        using var nativeUrl = NativeUtf8String.FromNullableString(url, nameof(url));
        mln_resource_transform_response response = new()
        {
            size = (uint)sizeof(mln_resource_transform_response),
        };
        var status = Invoke(this, (uint)kind, nativeUrl.Pointer, &response);
        replacementUrl = response.url is null ? null : Marshal.PtrToStringUTF8((nint)response.url);
        return status;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
    private static mln_status OnTransform(
        void* userData,
        uint kind,
        sbyte* url,
        mln_resource_transform_response* outResponse
    )
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

    private static mln_status Invoke(
        ResourceTransformState? state,
        uint kind,
        sbyte* url,
        mln_resource_transform_response* outResponse
    )
    {
        if (state is null || outResponse is null)
        {
            return mln_status.MLN_STATUS_INVALID_ARGUMENT;
        }

        try
        {
            var requestUrl = url is null
                ? string.Empty
                : Marshal.PtrToStringUTF8((nint)url) ?? string.Empty;
            var replacement = state.callback(
                new ResourceTransformRequest((ResourceKind)kind, requestUrl)
            );

            outResponse->size = (uint)sizeof(mln_resource_transform_response);
            outResponse->url = null;
            if (string.IsNullOrEmpty(replacement))
            {
                return mln_status.MLN_STATUS_OK;
            }

            using var responseUrl = NativeUtf8String.FromNullableString(
                replacement,
                nameof(replacement)
            );
            return NativeMethods.mln_resource_transform_response_set_url(
                outResponse,
                responseUrl.Pointer,
                responseUrl.ByteLength
            );
        }
        catch (InvalidArgumentException)
        {
            return mln_status.MLN_STATUS_INVALID_ARGUMENT;
        }
        catch (ArgumentException)
        {
            return mln_status.MLN_STATUS_INVALID_ARGUMENT;
        }
        catch (OverflowException)
        {
            return mln_status.MLN_STATUS_INVALID_ARGUMENT;
        }
        catch (OutOfMemoryException)
        {
            return mln_status.MLN_STATUS_NATIVE_ERROR;
        }
        catch (Exception)
        {
            return mln_status.MLN_STATUS_NATIVE_ERROR;
        }
    }

    public void Dispose()
    {
        var current = System.Threading.Interlocked.Exchange(ref handle, 0);
        if (current != 0)
        {
            GCHandle.FromIntPtr(current).Free();
        }
    }
}
