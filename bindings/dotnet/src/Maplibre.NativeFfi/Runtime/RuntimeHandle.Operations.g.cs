// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi.Runtime;

public sealed unsafe partial class RuntimeHandle : IDisposable, IAsyncDisposable
{
    private readonly NativeHandleState<MlnRuntime> state;
    private volatile Task teardown = Task.CompletedTask;
    private readonly ulong nativeId;
    public ulong Id => nativeId;

    internal RuntimeHandle(MlnRuntime handle)
    {
        nativeId = handle.Value;
        state = new NativeHandleState<MlnRuntime>(
            handle,
            StartRelease,
            nameof(RuntimeHandle),
            static (live, diagnostic) => NativeMethods.mln_runtime_dispose(live, diagnostic)
        );
    }

    internal static RuntimeHandle Adopt(MlnRuntime handle)
    {
        RuntimeHandle? owner = null;
        try
        {
            owner = new RuntimeHandle(handle);
            return owner;
        }
        catch
        {
            if (owner is null)
                NativeMethods.mln_runtime_dispose(handle, null);
            else
                owner.state.Retire();
            throw;
        }
    }

    internal MlnRuntime Handle => state.Handle;

    internal NativeHandleState<MlnRuntime>.ReadScope Borrow() => state.Borrow();

