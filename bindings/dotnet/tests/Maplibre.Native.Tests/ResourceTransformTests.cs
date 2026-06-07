using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Resource;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class ResourceTransformTests
{
    [Fact]
    public void ResourceTransformCopiesRequestWhenKeepingOriginalUrl()
    {
        ResourceTransformRequest? copiedRequest = null;
        using var state = new ResourceTransformState(request =>
        {
            copiedRequest = request;
            return null;
        });

        Assert.Equal(
            mln_status.MLN_STATUS_OK,
            state.TransformForTest(
                ResourceKind.Tile,
                "https://example.test/tile",
                out var replacementUrl
            )
        );
        Assert.Null(replacementUrl);
        Assert.Equal(ResourceKind.Tile, copiedRequest?.Kind);
        Assert.Equal("https://example.test/tile", copiedRequest?.Url);
    }

    [Fact]
    public void ResourceTransformEmbeddedNulReplacementMapsToInvalidArgument()
    {
        using var state = new ResourceTransformState(_ => "https://example.test/\0truncated");

        Assert.Equal(
            mln_status.MLN_STATUS_INVALID_ARGUMENT,
            state.TransformForTest(
                ResourceKind.Style,
                "https://example.test/style.json",
                out var replacementUrl
            )
        );
        Assert.Null(replacementUrl);
    }

    [Fact]
    public void ResourceTransformExceptionMapsToNativeError()
    {
        using var state = new ResourceTransformState(_ =>
            throw new InvalidOperationException("boom")
        );

        Assert.Equal(
            mln_status.MLN_STATUS_NATIVE_ERROR,
            state.TransformForTest(
                ResourceKind.Style,
                "https://example.test/style.json",
                out var replacementUrl
            )
        );
        Assert.Null(replacementUrl);
    }

    [Fact]
    public void ResourceTransformStateDisposeIsIdempotent()
    {
        var state = new ResourceTransformState(_ => null);

        state.Dispose();
        state.Dispose();
    }

    [Fact]
    public unsafe void ResourceTransformResponseHelperRequiresNativeCallbackContext()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        var response = new mln_resource_transform_response
        {
            size = (uint)sizeof(mln_resource_transform_response),
            context = null,
        };
        using var replacementUrl = NativeUtf8String.FromNullableString(
            "https://example.test/style.json",
            "replacementUrl"
        );

        Assert.Equal(
            mln_status.MLN_STATUS_INVALID_STATE,
            NativeMethods.mln_resource_transform_response_set_url(
                &response,
                replacementUrl.Pointer,
                replacementUrl.ByteLength
            )
        );
        Assert.Equal(nint.Zero, (nint)response.url);
    }

    [Fact]
    public void CanInstallReplaceAndClearResourceTransform()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();

        runtime.SetResourceTransform(request => request.Url + "?first");
        runtime.SetResourceTransform(request => request.Url + "?second");
        runtime.ClearResourceTransform();
    }
}
