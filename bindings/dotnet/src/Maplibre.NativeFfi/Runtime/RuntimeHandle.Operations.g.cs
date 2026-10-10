// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A runtime: the native scheduler thread and event store for its maps.
/// </summary>
/// <remarks>
/// See <c>mln_runtime</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class RuntimeHandle
    : IDisposable,
        IAsyncDisposable,
        INativeOwner<MlnRuntime>
{
    private readonly NativeHandleState<MlnRuntime> state;
    private volatile Task teardown = Task.CompletedTask;

    internal RuntimeHandle(MlnRuntime handle)
    {
        state = new(handle, StartRelease, nameof(RuntimeHandle), Abandon);
    }

    internal static RuntimeHandle Adopt(MlnRuntime handle) =>
        NativeHandleState<MlnRuntime>.Adopt(handle, () => new RuntimeHandle(handle), Abandon);

    private static mln_status Abandon(MlnRuntime live, mln_diagnostic* diagnostic) =>
        NativeMethods.mln_runtime_dispose(live, diagnostic);

    NativeHandleState<MlnRuntime> INativeOwner<MlnRuntime>.State => state;
    internal MlnRuntime Handle => state.Handle;
    internal NativeCallbackOwner CallbackOwner => state.CallbackOwner;

    // Runtime events report their source by this identity.
    public ulong Id => state.IssuedHandle.Value;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_runtime_dispose");
        state.Retire();
    }

    /// <summary>
    /// Starts an ordered runtime barrier.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_barrier</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task BarrierAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_runtime_barrier");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_barrier(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    /// <summary>
    /// Clears the runtime-scoped outgoing HTTP header transform.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_clear_http_header_transform</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task ClearHttpHeaderTransformAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_runtime_clear_http_header_transform");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_clear_http_header_transform(
                    Handle,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Clears the runtime-scoped network resource provider.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_clear_resource_provider</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task ClearResourceProviderAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_runtime_clear_resource_provider");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_clear_resource_provider(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    /// <summary>
    /// Clears the runtime-scoped URL transform for network resources.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_clear_resource_transform</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task ClearResourceTransformAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_runtime_clear_resource_transform");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_clear_resource_transform(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    /// <summary>
    /// Creates a runtime with a new core-owned worker.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public static RuntimeHandle Create(RuntimeOptions options)
    {
        using var scope = new NativeCallScope(null, "mln_runtime_create");
        var nativeOptions = NativeRuntimeOptions(options, scope);
        MlnRuntime outRuntime = default;
        Check(NativeMethods.mln_runtime_create(&nativeOptions, &outRuntime, Diagnostic));
        var owner = RuntimeHandle.Adopt(outRuntime);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }

    /// <summary>
    /// Creates a map on the runtime worker.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_create_map</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<MapHandle> CreateMapAsync(
        MapOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_create_map");
        return scope.Query<MlnMap, MapHandle>(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_create_map(
                    Handle,
                    scope.Value(NativeMapOptions(options)),
                    completion,
                    diagnostic
                ),
            handle => MapHandle.Adopt(this, handle),
            cancellationToken
        );
    }

    /// <summary>
    /// Starts creating an offline region.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_create_offline_region</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<OfflineRegionInfo> CreateOfflineRegionAsync(
        OfflineRegionDefinition definition,
        byte[] metadata,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_create_offline_region");
        var bufferMetadata = scope.Buffer(metadata);
        return scope.Query<mln_offline_region_info, OfflineRegionInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_create_offline_region(
                    Handle,
                    scope.Value(NativeOfflineRegionDefinition(definition, scope)),
                    (byte*)bufferMetadata.data,
                    checked((nuint)bufferMetadata.size),
                    completion,
                    diagnostic
                ),
            CopyOfflineRegionInfo,
            cancellationToken
        );
    }

    /// <summary>
    /// Deletes an offline region.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_delete_offline_region</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task DeleteOfflineRegionAsync(
        long regionId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_delete_offline_region");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_delete_offline_region(
                    Handle,
                    regionId,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Drains this runtime's queued events into a new owned batch.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_drain_events</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public EventBatchHandle DrainEvents()
    {
        using var call = Enter(this, "mln_runtime_drain_events");
        MlnEventBatch outBatch = default;
        Check(NativeMethods.mln_runtime_drain_events(Handle, &outBatch, Diagnostic));
        return EventBatchHandle.Adopt(outBatch);
    }

    /// <summary>
    /// Reports which runtime-scoped event types this runtime queues.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_get_event_mask</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public RuntimeEventMask GetEventMask()
    {
        using var read = state.Read(this, "mln_runtime_get_event_mask");
        ulong outMask = default;
        Check(NativeMethods.mln_runtime_get_event_mask(read.Handle, &outMask, Diagnostic));
        return (RuntimeEventMask)outMask;
    }

    /// <summary>
    /// Starts getting one offline region by ID.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_get_offline_region</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<OfflineRegionInfo?> GetOfflineRegionAsync(
        long regionId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_get_offline_region");
        return scope.QueryOptionalValue<mln_offline_region_info, OfflineRegionInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_get_offline_region(
                    Handle,
                    regionId,
                    completion,
                    diagnostic
                ),
            CopyOfflineRegionInfo,
            cancellationToken
        );
    }

    /// <summary>
    /// Starts getting the current download status for an offline region.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_get_offline_region_status</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<OfflineRegionStatus> GetOfflineRegionStatusAsync(
        long regionId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_get_offline_region_status");
        return scope.Query<mln_offline_region_status, OfflineRegionStatus>(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_get_offline_region_status(
                    Handle,
                    regionId,
                    completion,
                    diagnostic
                ),
            CopyOfflineRegionStatus,
            cancellationToken
        );
    }

    /// <summary>
    /// Invalidates cached resources for an offline region.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_invalidate_offline_region</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task InvalidateOfflineRegionAsync(
        long regionId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_invalidate_offline_region");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_invalidate_offline_region(
                    Handle,
                    regionId,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Starts listing the offline regions in the runtime database.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_list_offline_regions</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<OfflineRegionInfo[]> ListOfflineRegionsAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_list_offline_regions");
        return scope.QueryArray<mln_offline_region_info, OfflineRegionInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_list_offline_regions(Handle, completion, diagnostic),
            CopyOfflineRegionInfo,
            cancellationToken
        );
    }

    /// <summary>
    /// Starts merging offline regions from another MapLibre offline database.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_merge_offline_regions</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<OfflineRegionInfo[]> MergeOfflineRegionsAsync(
        string sideDatabasePath,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_merge_offline_regions");
        return scope.QueryArray<mln_offline_region_info, OfflineRegionInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_merge_offline_regions(
                    Handle,
                    scope.CStringArgument(sideDatabasePath),
                    completion,
                    diagnostic
                ),
            CopyOfflineRegionInfo,
            cancellationToken
        );
    }

    /// <summary>
    /// Releases a runtime after synchronous child preflight.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task CloseAsync()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_runtime_release");
        state.Close();
        return teardown;
    }

    public ValueTask DisposeAsync() => new(CloseAsync());

    private mln_status StartRelease(MlnRuntime handle, mln_diagnostic* _)
    {
        teardown = NativeCompletion.SubmitUnit(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_release(handle, completion, diagnostic)
        );
        return mln_status.MLN_STATUS_OK;
    }

    /// <summary>
    /// Starts a MapLibre ambient cache maintenance operation for this runtime.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_run_ambient_cache_operation</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task RunAmbientCacheOperationAsync(
        AmbientCacheOperation operation,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_run_ambient_cache_operation");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_run_ambient_cache_operation(
                    Handle,
                    (uint)operation,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Selects which runtime-scoped event types this runtime queues.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_event_mask</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public void SetEventMask(RuntimeEventMask mask)
    {
        using var call = Enter(this, "mln_runtime_set_event_mask");
        Check(NativeMethods.mln_runtime_set_event_mask(Handle, (ulong)mask, Diagnostic));
    }

    /// <summary>
    /// Registers or replaces the runtime-scoped outgoing HTTP header transform.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_http_header_transform</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetHttpHeaderTransformAsync(
        HttpHeaderTransform transform,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_set_http_header_transform");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_set_http_header_transform(
                    Handle,
                    scope.Value(NativeHttpHeaderTransform(transform, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Starts a change to this runtime's maximum ambient cache size.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_maximum_ambient_cache_size</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetMaximumAmbientCacheSizeAsync(
        ulong size,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_set_maximum_ambient_cache_size");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_set_maximum_ambient_cache_size(
                    Handle,
                    size,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Sets an offline region's native download state.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_offline_region_download_state</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetOfflineRegionDownloadStateAsync(
        long regionId,
        OfflineRegionDownloadState state,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(
            this,
            "mln_runtime_set_offline_region_download_state"
        );
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_set_offline_region_download_state(
                    Handle,
                    regionId,
                    (uint)state,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Enables or disables runtime events for an offline region.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_offline_region_observed</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetOfflineRegionObservedAsync(
        long regionId,
        bool observed,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_set_offline_region_observed");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_set_offline_region_observed(
                    Handle,
                    regionId,
                    (byte)(observed ? 1 : 0),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Registers or replaces a runtime-scoped network resource provider.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_resource_provider</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetResourceProviderAsync(
        ResourceProvider provider,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_set_resource_provider");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_set_resource_provider(
                    Handle,
                    scope.Value(NativeResourceProvider(provider, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Registers or updates a runtime-scoped URL transform for network
    /// resources.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_resource_transform</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetResourceTransformAsync(
        ResourceTransform transform,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_set_resource_transform");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_set_resource_transform(
                    Handle,
                    scope.Value(NativeResourceTransform(transform, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Starts updating opaque binary metadata for an offline region.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_update_offline_region_metadata</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<OfflineRegionInfo> UpdateOfflineRegionMetadataAsync(
        long regionId,
        byte[] metadata,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_update_offline_region_metadata");
        var bufferMetadata = scope.Buffer(metadata);
        return scope.Query<mln_offline_region_info, OfflineRegionInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_update_offline_region_metadata(
                    Handle,
                    regionId,
                    (byte*)bufferMetadata.data,
                    checked((nuint)bufferMetadata.size),
                    completion,
                    diagnostic
                ),
            CopyOfflineRegionInfo,
            cancellationToken
        );
    }
}
