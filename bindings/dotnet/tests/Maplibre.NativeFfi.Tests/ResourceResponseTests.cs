using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class ResourceResponseTests
{
    [Fact]
    public void ResourceResponseClonesBytesAtBoundary()
    {
        var source = new byte[] { 1, 2, 3 };
        var response = new ResourceResponse { Status = ResourceResponseStatus.Ok, Bytes = source };
        source[0] = 9;
        var copied = response.Bytes;
        copied[1] = 9;

        Assert.Equal([1, 2, 3], response.Bytes);
    }

    [Fact]
    public void NativeResourceResponseCopiesOwnedFields()
    {
        using var scope = new NativeCallScope();
        var value = GeneratedValues.NativeResourceResponse(
            new ResourceResponse
            {
                Status = ResourceResponseStatus.Error,
                ErrorReason = ResourceErrorReason.NotFound,
                Bytes = [1, 2, 3],
                ErrorMessage = "missing",
                MustRevalidate = true,
                ModifiedUnixMs = 1234,
                ExpiresUnixMs = 5678,
                Etag = "abc",
                RetryAfterUnixMs = 9000,
            },
            scope
        );
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
