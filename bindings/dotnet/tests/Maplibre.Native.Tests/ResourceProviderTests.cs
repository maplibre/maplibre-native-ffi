using System.Runtime.InteropServices;
using System.Text;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Map;
using Maplibre.Native.Resource;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

#pragma warning disable xUnit1031, xUnit1051

public sealed unsafe class ResourceProviderTests
{
    private const string StyleUrl = "https://example.test/style.json";
    private const string StyleJson = "{\"version\":8,\"sources\":{},\"layers\":[]}";

    // Support invariant for resource provider callbacks: request data is copied
    // into language-owned values before user code receives it.
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

    [BindingSpecTest("BND-069", "BND-141")]
    [Fact]
    public void ResourceRequestSnapshotsPriorDataAndReturnsCopies()
    {
        var source = new byte[] { 1, 2, 3 };
        var request = new ResourceRequest(
            ResourceKind.Tile,
            "https://example.test/tile",
            ResourceLoadingMethod.All,
            ResourcePriority.Regular,
            ResourceUsage.Online,
            ResourceStoragePolicy.Permanent,
            null,
            null,
            null,
            null,
            (ulong)source.Length,
            source
        );
        source[0] = 9;

        var first = request.PriorData;
        Assert.Equal([1, 2, 3], first);
        first![0] = 8;
        Assert.Equal([1, 2, 3], request.PriorData);
    }

    [BindingSpecTest("BND-142", "BND-147", "BND-151")]
    [Fact]
    public void PassThroughFinalizationClosesRequestHandleBeforeNativeRelease()
    {
        var handle = new ResourceRequestHandle((mln_resource_request_handle*)1234);

        var decision = handle.FinishProviderDecision(ResourceProviderDecision.PassThrough);

        Assert.Equal((uint)ResourceProviderDecision.PassThrough, decision);
        Assert.True(handle.IsClosed);
        var completeError = Assert.Throws<InvalidStateException>(() =>
            handle.Complete(new ResourceResponse(ResourceResponseStatus.NoContent))
        );
        Assert.Equal(MaplibreStatus.InvalidState, completeError.Status);
        var cancelledError = Assert.Throws<InvalidStateException>(() => handle.IsCancelled());
        Assert.Equal(MaplibreStatus.InvalidState, cancelledError.Status);
        handle.Close();
    }

    [BindingSpecTest("BND-121")]
    [Fact]
    public void UnknownProviderDecisionReturnsErrorDecisionPath()
    {
        var handle = new ResourceRequestHandle((mln_resource_request_handle*)1234);

        var decision = handle.FinishProviderDecision((ResourceProviderDecision)999);

        Assert.Equal(uint.MaxValue, decision);
        Assert.True(handle.IsClosed);
    }

    [BindingSpecTest("BND-146", "BND-152")]
    [Fact]
    public void CompletionThatReachesNativeIsTerminalWhenNativeReturnsError()
    {
        var completeCalls = 0;
        var releaseCalls = 0;
        var handle = new ResourceRequestHandle(
            (mln_resource_request_handle*)1234,
            (_, _) =>
            {
                completeCalls++;
                return mln_status.MLN_STATUS_INVALID_STATE;
            },
            (_, cancelled) =>
            {
                *cancelled = false;
                return mln_status.MLN_STATUS_OK;
            },
            _ => releaseCalls++
        );
        Assert.Equal(
            (uint)ResourceProviderDecision.Handle,
            handle.FinishProviderDecision(ResourceProviderDecision.Handle)
        );

        var error = Assert.Throws<InvalidStateException>(() =>
            handle.Complete(new ResourceResponse(ResourceResponseStatus.NoContent))
        );

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.True(handle.IsClosed);
        Assert.Equal(1, completeCalls);
        Assert.Equal(1, releaseCalls);
        var secondError = Assert.Throws<InvalidStateException>(() =>
            handle.Complete(new ResourceResponse(ResourceResponseStatus.NoContent))
        );
        Assert.Null(secondError.RawStatus);
        Assert.Equal(1, completeCalls);
        Assert.Equal(1, releaseCalls);
    }

