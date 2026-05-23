using System.Text;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Resource;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class ResourceProviderTests
{
    [Fact]
    public void ResourceProviderCopiesRequestAndReturnsDecision()
    {
        ResourceRequest? copiedRequest = null;
        using var state = new ResourceProviderState((request, handle) =>
        {
            copiedRequest = request;
            Assert.False(handle.IsClosed);
            return ResourceProviderDecision.PassThrough;
        });

        var url = Encoding.UTF8.GetBytes("https://example.test/tile\0");
        var etag = Encoding.UTF8.GetBytes("etag-1\0");
        fixed (byte* urlPointer = url)
        fixed (byte* etagPointer = etag)
        {
            var request = new mln_resource_request
            {
                url = (sbyte*)urlPointer,
                kind = (uint)ResourceKind.Tile,
                loading_method = (uint)ResourceLoadingMethod.NetworkOnly,
                priority = (uint)ResourcePriority.Low,
                usage = (uint)ResourceUsage.Offline,
                storage_policy = (uint)ResourceStoragePolicy.Volatile,
                has_range = 1,
                range_start = 10,
                range_end = 20,
                has_prior_modified = 1,
                prior_modified_unix_ms = 1234,
                has_prior_expires = 1,
                prior_expires_unix_ms = 5678,
                prior_etag = (sbyte*)etagPointer,
                prior_data_size = 42,
            };

            var decision = state.HandleForTest(&request);

            Assert.Equal((uint)ResourceProviderDecision.PassThrough, decision);
        }

        Assert.NotNull(copiedRequest);
        Assert.Equal(ResourceKind.Tile, copiedRequest.Kind);
        Assert.Equal("https://example.test/tile", copiedRequest.Url);
        Assert.Equal(ResourceLoadingMethod.NetworkOnly, copiedRequest.LoadingMethod);
        Assert.Equal(ResourcePriority.Low, copiedRequest.Priority);
        Assert.Equal(ResourceUsage.Offline, copiedRequest.Usage);
        Assert.Equal(ResourceStoragePolicy.Volatile, copiedRequest.StoragePolicy);
        Assert.Equal(new ByteRange(10, 20), copiedRequest.Range);
        Assert.Equal(DateTimeOffset.FromUnixTimeMilliseconds(1234), copiedRequest.PriorModified);
        Assert.Equal(DateTimeOffset.FromUnixTimeMilliseconds(5678), copiedRequest.PriorExpires);
        Assert.Equal("etag-1", copiedRequest.PriorEtag);
        Assert.Equal(42u, copiedRequest.PriorDataSize);
    }

    [Fact]
    public void ResourceProviderExceptionReturnsUnknownDecision()
    {
        using var state = new ResourceProviderState((_, _) => throw new InvalidOperationException("boom"));
        var url = Encoding.UTF8.GetBytes("https://example.test/style.json\0");
        fixed (byte* urlPointer = url)
        {
            var request = new mln_resource_request { url = (sbyte*)urlPointer };
            Assert.Equal(uint.MaxValue, state.HandleForTest(&request));
        }
    }

    [Fact]
    public void CanInstallAndReplaceResourceProvider()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();

        runtime.SetResourceProvider((_, _) => ResourceProviderDecision.PassThrough);
        runtime.SetResourceProvider((_, _) => ResourceProviderDecision.PassThrough);
    }
}
