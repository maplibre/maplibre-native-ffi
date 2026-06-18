using System.Runtime.InteropServices;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Resource;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class ResourceTransformTests
{
    [BindingSpecTest("BND-141")]
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

    [BindingSpecTest("BND-025")]
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

    [BindingSpecTest("BND-121")]
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

    [BindingSpecTest("BND-123")]
    [Fact]
    public void ResourceTransformStateDisposeIsIdempotent()
    {
        var state = new ResourceTransformState(_ => null);

        state.Dispose();
        state.Dispose();
    }

    [BindingSpecTest("BND-122")]
    [Fact]
    public unsafe void ResourceTransformInstallFailurePreservesPreviousCallbackAndReleasesReplacement()
    {
        var failInstall = false;
        ResourceTransformState? failedReplacement = null;
        using var install = RuntimeHandle.UseResourceCallbackInstallMethodsForTest(
            (_, _) => mln_status.MLN_STATUS_OK,
            (_, transform) =>
            {
                if (!failInstall)
                {
                    return mln_status.MLN_STATUS_OK;
                }

                failedReplacement = (ResourceTransformState?)
                    GCHandle.FromIntPtr((nint)transform->user_data).Target;
                return mln_status.MLN_STATUS_INVALID_STATE;
            }
        );
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        runtime.SetResourceTransform(request => request.Url + "?first");
        var previous = Assert.IsType<ResourceTransformState>(runtime.ResourceTransformStateForTest);

        failInstall = true;
        Assert.Throws<InvalidStateException>(() =>
            runtime.SetResourceTransform(request => request.Url + "?second")
        );

        Assert.Same(previous, runtime.ResourceTransformStateForTest);
        Assert.True(previous.IsHandleAllocatedForTest);
        Assert.NotNull(failedReplacement);
        Assert.False(failedReplacement.IsHandleAllocatedForTest);
    }

    [BindingSpecTest("BND-140")]
    [Fact]
    public void CanInstallReplaceAndClearResourceTransform()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());

        runtime.SetResourceTransform(request => request.Url + "?first");
        runtime.SetResourceTransform(request => request.Url + "?second");
        runtime.ClearResourceTransform();
    }
}
