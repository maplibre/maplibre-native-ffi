using System.Runtime.InteropServices;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Resource;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class ResourceResponseTests
{
    [Fact]
    public void ResourceResponseClonesBytesAtBoundary()
    {
        var source = new byte[] { 1, 2, 3 };
        var response = ResourceResponse.Ok(source);
        source[0] = 9;
        var copied = response.Bytes;
        copied[1] = 9;

        Assert.Equal([1, 2, 3], response.Bytes);
    }

    [Fact]
    public void NativeResourceResponseCopiesOwnedFields()
    {
        var modified = DateTimeOffset.FromUnixTimeMilliseconds(1234);
        var expires = DateTimeOffset.FromUnixTimeMilliseconds(5678);
        var retryAfter = DateTimeOffset.FromUnixTimeMilliseconds(9000);
        using var native = NativeResourceResponse.From(
            new ResourceResponse(ResourceResponseStatus.Error)
            {
                ErrorReason = ResourceErrorReason.NotFound,
                Bytes = [1, 2, 3],
                ErrorMessage = "missing",
                MustRevalidate = true,
                Modified = modified,
                Expires = expires,
                Etag = "abc",
                RetryAfter = retryAfter,
            }
        );

        var value = native.Value;
        Assert.Equal((uint)ResourceResponseStatus.Error, value.status);
        Assert.Equal((uint)ResourceErrorReason.NotFound, value.error_reason);
        Assert.Equal(3u, value.byte_count);
        Assert.Equal(1, value.bytes[0]);
        Assert.Equal("missing", Marshal.PtrToStringUTF8((nint)value.error_message));
        Assert.Equal(1, value.must_revalidate);
        Assert.Equal(1, value.has_modified);
        Assert.Equal(1234, value.modified_unix_ms);
        Assert.Equal(1, value.has_expires);
        Assert.Equal(5678, value.expires_unix_ms);
        Assert.Equal("abc", Marshal.PtrToStringUTF8((nint)value.etag));
        Assert.Equal(1, value.has_retry_after);
        Assert.Equal(9000, value.retry_after_unix_ms);
    }
}
