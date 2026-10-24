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

    /// <summary>
    /// The native handle value, which runtime events report as their
    /// source. It stays readable after close, so events drained later
    /// still match this handle.
    /// </summary>
    public ulong Id => state.Handle.Value;

    /// <summary>
    /// Whether a release, dispose, close, or discard has ended the handle.
    /// </summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>
    /// Names the handle's type, its <see cref="Id" />, and whether it is closed.
    /// </summary>
    public override string ToString() => state.ToString();

    public void Dispose()
    {
        state.Retire();
    }

    /// <summary>
    /// Stops event delivery and retires the previous handler.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_clear_event_handler</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task ClearEventHandlerAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_clear_event_handler(
            Handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Clears the runtime-scoped outgoing HTTP header transform.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_clear_http_header_transform</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task ClearHttpHeaderTransformAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_clear_http_header_transform(
            Handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Clears the runtime-scoped network resource provider.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_clear_resource_provider</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task ClearResourceProviderAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_clear_resource_provider(
            Handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Clears the runtime-scoped URL transform for network resources.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_clear_resource_transform</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task ClearResourceTransformAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_clear_resource_transform(
            Handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Creates a runtime.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public static RuntimeHandle Create(RuntimeOptions options)
    {
        using var scope = new NativeCallScope(null);
        var nativeOptions = NativeRuntimeOptions(options, scope);
        MlnRuntime outRuntime = default;
        Check(NativeMethods.mln_runtime_create(&nativeOptions, &outRuntime, Diagnostic));
        scope.Accept();
        return RuntimeHandle.Adopt(outRuntime);
    }

    /// <summary>
    /// Starts creation of a map on the runtime.
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
        using var scope = new NativeCallScope(this);
        var nativeOptions = NativeMapOptions(options);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_create_map(
            Handle,
            &nativeOptions,
            NativeCompletion.Start<MapHandle>(
                &nativeCompletion,
                nativeResult => MapHandle.Adopt(this, NativeCompletion.Value<MlnMap>(nativeResult)),
                cancellationToken,
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        ReadOnlySpan<byte> metadata
    )
    {
        fixed (byte* pinnedMetadata = metadata)
        {
            using var scope = new NativeCallScope(this);
            var nativeDefinition = NativeOfflineRegionDefinition(definition, scope);
            mln_completion nativeCompletion;
            var nativeStatus = NativeMethods.mln_runtime_create_offline_region(
                Handle,
                &nativeDefinition,
                (byte*)pinnedMetadata,
                checked((nuint)metadata.Length),
                NativeCompletion.Start<OfflineRegionInfo>(
                    &nativeCompletion,
                    static nativeResult =>
                        NativeCompletion.Single<mln_offline_region_info, OfflineRegionInfo>(
                            nativeResult,
                            CopyOfflineRegionInfo
                        ),
                    out var nativePending
                ),
                Diagnostic
            );
            return scope.Accepted(nativePending.Submitted(nativeStatus));
        }
    }

    /// <summary>
    /// Deletes an offline region.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_delete_offline_region</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task DeleteOfflineRegionAsync(long regionId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_delete_offline_region(
            Handle,
            regionId,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        using var call = Enter(this);
        ulong outMask = default;
        Check(NativeMethods.mln_runtime_get_event_mask(Handle, &outMask, Diagnostic));
        return (RuntimeEventMask)outMask;
    }

    /// <summary>
    /// Starts getting one offline region by ID.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_get_offline_region</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<OfflineRegionInfo?> GetOfflineRegionAsync(long regionId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_get_offline_region(
            Handle,
            regionId,
            NativeCompletion.Start<OfflineRegionInfo?>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.OptionalValue<mln_offline_region_info, OfflineRegionInfo>(
                        nativeResult,
                        CopyOfflineRegionInfo
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts getting the current download status for an offline region.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_get_offline_region_status</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<OfflineRegionStatus> GetOfflineRegionStatusAsync(long regionId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_get_offline_region_status(
            Handle,
            regionId,
            NativeCompletion.Start<OfflineRegionStatus>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_offline_region_status, OfflineRegionStatus>(
                        nativeResult,
                        CopyOfflineRegionStatus
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Invalidates cached resources for an offline region.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_invalidate_offline_region</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task InvalidateOfflineRegionAsync(long regionId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_invalidate_offline_region(
            Handle,
            regionId,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts listing the offline regions in the runtime database.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_list_offline_regions</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<OfflineRegionInfo[]> ListOfflineRegionsAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_list_offline_regions(
            Handle,
            NativeCompletion.Start<OfflineRegionInfo[]>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Array<mln_offline_region_info, OfflineRegionInfo>(
                        nativeResult,
                        CopyOfflineRegionInfo
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts merging offline regions from another MapLibre offline database.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_merge_offline_regions</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<OfflineRegionInfo[]> MergeOfflineRegionsAsync(string sideDatabasePath)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_merge_offline_regions(
            Handle,
            scope.Utf8(sideDatabasePath),
            NativeCompletion.Start<OfflineRegionInfo[]>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Array<mln_offline_region_info, OfflineRegionInfo>(
                        nativeResult,
                        CopyOfflineRegionInfo
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts release of a runtime after checking its children.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task CloseAsync()
    {
        state.Close(idempotent: false);
        return teardown;
    }

    public ValueTask DisposeAsync() => state.Close() ? new(teardown) : default;

    private mln_status StartRelease(MlnRuntime handle, mln_diagnostic* _)
    {
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_release(
            handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        teardown = nativePending.Submitted(nativeStatus);
        return mln_status.MLN_STATUS_OK;
    }

    /// <summary>
    /// Starts a MapLibre ambient cache maintenance operation for this runtime.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_run_ambient_cache_operation</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task RunAmbientCacheOperationAsync(AmbientCacheOperation operation)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_run_ambient_cache_operation(
            Handle,
            (uint)operation,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Selects how runtime and map events reach the host.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_event_handler</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetEventHandlerAsync(RuntimeEventHandler handler)
    {
        using var scope = new NativeCallScope(this);
        var nativeHandler = NativeRuntimeEventHandler(handler, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_event_handler(
            Handle,
            &nativeHandler,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        using var call = Enter(this);
        Check(NativeMethods.mln_runtime_set_event_mask(Handle, (ulong)mask, Diagnostic));
    }

    /// <summary>
    /// Registers or replaces the runtime-scoped outgoing HTTP header transform.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_http_header_transform</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetHttpHeaderTransformAsync(HttpHeaderTransform transform)
    {
        using var scope = new NativeCallScope(this);
        var nativeTransform = NativeHttpHeaderTransform(transform, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_http_header_transform(
            Handle,
            &nativeTransform,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts a change to this runtime's maximum ambient cache size.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_maximum_ambient_cache_size</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetMaximumAmbientCacheSizeAsync(ulong size)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_maximum_ambient_cache_size(
            Handle,
            size,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Sets an offline region's native download state.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_offline_region_download_state</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetOfflineRegionDownloadStateAsync(long regionId, OfflineRegionDownloadState state)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_offline_region_download_state(
            Handle,
            regionId,
            (uint)state,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Enables or disables runtime events for an offline region.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_offline_region_observed</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetOfflineRegionObservedAsync(long regionId, bool observed)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_offline_region_observed(
            Handle,
            regionId,
            (byte)(observed ? 1 : 0),
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Registers or replaces a runtime-scoped network resource provider.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_resource_provider</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetResourceProviderAsync(ResourceProvider provider)
    {
        using var scope = new NativeCallScope(this);
        var nativeProvider = NativeResourceProvider(provider, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_resource_provider(
            Handle,
            &nativeProvider,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Registers or updates a runtime-scoped URL transform for network
    /// resources.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_set_resource_transform</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetResourceTransformAsync(ResourceTransform transform)
    {
        using var scope = new NativeCallScope(this);
        var nativeTransform = NativeResourceTransform(transform, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_resource_transform(
            Handle,
            &nativeTransform,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        ReadOnlySpan<byte> metadata
    )
    {
        fixed (byte* pinnedMetadata = metadata)
        {
            using var scope = new NativeCallScope(this);
            mln_completion nativeCompletion;
            var nativeStatus = NativeMethods.mln_runtime_update_offline_region_metadata(
                Handle,
                regionId,
                (byte*)pinnedMetadata,
                checked((nuint)metadata.Length),
                NativeCompletion.Start<OfflineRegionInfo>(
                    &nativeCompletion,
                    static nativeResult =>
                        NativeCompletion.Single<mln_offline_region_info, OfflineRegionInfo>(
                            nativeResult,
                            CopyOfflineRegionInfo
                        ),
                    out var nativePending
                ),
                Diagnostic
            );
            return scope.Accepted(nativePending.Submitted(nativeStatus));
        }
    }
}
