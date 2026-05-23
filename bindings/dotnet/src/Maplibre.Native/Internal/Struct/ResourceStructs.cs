using System.Runtime.InteropServices;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Resource;

namespace Maplibre.Native.Internal.Struct;

internal sealed unsafe class NativeResourceResponse : IDisposable
{
    private readonly nint bytes;
    private readonly NativeUtf8String errorMessage;
    private readonly NativeUtf8String etag;

    private NativeResourceResponse(mln_resource_response value, nint bytes, NativeUtf8String errorMessage, NativeUtf8String etag)
    {
        Value = value;
        this.bytes = bytes;
        this.errorMessage = errorMessage;
        this.etag = etag;
    }

    internal mln_resource_response Value { get; }

    internal static NativeResourceResponse From(ResourceResponse response)
    {
        ArgumentNullException.ThrowIfNull(response);

        nint bytesPointer = 0;
        if (response.Bytes.Length > 0)
        {
            bytesPointer = (nint)NativeMemory.Alloc((nuint)response.Bytes.Length);
            Marshal.Copy(response.Bytes, 0, bytesPointer, response.Bytes.Length);
        }

        try
        {
            var errorMessage = NativeUtf8String.FromNullableString(response.ErrorMessage, nameof(response.ErrorMessage));
            try
            {
                var etag = NativeUtf8String.FromNullableString(response.Etag, nameof(response.Etag));
                var value = new mln_resource_response
                {
                    size = (uint)sizeof(mln_resource_response),
                    status = (uint)response.Status,
                    error_reason = (uint)response.ErrorReason,
                    bytes = (byte*)bytesPointer,
                    byte_count = (nuint)response.Bytes.Length,
                    error_message = errorMessage.Pointer,
                    must_revalidate = response.MustRevalidate ? (byte)1 : (byte)0,
                    has_modified = response.Modified.HasValue ? (byte)1 : (byte)0,
                    modified_unix_ms = response.Modified?.ToUnixTimeMilliseconds() ?? 0,
                    has_expires = response.Expires.HasValue ? (byte)1 : (byte)0,
                    expires_unix_ms = response.Expires?.ToUnixTimeMilliseconds() ?? 0,
                    etag = etag.Pointer,
                    has_retry_after = response.RetryAfter.HasValue ? (byte)1 : (byte)0,
                    retry_after_unix_ms = response.RetryAfter?.ToUnixTimeMilliseconds() ?? 0,
                };
                return new NativeResourceResponse(value, bytesPointer, errorMessage, etag);
            }
            catch
            {
                errorMessage.Dispose();
                throw;
            }
        }
        catch
        {
            if (bytesPointer != 0)
            {
                NativeMemory.Free((void*)bytesPointer);
            }
            throw;
        }
    }

    public void Dispose()
    {
        errorMessage.Dispose();
        etag.Dispose();
        if (bytes != 0)
        {
            NativeMemory.Free((void*)bytes);
        }
    }
}
