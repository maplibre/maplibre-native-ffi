// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

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

    public Task<MapHandle> MapCreateAsync(MapOptions options)
    {
        using var scope = new NativeCallScope(this, "mln_map_create");
        return scope.Query<MlnMap, MapHandle>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_create(
                    Handle,
                    scope.Value(NativeMapOptions(options)),
                    completion,
                    diagnostic
                ),
            handle => MapHandle.Adopt(this, handle)
        );
    }

    public Task BarrierAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_runtime_barrier");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_barrier(Handle, completion, diagnostic),
            cancellationToken
        );
    }

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

    public Task ClearResourceProviderAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_runtime_clear_resource_provider");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_clear_resource_provider(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    public Task ClearResourceTransformAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_runtime_clear_resource_transform");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_clear_resource_transform(Handle, completion, diagnostic),
            cancellationToken
        );
    }

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

    public EventBatchHandle DrainEvents()
    {
        using var call = Enter(this, "mln_runtime_drain_events");
        MlnEventBatch outBatch = default;
        Check(NativeMethods.mln_runtime_drain_events(Handle, &outBatch, Diagnostic));
        return EventBatchHandle.Adopt(outBatch);
    }

    public RuntimeEventMask GetEventMask()
    {
        using var read = state.Read(this, "mln_runtime_get_event_mask");
        ulong outMask = default;
        Check(NativeMethods.mln_runtime_get_event_mask(read.Handle, &outMask, Diagnostic));
        return (RuntimeEventMask)outMask;
    }

    public Task<OfflineRegionInfo> OfflineRegionCreateAsync(
        OfflineRegionDefinition definition,
        byte[] metadata,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_offline_region_create");
        var bufferMetadata = scope.Buffer(metadata);
        return scope.Query<mln_offline_region_info, OfflineRegionInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_offline_region_create(
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

    public Task OfflineRegionDeleteAsync(
        long regionId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_offline_region_delete");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_offline_region_delete(
                    Handle,
                    regionId,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<OfflineRegionInfo?> OfflineRegionGetAsync(
        long regionId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_offline_region_get");
        return scope.QueryOptionalValue<mln_offline_region_info, OfflineRegionInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_offline_region_get(
                    Handle,
                    regionId,
                    completion,
                    diagnostic
                ),
            CopyOfflineRegionInfo,
            cancellationToken
        );
    }

    public Task<OfflineRegionStatus> OfflineRegionGetStatusAsync(
        long regionId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_offline_region_get_status");
        return scope.Query<mln_offline_region_status, OfflineRegionStatus>(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_offline_region_get_status(
                    Handle,
                    regionId,
                    completion,
                    diagnostic
                ),
            CopyOfflineRegionStatus,
            cancellationToken
        );
    }

    public Task OfflineRegionInvalidateAsync(
        long regionId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_offline_region_invalidate");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_offline_region_invalidate(
                    Handle,
                    regionId,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task OfflineRegionSetDownloadStateAsync(
        long regionId,
        OfflineRegionDownloadState state,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(
            this,
            "mln_runtime_offline_region_set_download_state"
        );
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_offline_region_set_download_state(
                    Handle,
                    regionId,
                    (uint)state,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task OfflineRegionSetObservedAsync(
        long regionId,
        bool observed,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_offline_region_set_observed");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_offline_region_set_observed(
                    Handle,
                    regionId,
                    (byte)(observed ? 1 : 0),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<OfflineRegionInfo> OfflineRegionUpdateMetadataAsync(
        long regionId,
        byte[] metadata,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_offline_region_update_metadata");
        var bufferMetadata = scope.Buffer(metadata);
        return scope.Query<mln_offline_region_info, OfflineRegionInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_offline_region_update_metadata(
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

    public Task<OfflineRegionInfo[]> OfflineRegionsListAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_offline_regions_list");
        return scope.QueryArray<mln_offline_region_info, OfflineRegionInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_offline_regions_list(Handle, completion, diagnostic),
            CopyOfflineRegionInfo,
            cancellationToken
        );
    }

    public Task<OfflineRegionInfo[]> OfflineRegionsMergeDatabaseAsync(
        string sideDatabasePath,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_runtime_offline_regions_merge_database");
        return scope.QueryArray<mln_offline_region_info, OfflineRegionInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_offline_regions_merge_database(
                    Handle,
                    scope.CStringArgument(sideDatabasePath),
                    completion,
                    diagnostic
                ),
            CopyOfflineRegionInfo,
            cancellationToken
        );
    }

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

    public void SetEventMask(RuntimeEventMask mask)
    {
        using var call = Enter(this, "mln_runtime_set_event_mask");
        Check(NativeMethods.mln_runtime_set_event_mask(Handle, (ulong)mask, Diagnostic));
    }

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
}