    [BindingSpecTest("BND-145", "BND-153")]
    [Fact]
    public void CloseWaitsForInFlightCompletionBeforeNativeRelease()
    {
        using var completionStarted = new ManualResetEventSlim(false);
        using var allowCompletion = new ManualResetEventSlim(false);
        var completeCalls = 0;
        var releaseCalls = 0;
        var handle = new ResourceRequestHandle(
            (mln_resource_request_handle*)1234,
            (_, _) =>
            {
                completeCalls++;
                completionStarted.Set();
                Assert.True(allowCompletion.Wait(TimeSpan.FromSeconds(5)));
                return mln_status.MLN_STATUS_OK;
            },
            (_, cancelled) =>
            {
                *cancelled = false;
                return mln_status.MLN_STATUS_OK;
            },
            _ => releaseCalls++
        );
        Assert.Equal(
            (uint)ResourceProviderDecision.Handle,
            handle.FinishProviderDecision(ResourceProviderDecision.Handle)
        );

        var complete = Task.Run(() =>
            handle.Complete(new ResourceResponse(ResourceResponseStatus.NoContent))
        );
        Assert.True(completionStarted.Wait(TimeSpan.FromSeconds(5)));

        var close = Task.Run(handle.Close);
        Assert.False(close.Wait(TimeSpan.FromMilliseconds(50)));
        Assert.Equal(0, Volatile.Read(ref releaseCalls));

        allowCompletion.Set();
        complete.GetAwaiter().GetResult();
        close.GetAwaiter().GetResult();

        Assert.True(handle.IsClosed);
        Assert.Equal(1, completeCalls);
        Assert.Equal(1, releaseCalls);
    }

    [BindingSpecTest("BND-153")]
    [Fact]
    public void CloseWaitsForInFlightCancellationCheckBeforeNativeRelease()
    {
        using var cancellationStarted = new ManualResetEventSlim(false);
        using var allowCancellation = new ManualResetEventSlim(false);
        var cancelCalls = 0;
        var releaseCalls = 0;
        var handle = new ResourceRequestHandle(
            (mln_resource_request_handle*)1234,
            (_, _) => mln_status.MLN_STATUS_OK,
            (_, cancelled) =>
            {
                cancelCalls++;
                cancellationStarted.Set();
                Assert.True(allowCancellation.Wait(TimeSpan.FromSeconds(5)));
                *cancelled = false;
                return mln_status.MLN_STATUS_OK;
            },
            _ => releaseCalls++
        );
        Assert.Equal(
            (uint)ResourceProviderDecision.Handle,
            handle.FinishProviderDecision(ResourceProviderDecision.Handle)
        );

        var isCancelled = Task.Run(handle.IsCancelled);
        Assert.True(cancellationStarted.Wait(TimeSpan.FromSeconds(5)));

        var close = Task.Run(handle.Close);
        Assert.False(close.Wait(TimeSpan.FromMilliseconds(50)));
        Assert.Equal(0, Volatile.Read(ref releaseCalls));

        allowCancellation.Set();
        Assert.False(isCancelled.GetAwaiter().GetResult());
        close.GetAwaiter().GetResult();

        Assert.True(handle.IsClosed);
        Assert.Equal(1, cancelCalls);
        Assert.Equal(1, releaseCalls);
    }

    [BindingSpecTest("BND-121")]
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

    [BindingSpecTest("BND-123")]
    [Fact]
    public void ResourceProviderStateDisposeIsIdempotent()
    {
        var state = new ResourceProviderState((_, _) => ResourceProviderDecision.PassThrough);

        state.Dispose();
        state.Dispose();
    }

    [BindingSpecTest("BND-122")]
    [Fact]
    public void ResourceProviderInstallFailurePreservesPreviousCallbackAndReleasesReplacement()
    {
        var failInstall = false;
        ResourceProviderState? failedReplacement = null;
        using var install = RuntimeHandle.UseResourceCallbackInstallMethodsForTest(
            (_, provider) =>
            {
                if (!failInstall)
                {
                    return mln_status.MLN_STATUS_OK;
                }

                failedReplacement = (ResourceProviderState?)
                    GCHandle.FromIntPtr((nint)provider->user_data).Target;
                return mln_status.MLN_STATUS_INVALID_STATE;
            },
            (_, _) => mln_status.MLN_STATUS_OK
        );
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        runtime.SetResourceProvider((_, _) => ResourceProviderDecision.PassThrough);
        var previous = Assert.IsType<ResourceProviderState>(runtime.ResourceProviderStateForTest);

        failInstall = true;
        Assert.Throws<InvalidStateException>(() =>
            runtime.SetResourceProvider((_, _) => ResourceProviderDecision.Handle)
        );

        Assert.Same(previous, runtime.ResourceProviderStateForTest);
        Assert.True(previous.IsHandleAllocatedForTest);
        Assert.NotNull(failedReplacement);
        Assert.False(failedReplacement.IsHandleAllocatedForTest);
    }

