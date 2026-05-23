using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Resource;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class ResourceTransformTests
{
    [Fact]
    public void ResourceTransformCopiesRequestAndKeepsReplacementUrlAlive()
    {
        using var state = new ResourceTransformState(request =>
        {
            Assert.Equal(ResourceKind.Tile, request.Kind);
            Assert.Equal("https://example.test/tile", request.Url);
            return request.Url + "?token=abc";
        });

        Assert.Equal("https://example.test/tile?token=abc", state.TransformForTest(ResourceKind.Tile, "https://example.test/tile"));
    }

    [Fact]
    public void ResourceTransformExceptionMapsToNoRewriteForNativeCallback()
    {
        using var state = new ResourceTransformState(_ => throw new InvalidOperationException("boom"));

        Assert.Null(state.TransformForTest(ResourceKind.Style, "https://example.test/style.json"));
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