    internal global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackOwner CallbackOwner =>
        state.CallbackOwner;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_dispose"
        );
        state.Retire();
    }

    public Task<MapHandle> MapCreateAsync(MapOptions options)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_create"
        );
        return NativeCompletion.Submit(
            (completion, diagnostic) =>
            {
                var nativeOptions = NativeMapOptions(options);
                return NativeMethods.mln_map_create(Handle, &nativeOptions, completion, diagnostic);
            },
            result => MapHandle.Adopt(this, NativeCompletion.Value<MlnMap>(result))
        );
    }

    public Task BarrierAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_barrier"
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_barrier(Handle, completion, diagnostic),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task ClearHttpHeaderTransformAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_clear_http_header_transform"
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_clear_http_header_transform(
                        Handle,
                        completion,
                        diagnostic
                    ),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task ClearResourceProviderAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_clear_resource_provider"
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_clear_resource_provider(
                        Handle,
                        completion,
                        diagnostic
                    ),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task ClearResourceTransformAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_clear_resource_transform"
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_clear_resource_transform(
                        Handle,
                        completion,
                        diagnostic
                    ),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public static RuntimeHandle Create(RuntimeOptions options)
    {
        using var scope = new NativeCallScope();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_runtime_create"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        var nativeOptions = NativeRuntimeOptions(options, scope);
        MlnRuntime outRuntime = default;
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_runtime_create(
                &nativeOptions,
                &outRuntime,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        var owner = RuntimeHandle.Adopt(outRuntime);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }

    public EventBatchHandle DrainEvents()
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_drain_events"
        );
        MlnEventBatch outBatch = default;
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_runtime_drain_events(
                Handle,
                &outBatch,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        return EventBatchHandle.Adopt(outBatch);
    }

    public RuntimeEventMask GetEventMask()
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_get_event_mask"
        );
        ulong outMask = default;
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_runtime_get_event_mask(
                read.Handle,
                &outMask,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        return (RuntimeEventMask)outMask;
    }

    public Task<OfflineRegionInfo> OfflineRegionCreateAsync(
        OfflineRegionDefinition definition,
        byte[] metadata,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_offline_region_create"
        );
        var bufferMetadata = scope.Buffer(metadata);
        var operation = NativeCompletion.Submit(
            (completion, diagnostic) =>
            {
                var nativeDefinition = NativeOfflineRegionDefinition(definition, scope);
                return NativeMethods.mln_runtime_offline_region_create(
                    Handle,
                    &nativeDefinition,
                    (byte*)bufferMetadata.data,
                    checked((nuint)bufferMetadata.size),
                    completion,
                    diagnostic
                );
            },
            result => CopyOfflineRegionInfo(NativeCompletion.Value<mln_offline_region_info>(result))
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task OfflineRegionDeleteAsync(
        long regionId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_offline_region_delete"
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_offline_region_delete(
                        Handle,
                        regionId,
                        completion,
                        diagnostic
                    ),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task<OfflineRegionInfo?> OfflineRegionGetAsync(
        long regionId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_offline_region_get"
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_offline_region_get(
                        Handle,
                        regionId,
                        completion,
                        diagnostic
                    ),
                result =>
                    result->value_count == 0
                        ? (OfflineRegionInfo?)null
                        : CopyOfflineRegionInfo(
                            NativeCompletion.Value<mln_offline_region_info>(result)
                        )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<OfflineRegionStatus> OfflineRegionGetStatusAsync(
        long regionId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_offline_region_get_status"
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_offline_region_get_status(
                        Handle,
                        regionId,
                        completion,
                        diagnostic
                    ),
                result =>
                    CopyOfflineRegionStatus(
                        NativeCompletion.Value<mln_offline_region_status>(result)
                    )
            )
            .WaitAsync(cancellationToken);
    }

    public Task OfflineRegionInvalidateAsync(
        long regionId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_offline_region_invalidate"
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_offline_region_invalidate(
                        Handle,
                        regionId,
                        completion,
                        diagnostic
                    ),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task OfflineRegionSetDownloadStateAsync(
        long regionId,
        OfflineRegionDownloadState state,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_offline_region_set_download_state"
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_offline_region_set_download_state(
                        Handle,
                        regionId,
                        (uint)state,
                        completion,
                        diagnostic
                    ),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task OfflineRegionSetObservedAsync(
        long regionId,
        bool observed,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_offline_region_set_observed"
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_offline_region_set_observed(
                        Handle,
                        regionId,
                        (byte)(observed ? 1 : 0),
                        completion,
                        diagnostic
                    ),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task<OfflineRegionInfo> OfflineRegionUpdateMetadataAsync(
        long regionId,
        byte[] metadata,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_offline_region_update_metadata"
        );
        var bufferMetadata = scope.Buffer(metadata);
        var operation = NativeCompletion.Submit(
            (completion, diagnostic) =>
                NativeMethods.mln_runtime_offline_region_update_metadata(
                    Handle,
                    regionId,
                    (byte*)bufferMetadata.data,
                    checked((nuint)bufferMetadata.size),
                    completion,
                    diagnostic
                ),
            result => CopyOfflineRegionInfo(NativeCompletion.Value<mln_offline_region_info>(result))
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<OfflineRegionInfo[]> OfflineRegionsListAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_offline_regions_list"
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_offline_regions_list(Handle, completion, diagnostic),
                result =>
                {
                    var values = NativeCompletion.Values<mln_offline_region_info>(result);
                    var copied = new OfflineRegionInfo[values.Length];
                    for (var index = 0; index < values.Length; index++)
                        copied[index] = CopyOfflineRegionInfo(values[index]);
                    return copied;
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<OfflineRegionInfo[]> OfflineRegionsMergeDatabaseAsync(
        string sideDatabasePath,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_offline_regions_merge_database"
        );
        ArgumentNullException.ThrowIfNull(sideDatabasePath);
        using var nativeSideDatabasePath = NativeUtf8String.FromNullableString(
            sideDatabasePath,
            nameof(sideDatabasePath)
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_offline_regions_merge_database(
                        Handle,
                        nativeSideDatabasePath.Pointer,
                        completion,
                        diagnostic
                    ),
                result =>
                {
                    var values = NativeCompletion.Values<mln_offline_region_info>(result);
                    var copied = new OfflineRegionInfo[values.Length];
                    for (var index = 0; index < values.Length; index++)
                        copied[index] = CopyOfflineRegionInfo(values[index]);
                    return copied;
                }
            )
            .WaitAsync(cancellationToken);
    }

    public void Close() => CloseAsync().GetAwaiter().GetResult();

    public Task CloseAsync()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_release"
        );
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
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_run_ambient_cache_operation"
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_run_ambient_cache_operation(
                        Handle,
                        (uint)operation,
                        completion,
                        diagnostic
                    ),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public void SetEventMask(RuntimeEventMask mask)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_set_event_mask"
        );
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_runtime_set_event_mask(
                Handle,
                (ulong)mask,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
    }

    public Task SetHttpHeaderTransformAsync(
        HttpHeaderTransform transform,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_set_http_header_transform"
        );
        var operation = NativeCompletion.Submit(
            (completion, diagnostic) =>
            {
                var nativeTransform = NativeHttpHeaderTransform(transform, scope);
                return NativeMethods.mln_runtime_set_http_header_transform(
                    Handle,
                    &nativeTransform,
                    completion,
                    diagnostic
                );
            },
            result => true
        );
        scope.Accept(this.CallbackOwner);
        return operation.WaitAsync(cancellationToken);
    }

    public Task SetMaximumAmbientCacheSizeAsync(
        ulong size,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_set_maximum_ambient_cache_size"
        );
        return NativeCompletion
            .Submit(
                (completion, diagnostic) =>
                    NativeMethods.mln_runtime_set_maximum_ambient_cache_size(
                        Handle,
                        size,
                        completion,
                        diagnostic
                    ),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task SetResourceProviderAsync(
        ResourceProvider provider,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_set_resource_provider"
        );
        var operation = NativeCompletion.Submit(
            (completion, diagnostic) =>
            {
                var nativeProvider = NativeResourceProvider(provider, scope);
                return NativeMethods.mln_runtime_set_resource_provider(
                    Handle,
                    &nativeProvider,
                    completion,
                    diagnostic
                );
            },
            result => true
        );
        scope.Accept(this.CallbackOwner);
        return operation.WaitAsync(cancellationToken);
    }

    public Task SetResourceTransformAsync(
        ResourceTransform transform,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_runtime_set_resource_transform"
        );
        var operation = NativeCompletion.Submit(
            (completion, diagnostic) =>
            {
                var nativeTransform = NativeResourceTransform(transform, scope);
                return NativeMethods.mln_runtime_set_resource_transform(
                    Handle,
                    &nativeTransform,
                    completion,
                    diagnostic
                );
            },
            result => true
        );
        scope.Accept(this.CallbackOwner);
        return operation.WaitAsync(cancellationToken);
    }
}
