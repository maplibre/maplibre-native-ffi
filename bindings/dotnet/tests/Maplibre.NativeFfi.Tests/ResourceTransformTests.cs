using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Resource;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

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
    public unsafe void ResourceTransformInstallFailureReleasesReplacement()
    {
        var failInstall = false;
        ResourceTransformState? failedReplacement = null;
        using var install = RuntimeHandle.UseResourceCallbackInstallMethodsForTest(
            (_, _, _) => mln_status.MLN_STATUS_OK,
            (_, transform, _) =>
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
        runtime.SetResourceTransformAsync(
            request => request.Url + "?first",
            TestContext.Current.CancellationToken
        );

        failInstall = true;
        Assert.Throws<InvalidStateException>(() =>
            runtime
                .SetResourceTransformAsync(
                    request => request.Url + "?second",
                    TestContext.Current.CancellationToken
                )
                .GetAwaiter()
                .GetResult()
        );

        Assert.NotNull(failedReplacement);
        Assert.False(failedReplacement.IsHandleAllocatedForTest);
    }

    [BindingSpecTest("BND-140")]
    [Fact]
    public async Task InstallReplaceAndClearOfTheResourceTransformEachCommit()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());

        RuntimeEventTestHelpers.AssertRuntimeCommitted(
            runtime.SetResourceTransformAsync(
                request => request.Url + "?first",
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertRuntimeCommitted(
            runtime.SetResourceTransformAsync(
                request => request.Url + "?second",
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertRuntimeCommitted(
            runtime.ClearResourceTransformAsync(TestContext.Current.CancellationToken)
        );

        await runtime.BarrierAsync(TestContext.Current.CancellationToken);
    }
}
