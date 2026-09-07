using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Resource;

namespace Maplibre.NativeFfi.Internal.Callback;

internal sealed unsafe class HttpHeaderTransformState : IDisposable
{
    private readonly HttpHeaderTransformCallback callback;
    private nint handle;

    internal HttpHeaderTransformState(HttpHeaderTransformCallback callback)
    {
        this.callback = callback ?? throw new ArgumentNullException(nameof(callback));
        handle = GCHandle.ToIntPtr(GCHandle.Alloc(this));
    }

    internal mln_http_header_transform Descriptor =>
        new()
        {
            size = (uint)sizeof(mln_http_header_transform),
            callback = &OnTransform,
            user_data = (void*)handle,
            release_user_data = &Release,
        };

    [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
    private static mln_status OnTransform(
        void* userData,
        uint kind,
        sbyte* url,
        mln_http_header_transform_response* outResponse
    )
    {
        try
        {
            var state = (HttpHeaderTransformState?)GCHandle.FromIntPtr((nint)userData).Target;
            if (state is null || outResponse is null || url is null)
            {
                return mln_status.MLN_STATUS_INVALID_ARGUMENT;
            }
            outResponse->size = (uint)sizeof(mln_http_header_transform_response);
            var request = new HttpHeaderTransformRequest(
                (ResourceKind)kind,
                Marshal.PtrToStringUTF8((nint)url) ?? string.Empty
            );
            var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var header in state.callback(request) ?? [])
            {
                if (!names.Add(header.Name))
                {
                    return mln_status.MLN_STATUS_INVALID_ARGUMENT;
                }
                using var name = NativeUtf8String.FromNullableString(
                    header.Name,
                    nameof(header.Name)
                );
                using var value = NativeUtf8String.FromNullableString(
                    header.Value,
                    nameof(header.Value)
                );
                var status = NativeMethods.mln_http_header_transform_response_set(
                    outResponse,
                    name.Pointer,
                    name.ByteLength,
                    value.Pointer,
                    value.ByteLength
                );
                if (status != mln_status.MLN_STATUS_OK)
                {
                    return status;
                }
            }
            return mln_status.MLN_STATUS_OK;
        }
        catch (InvalidArgumentException)
        {
            return mln_status.MLN_STATUS_INVALID_ARGUMENT;
        }
        catch (ArgumentException)
        {
            return mln_status.MLN_STATUS_INVALID_ARGUMENT;
        }
        catch
        {
            return mln_status.MLN_STATUS_NATIVE_ERROR;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
    private static void Release(void* userData) =>
        CallbackRelease.Dispose<HttpHeaderTransformState>(userData);

    public void Dispose()
    {
        var current = System.Threading.Interlocked.Exchange(ref handle, 0);
        if (current != 0)
        {
            GCHandle.FromIntPtr(current).Free();
        }
    }
}
