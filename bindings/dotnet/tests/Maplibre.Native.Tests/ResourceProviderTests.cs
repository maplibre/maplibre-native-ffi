using System.Text;
using Maplibre.Native.Error;
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
        using var state = new ResourceProviderState(
            (request, handle) =>
            {
                copiedRequest = request;
                Assert.False(handle.IsClosed);
                return ResourceProviderDecision.PassThrough;
            }
        );

        var url = Encoding.UTF8.GetBytes("https://example.test/tile\0");
        var etag = Encoding.UTF8.GetBytes("etag-1\0");
        var priorData = new byte[] { 1, 2, 3 };
        fixed (byte* urlPointer = url)
        fixed (byte* etagPointer = etag)
        fixed (byte* priorDataPointer = priorData)
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
                prior_data = priorDataPointer,
                prior_data_size = (nuint)priorData.Length,
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
        Assert.Equal(10u, copiedRequest.Range?.Start);
        Assert.Equal(20u, copiedRequest.Range?.End);
        Assert.Equal(DateTimeOffset.FromUnixTimeMilliseconds(1234), copiedRequest.PriorModified);
        Assert.Equal(DateTimeOffset.FromUnixTimeMilliseconds(5678), copiedRequest.PriorExpires);
        Assert.Equal("etag-1", copiedRequest.PriorEtag);
        Assert.Equal(3u, copiedRequest.PriorDataSize);
        Assert.Equal([1, 2, 3], copiedRequest.PriorData);
    }

    [Fact]
    public void PassThroughFinalizationClosesRequestHandleBeforeNativeRelease()
    {
        var handle = new ResourceRequestHandle((mln_resource_request_handle*)1234);

        var decision = handle.FinishProviderDecision(ResourceProviderDecision.PassThrough);

        Assert.Equal((uint)ResourceProviderDecision.PassThrough, decision);
        Assert.True(handle.IsClosed);
        var completeError = Assert.Throws<InvalidStateException>(() =>
            handle.Complete(ResourceResponse.NoContent())
        );
        Assert.Equal(MaplibreStatus.InvalidState, completeError.Status);
        var cancelledError = Assert.Throws<InvalidStateException>(() => handle.IsCancelled());
        Assert.Equal(MaplibreStatus.InvalidState, cancelledError.Status);
        handle.Close();
    }

    [Fact]
    public void UnknownProviderDecisionReturnsErrorDecisionPath()
    {
        var handle = new ResourceRequestHandle((mln_resource_request_handle*)1234);

        var decision = handle.FinishProviderDecision((ResourceProviderDecision)999);

        Assert.Equal(uint.MaxValue, decision);
        Assert.True(handle.IsClosed);
    }

    [Fact]
    public void ResourceProviderExceptionReturnsUnknownDecision()
    {
        using var state = new ResourceProviderState(
            (_, _) => throw new InvalidOperationException("boom")
        );
        var url = Encoding.UTF8.GetBytes("https://example.test/style.json\0");
        fixed (byte* urlPointer = url)
        {
            var request = new mln_resource_request { url = (sbyte*)urlPointer };
            Assert.Equal(uint.MaxValue, state.HandleForTest(&request));
        }
    }

    [Fact]
    public void ResourceProviderStateDisposeIsIdempotent()
    {
        var state = new ResourceProviderState((_, _) => ResourceProviderDecision.PassThrough);

        state.Dispose();
        state.Dispose();
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