    [BindingSpecTest("BND-122")]
    [Fact]
    public void CanInstallAndReplaceResourceProvider()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());

        runtime.SetResourceProvider((_, _) => ResourceProviderDecision.PassThrough);
        runtime.SetResourceProvider((_, _) => ResourceProviderDecision.PassThrough);
    }

    [BindingSpecTest("BND-101", "BND-143", "BND-150")]
    [Fact]
    public void InlineHandledResourceProviderCompletionLoadsStyleAndClosesRequest()
    {
        ResourceRequestHandle? handled = null;
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        runtime.SetResourceProvider(
            (request, handle) =>
            {
                Assert.Equal(StyleUrl, request.Url);
                handled = handle;
                handle.Complete(StyleResponse());
                return ResourceProviderDecision.Handle;
            }
        );
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        map.SetStyleUrl(StyleUrl);
        var runtimeEvent = RuntimeEventTestHelpers.WaitForMapEvent(
            runtime,
            map,
            RuntimeEventType.MapStyleLoaded
        );

        Assert.Same(map, runtimeEvent.MapSource);
        Assert.NotNull(handled);
        Assert.True(handled.IsClosed);
    }

    [BindingSpecTest("BND-101", "BND-144")]
    [Fact]
    public void LaterHandledResourceProviderCompletionLoadsStyle()
    {
        using var providerCalled = new ManualResetEventSlim(false);
        ResourceRequestHandle? handled = null;
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        runtime.SetResourceProvider(
            (request, handle) =>
            {
                Assert.Equal(StyleUrl, request.Url);
                handled = handle;
                providerCalled.Set();
                return ResourceProviderDecision.Handle;
            }
        );
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        map.SetStyleUrl(StyleUrl);
        DriveRuntimeUntil(runtime, providerCalled);
        Assert.NotNull(handled);
        handled.Complete(StyleResponse());

        var runtimeEvent = RuntimeEventTestHelpers.WaitForMapEvent(
            runtime,
            map,
            RuntimeEventType.MapStyleLoaded
        );

        Assert.Same(map, runtimeEvent.MapSource);
        Assert.True(handled.IsClosed);
    }

    [BindingSpecTest("BND-148")]
    [Fact]
    public void CancelledResourceRequestReportsCancellationBeforeLateCompletionStatus()
    {
        using var providerCalled = new ManualResetEventSlim(false);
        ResourceRequestHandle? handled = null;
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        runtime.SetResourceProvider(
            (_, handle) =>
            {
                handled = handle;
                providerCalled.Set();
                return ResourceProviderDecision.Handle;
            }
        );
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        map.SetStyleUrl(StyleUrl);
        DriveRuntimeUntil(runtime, providerCalled);
        Assert.NotNull(handled);

        map.SetStyleJson(StyleJson);
        DriveRuntimeUntilCancelled(runtime, handled);

        var error = Assert.Throws<InvalidStateException>(() => handled.Complete(StyleResponse()));

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.NotNull(error.RawStatus);
        Assert.True(handled.IsClosed);
    }

    [BindingSpecTest("BND-149")]
    [Fact]
    public void ResourceProviderErrorResponseProducesCopiedLoadingFailureEvent()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        runtime.SetResourceProvider(
            (_, handle) =>
            {
                handle.Complete(
                    new ResourceResponse(ResourceResponseStatus.Error)
                    {
                        ErrorReason = ResourceErrorReason.NotFound,
                        ErrorMessage = "style missing",
                    }
                );
                return ResourceProviderDecision.Handle;
            }
        );
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        map.SetStyleUrl(StyleUrl);
        var runtimeEvent = RuntimeEventTestHelpers.WaitForMapEvent(
            runtime,
            map,
            RuntimeEventType.MapLoadingFailed
        );

        Assert.Same(map, runtimeEvent.MapSource);
        Assert.Contains("style", runtimeEvent.Message, StringComparison.OrdinalIgnoreCase);
    }

    private static ResourceResponse StyleResponse() =>
        new(ResourceResponseStatus.Ok) { Bytes = Encoding.UTF8.GetBytes(StyleJson) };

    private static void DriveRuntimeUntil(RuntimeHandle runtime, ManualResetEventSlim signal)
    {
        for (var attempt = 0; attempt < 1000; attempt++)
        {
            runtime.RunOnce();
            if (signal.IsSet)
            {
                return;
            }

            Thread.Sleep(1);
        }

        Assert.True(signal.IsSet);
    }

    private static void DriveRuntimeUntilCancelled(
        RuntimeHandle runtime,
        ResourceRequestHandle handle
    )
    {
        for (var attempt = 0; attempt < 1000; attempt++)
        {
            runtime.RunOnce();
            if (handle.IsCancelled())
            {
                return;
            }

            Thread.Sleep(1);
        }

        Assert.True(handle.IsCancelled());
    }
}

#pragma warning restore xUnit1031, xUnit1051
