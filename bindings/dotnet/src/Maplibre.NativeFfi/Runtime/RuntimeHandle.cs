using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Internal.Loader;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Offline;
using Maplibre.NativeFfi.Resource;

namespace Maplibre.NativeFfi.Runtime;

internal unsafe delegate mln_status RuntimeSetResourceProvider(
    MlnRuntime runtime,
    mln_resource_provider* provider
);

internal unsafe delegate mln_status RuntimeSetResourceTransform(
    MlnRuntime runtime,
    mln_resource_transform* transform
);

internal unsafe delegate mln_status RuntimeTakeOfflineRegionStatusResult(
    MlnRuntime runtime,
    MlnOperation operationId,
    mln_offline_region_status* outStatus
);

/// <summary>Runtime handle for owner-thread work and any-thread event draining.</summary>
public sealed unsafe class RuntimeHandle : IDisposable
{
    private static readonly RuntimeSetResourceProvider DefaultSetResourceProvider = static (
        runtime,
        provider
    ) => NativeMethods.mln_runtime_set_resource_provider(runtime, provider);
    private static readonly RuntimeSetResourceTransform DefaultSetResourceTransform = static (
        runtime,
        transform
    ) => NativeMethods.mln_runtime_set_resource_transform(runtime, transform);
    private static readonly RuntimeTakeOfflineRegionStatusResult DefaultTakeOfflineRegionStatus =
        static (_, operationId, outStatus) =>
            NativeMethods.mln_runtime_offline_region_get_status_take_result(operationId, outStatus);

    [ThreadStatic]
    private static RuntimeSetResourceProvider? setResourceProviderForTest;

    [ThreadStatic]
    private static RuntimeSetResourceTransform? setResourceTransformForTest;

    [ThreadStatic]
    private static RuntimeTakeOfflineRegionStatusResult? takeOfflineRegionStatusForTest;

    private readonly Lock callbackGate = new();
    private readonly Lock mapGate = new();
    private readonly Lock operationGate = new();
    private readonly HashSet<OperationHandle> liveOperations = [];
    private readonly Dictionary<ulong, WeakReference<Map.MapHandle>> liveMaps = [];
    private readonly NativeHandleState<MlnRuntime> state;
    private MlnNotificationSource notificationSource;
    private ResourceProviderState? resourceProviderState;
    private ResourceTransformState? resourceTransformState;
    private HttpHeaderTransformState? httpHeaderTransformState;

    private RuntimeHandle(MlnRuntime handle, MlnNotificationSource notificationSource)
    {
        this.notificationSource = notificationSource;
        state = new NativeHandleState<MlnRuntime>(
            handle,
            static handle => NativeMethods.mln_runtime_destroy(handle),
            nameof(RuntimeHandle)
        );
    }

    /// <summary>Creates a runtime on the current thread.</summary>
    public static RuntimeHandle Create(RuntimeOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        NativeLibraryLoader.EnsureLoaded();
        using var nativeOptions = options.ToNative();
        var value = nativeOptions.Value;
        MlnNotificationSource source = default;
        NativeStatus.Check(NativeMethods.mln_notification_source_create(&source));
        value.notification_source = source;
        MlnRuntime runtime = default;
        try
        {
            NativeStatus.Check(NativeMethods.mln_runtime_create(&value, &runtime));
            return new RuntimeHandle(runtime, source);
        }
        catch
        {
            _ = NativeMethods.mln_notification_source_close(source);
            throw;
        }
    }

    internal MlnRuntime Handle => state.Handle;

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Installs or replaces the runtime-scoped resource provider callback.</summary>
    public void SetResourceProvider(ResourceProviderCallback callback)
    {
        var replacement = new ResourceProviderState(callback);
        lock (callbackGate)
        {
            try
            {
                var descriptor = replacement.Descriptor;
                NativeStatus.Check(SetResourceProviderNative(Handle, &descriptor));
                var previous = resourceProviderState;
                resourceProviderState = replacement;
                previous?.Dispose();
            }
            catch (Exception error)
            {
                DisposeAndSuppress(error, replacement);
                throw;
            }
        }
    }

    /// <summary>Installs or replaces the runtime-scoped resource transform callback.</summary>
    public void SetResourceTransform(ResourceTransformCallback callback)
    {
        var replacement = new ResourceTransformState(callback);
        lock (callbackGate)
        {
            try
            {
                var descriptor = replacement.Descriptor;
                NativeStatus.Check(SetResourceTransformNative(Handle, &descriptor));
                var previous = resourceTransformState;
                resourceTransformState = replacement;
                previous?.Dispose();
            }
            catch (Exception error)
            {
                DisposeAndSuppress(error, replacement);
                throw;
            }
        }
    }

    /// <summary>Installs or replaces headers added to built-in HTTP requests.</summary>
    public void SetHttpHeaderTransform(HttpHeaderTransformCallback callback)
    {
        var replacement = new HttpHeaderTransformState(callback);
        lock (callbackGate)
        {
            try
            {
                var descriptor = replacement.Descriptor;
                NativeStatus.Check(
                    NativeMethods.mln_runtime_set_http_header_transform(Handle, &descriptor)
                );
                var previous = httpHeaderTransformState;
                httpHeaderTransformState = replacement;
                previous?.Dispose();
            }
            catch (Exception error)
            {
                DisposeAndSuppress(error, replacement);
                throw;
            }
        }
    }

    internal ResourceProviderState? ResourceProviderStateForTest => resourceProviderState;

    internal ResourceTransformState? ResourceTransformStateForTest => resourceTransformState;

    internal static IDisposable UseResourceCallbackInstallMethodsForTest(
        RuntimeSetResourceProvider setProvider,
        RuntimeSetResourceTransform setTransform
    )
    {
        var previousProvider = setResourceProviderForTest;
        var previousTransform = setResourceTransformForTest;
        setResourceProviderForTest = setProvider;
        setResourceTransformForTest = setTransform;
        return new RestoreResourceCallbackInstallMethods(previousProvider, previousTransform);
    }

    private static RuntimeSetResourceProvider SetResourceProviderNative =>
        setResourceProviderForTest ?? DefaultSetResourceProvider;

    private static RuntimeSetResourceTransform SetResourceTransformNative =>
        setResourceTransformForTest ?? DefaultSetResourceTransform;

    internal static IDisposable UseOfflineTakeResultMethodsForTest(
        RuntimeTakeOfflineRegionStatusResult takeOfflineRegionStatus
    )
    {
        var previous = takeOfflineRegionStatusForTest;
        takeOfflineRegionStatusForTest = takeOfflineRegionStatus;
        return new RestoreOfflineTakeResultMethods(previous);
    }

    private static RuntimeTakeOfflineRegionStatusResult TakeOfflineRegionStatusNative =>
        takeOfflineRegionStatusForTest ?? DefaultTakeOfflineRegionStatus;

    private sealed class RestoreOfflineTakeResultMethods(
        RuntimeTakeOfflineRegionStatusResult? previous
    ) : IDisposable
    {
        public void Dispose()
        {
            takeOfflineRegionStatusForTest = previous;
        }
    }

    private sealed class RestoreResourceCallbackInstallMethods(
        RuntimeSetResourceProvider? previousProvider,
        RuntimeSetResourceTransform? previousTransform
    ) : IDisposable
    {
        public void Dispose()
        {
            setResourceProviderForTest = previousProvider;
            setResourceTransformForTest = previousTransform;
        }
    }

    /// <summary>Clears the runtime-scoped resource provider callback.</summary>
    public void ClearResourceProvider()
    {
        lock (callbackGate)
        {
            NativeStatus.Check(NativeMethods.mln_runtime_clear_resource_provider(Handle));
            var previous = resourceProviderState;
            resourceProviderState = null;
            previous?.Dispose();
        }
    }

    /// <summary>Clears the runtime-scoped resource transform callback.</summary>
    public void ClearResourceTransform()
    {
        lock (callbackGate)
        {
            NativeStatus.Check(NativeMethods.mln_runtime_clear_resource_transform(Handle));
            var previous = resourceTransformState;
            resourceTransformState = null;
            previous?.Dispose();
        }
    }

    /// <summary>Clears headers added to built-in HTTP requests.</summary>
    public void ClearHttpHeaderTransform()
    {
        lock (callbackGate)
        {
            NativeStatus.Check(NativeMethods.mln_runtime_clear_http_header_transform(Handle));
            var previous = httpHeaderTransformState;
            httpHeaderTransformState = null;
            previous?.Dispose();
        }
    }

    /// <summary>Starts an ambient cache maintenance operation.</summary>
    public OperationHandle StartAmbientCacheOperation(AmbientCacheOperation operation)
    {
        MlnOperation operationId = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_run_ambient_cache_operation_start(
                Handle,
                (uint)operation,
                &operationId
            )
        );
        return new OperationHandle(this, operationId, OperationResultKind.None);
    }

    /// <summary>Starts a change to this runtime's maximum ambient cache size.</summary>
    /// <remarks>
    /// MapLibre evicts ambient resources to fit the new budget, so lowering it discards cached
    /// resources. Offline regions are unaffected.
    /// </remarks>
    public OperationHandle StartSetMaximumAmbientCacheSize(ulong size)
    {
        MlnOperation operationId = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_set_maximum_ambient_cache_size_start(
                Handle,
                size,
                &operationId
            )
        );
        return new OperationHandle(this, operationId, OperationResultKind.None);
    }

    /// <summary>Starts an offline region creation operation.</summary>
    public OperationHandle StartCreateOfflineRegion(
        OfflineRegionDefinition definition,
        byte[] metadata
    )
    {
        ArgumentNullException.ThrowIfNull(metadata);
        using var nativeDefinition = NativeOfflineRegionDefinition.From(definition);
        var definitionValue = nativeDefinition.Value;
        MlnOperation operationId = default;
        fixed (byte* metadataPointer = metadata)
        {
            NativeStatus.Check(
                NativeMethods.mln_runtime_offline_region_create_start(
                    Handle,
                    &definitionValue,
                    metadata.Length == 0 ? null : metadataPointer,
                    (nuint)metadata.Length,
                    &operationId
                )
            );
        }
        return new OperationHandle(this, operationId, OperationResultKind.CreateRegion);
    }

    /// <summary>Starts an offline region lookup operation.</summary>
    public OperationHandle StartOfflineRegion(long id)
    {
        MlnOperation operationId = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_get_start(Handle, id, &operationId)
        );
        return new OperationHandle(this, operationId, OperationResultKind.GetRegion);
    }

    /// <summary>Starts an offline region list operation.</summary>
    public OperationHandle StartOfflineRegions()
    {
        MlnOperation operationId = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_regions_list_start(Handle, &operationId)
        );
        return new OperationHandle(this, operationId, OperationResultKind.ListRegions);
    }

    /// <summary>Starts an offline region database merge operation.</summary>
    public OperationHandle StartMergeOfflineRegionsDatabase(string path)
    {
        ArgumentNullException.ThrowIfNull(path);
        using var nativePath = NativeUtf8String.FromNullableString(path, nameof(path));
        MlnOperation operationId = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_regions_merge_database_start(
                Handle,
                nativePath.Pointer,
                &operationId
            )
        );
        return new OperationHandle(this, operationId, OperationResultKind.MergeRegions);
    }

    /// <summary>Starts an offline region metadata update operation.</summary>
    public OperationHandle StartUpdateOfflineRegionMetadata(long id, byte[] metadata)
    {
        ArgumentNullException.ThrowIfNull(metadata);
        MlnOperation operationId = default;
        fixed (byte* metadataPointer = metadata)
        {
            NativeStatus.Check(
                NativeMethods.mln_runtime_offline_region_update_metadata_start(
                    Handle,
                    id,
                    metadata.Length == 0 ? null : metadataPointer,
                    (nuint)metadata.Length,
                    &operationId
                )
            );
        }
        return new OperationHandle(this, operationId, OperationResultKind.UpdateRegionMetadata);
    }

    /// <summary>Starts an offline region status lookup operation.</summary>
    public OperationHandle StartOfflineRegionStatus(long id)
    {
        MlnOperation operationId = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_get_status_start(Handle, id, &operationId)
        );
        return new OperationHandle(this, operationId, OperationResultKind.RegionStatus);
    }

    /// <summary>Starts an offline region observed-state update operation.</summary>
    public OperationHandle StartSetOfflineRegionObserved(long id, bool observed)
    {
        MlnOperation operationId = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_set_observed_start(
                Handle,
                id,
                observed ? (byte)1 : (byte)0,
                &operationId
            )
        );
        return new OperationHandle(this, operationId, OperationResultKind.None);
    }

    /// <summary>Starts an offline region download-state update operation.</summary>
    public OperationHandle StartSetOfflineRegionDownloadState(
        long id,
        OfflineRegionDownloadState downloadState
    )
    {
        MlnOperation operationId = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_set_download_state_start(
                Handle,
                id,
                (uint)downloadState,
                &operationId
            )
        );
        return new OperationHandle(this, operationId, OperationResultKind.None);
    }

    /// <summary>Starts an offline region invalidation operation.</summary>
    public OperationHandle StartInvalidateOfflineRegion(long id)
    {
        MlnOperation operationId = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_invalidate_start(Handle, id, &operationId)
        );
        return new OperationHandle(this, operationId, OperationResultKind.None);
    }

    /// <summary>Starts an offline region delete operation.</summary>
    public OperationHandle StartDeleteOfflineRegion(long id)
    {
        MlnOperation operationId = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_delete_start(Handle, id, &operationId)
        );
        return new OperationHandle(this, operationId, OperationResultKind.None);
    }

    public OfflineRegionInfo TakeCreateOfflineRegionResult(OperationHandle operation)
    {
        ArgumentNullException.ThrowIfNull(operation);
        using var use = operation.AcquireResultUse(this, OperationResultKind.CreateRegion);
        MlnOfflineRegionSnapshot snapshot = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_create_take_result(use.Handle, &snapshot)
        );
        operation.MarkResultConsumed(use);
        return OfflineStructs.ReadSnapshot(snapshot);
    }

    public OfflineRegionInfo? TakeOfflineRegionResult(OperationHandle operation)
    {
        ArgumentNullException.ThrowIfNull(operation);
        using var use = operation.AcquireResultUse(this, OperationResultKind.GetRegion);
        MlnOfflineRegionSnapshot snapshot = default;
        bool found = false;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_get_take_result(use.Handle, &snapshot, &found)
        );
        operation.MarkResultConsumed(use);
        return found ? OfflineStructs.ReadSnapshot(snapshot) : null;
    }

    public IReadOnlyList<OfflineRegionInfo> TakeOfflineRegionsResult(OperationHandle operation)
    {
        ArgumentNullException.ThrowIfNull(operation);
        using var use = operation.AcquireResultUse(this, OperationResultKind.ListRegions);
        MlnOfflineRegionList list = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_regions_list_take_result(use.Handle, &list)
        );
        operation.MarkResultConsumed(use);
        return OfflineStructs.ReadList(list);
    }

    public IReadOnlyList<OfflineRegionInfo> TakeMergeOfflineRegionsDatabaseResult(
        OperationHandle operation
    )
    {
        ArgumentNullException.ThrowIfNull(operation);
        using var use = operation.AcquireResultUse(this, OperationResultKind.MergeRegions);
        MlnOfflineRegionList list = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_regions_merge_database_take_result(use.Handle, &list)
        );
        operation.MarkResultConsumed(use);
        return OfflineStructs.ReadList(list);
    }

    public OfflineRegionInfo TakeUpdateOfflineRegionMetadataResult(OperationHandle operation)
    {
        ArgumentNullException.ThrowIfNull(operation);
        using var use = operation.AcquireResultUse(this, OperationResultKind.UpdateRegionMetadata);
        MlnOfflineRegionSnapshot snapshot = default;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_update_metadata_take_result(
                use.Handle,
                &snapshot
            )
        );
        operation.MarkResultConsumed(use);
        return OfflineStructs.ReadSnapshot(snapshot);
    }

    public OfflineRegionStatus TakeOfflineRegionStatusResult(OperationHandle operation)
    {
        ArgumentNullException.ThrowIfNull(operation);
        using var use = operation.AcquireResultUse(this, OperationResultKind.RegionStatus);
        var status = new mln_offline_region_status
        {
            size = (uint)sizeof(mln_offline_region_status),
        };
        NativeStatus.Check(TakeOfflineRegionStatusNative(Handle, use.Handle, &status));
        operation.MarkResultConsumed(use);
        return OfflineStructs.ReadStatus(status);
    }

    internal void RegisterOperation(OperationHandle operation)
    {
        lock (operationGate)
        {
            liveOperations.Add(operation);
        }
    }

    internal void UnregisterOperation(OperationHandle operation)
    {
        lock (operationGate)
        {
            liveOperations.Remove(operation);
        }
    }

    internal void RegisterMap(Map.MapHandle map)
    {
        ArgumentNullException.ThrowIfNull(map);
        lock (mapGate)
        {
            liveMaps[map.NativeId] = new WeakReference<Map.MapHandle>(map);
        }
    }

    internal void UnregisterMap(Map.MapHandle map)
    {
        ArgumentNullException.ThrowIfNull(map);
        lock (mapGate)
        {
            liveMaps.Remove(map.NativeId);
        }
    }

    // Callers hold mapGate across a whole batch, so a drain takes the registry lock once
    // however many map-sourced events it carries.
    private Map.MapHandle? MapForLocked(ulong id)
    {
        if (id == 0)
        {
            return null;
        }

        if (!liveMaps.TryGetValue(id, out var reference))
        {
            return null;
        }

        if (reference.TryGetTarget(out var map))
        {
            return map;
        }

        liveMaps.Remove(id);
        return null;
    }

    /// <summary>Advances this runtime.</summary>
    /// <remarks>
    /// The call parks the owner thread when <paramref name="timeout" /> allows it,
    /// then drains the owner-thread task queues. Drain the queued runtime events
    /// with <see cref="DrainEvents()" /> afterwards.
    /// <para>
    /// <see cref="TimeSpan.Zero" /> drains and returns, a positive value parks for
    /// up to that long, and a negative value parks until a wake arrives. Timers and
    /// ready file descriptors wake the runtime only when they queue owner-thread
    /// work, so pass a bounded timeout to cap how long a call waits.
    /// </para>
    /// <para>
    /// A non-zero timeout blocks the calling thread. Call it outside any lock that a
    /// thread signalling a <see cref="WakeSource" /> takes. A queued event releases a
    /// park, so a host that pumps and drains in a loop keeps making progress.
    /// </para>
    /// </remarks>
    public void Pump(TimeSpan timeout)
    {
        var timeoutMilliseconds = timeout < TimeSpan.Zero ? -1L : (long)timeout.TotalMilliseconds;
        NativeStatus.Check(NativeMethods.mln_runtime_pump(Handle, timeoutMilliseconds));
    }

    /// <summary>
    /// Acquires a wake source that releases this runtime's parked owner thread. The
    /// returned source is usable from any thread, and the caller disposes it.
    /// </summary>
    public WakeSource AcquireWakeSource()
    {
        MlnWakeSource source = default;
        NativeStatus.Check(NativeMethods.mln_runtime_wake_source_acquire(Handle, &source));
        return new WakeSource(source);
    }

    /// <summary>Drains every queued runtime event into one batch of copies.</summary>
    /// <remarks>
    /// The batch reports the events that this runtime and its maps queued under their masks.
    /// Managed copies preserve queue order after the owned native batch is released.
    /// </remarks>
    public RuntimeEventBatch DrainEvents() => Drain(0);

    /// <summary>Drains at most <paramref name="maxEvents" /> queued runtime events.</summary>
    /// <remarks>
    /// Zero drains every queued event. A positive value drains at most that many and reports how
    /// many stayed queued in <see cref="RuntimeEventBatch.RemainingCount" />, so a host that takes
    /// a bounded slice per iteration learns to come back.
    /// </remarks>
    public RuntimeEventBatch DrainEvents(int maxEvents)
    {
        ArgumentOutOfRangeException.ThrowIfNegative(maxEvents);
        return Drain((nuint)maxEvents);
    }

    /// <summary>Selects which runtime-originated event types this runtime queues.</summary>
    /// <remarks>
    /// The mask reads the bits in <see cref="RuntimeEventMask.AllRuntimeEvents" /> and ignores the
    /// rest, so <see cref="RuntimeEventMask.All" /> selects every runtime-originated type. An event
    /// type this mask clears is never queued, so it neither reaches a batch nor wakes a parked
    /// <see cref="Pump" />.
    /// </remarks>
    public void SetEventMask(RuntimeEventMask mask)
    {
        NativeStatus.Check(NativeMethods.mln_runtime_set_event_mask(Handle, (ulong)mask));
    }

    /// <summary>Reports which runtime-originated event types this runtime queues.</summary>
    /// <remarks>
    /// The value is the mask last set, including bits outside
    /// <see cref="RuntimeEventMask.AllRuntimeEvents" /> that this runtime ignores. A runtime created
    /// with the default <see cref="RuntimeOptions.EventMask" /> reports every event type this
    /// library selects by default, including any this binding does not declare.
    /// </remarks>
    public RuntimeEventMask GetEventMask()
    {
        ulong mask = 0;
        NativeStatus.Check(NativeMethods.mln_runtime_get_event_mask(Handle, &mask));
        return (RuntimeEventMask)mask;
    }

    /// <summary>Drains and copies the endpoints that are ready for this runtime's receiver.</summary>
    public IReadOnlyList<ReadyEndpoint> DrainReadyEndpoints()
    {
        _ = Handle;
        MlnReadyBatch batch = default;
        NativeStatus.Check(
            NativeMethods.mln_notification_source_drain_ready(notificationSource, &batch)
        );
        try
        {
            var view = new mln_ready_batch_view { size = (uint)sizeof(mln_ready_batch_view) };
            NativeStatus.Check(NativeMethods.mln_ready_batch_get(batch, &view));
            if (view.endpoint_size < sizeof(mln_ready_endpoint))
            {
                throw new InvalidStateException(
                    MaplibreStatus.InvalidState,
                    null,
                    "The native ready endpoint stride is smaller than the known endpoint layout.",
                    null
                );
            }
            if (view.endpoint_count != 0 && view.endpoints is null)
            {
                throw new InvalidStateException(
                    MaplibreStatus.InvalidState,
                    null,
                    "The native ready batch has a null endpoint pointer.",
                    null
                );
            }

            var endpoints = new List<ReadyEndpoint>(checked((int)view.endpoint_count));
            var cursor = (byte*)view.endpoints;
            for (nuint index = 0; index < view.endpoint_count; index++)
            {
                var endpoint = (mln_ready_endpoint*)cursor;
                endpoints.Add(
                    new ReadyEndpoint(
                        Enum.IsDefined(typeof(NotificationEndpointKind), endpoint->kind)
                            ? (NotificationEndpointKind)endpoint->kind
                            : 0,
                        endpoint->kind,
                        endpoint->id
                    )
                );
                cursor += view.endpoint_size;
            }
            return endpoints;
        }
        finally
        {
            NativeMethods.mln_ready_batch_release(batch);
        }
    }

    private RuntimeEventBatch Drain(nuint maxEvents)
    {
        MlnEventBatch batch = default;
        NativeStatus.Check(NativeMethods.mln_runtime_drain_events(Handle, maxEvents, &batch));
        try
        {
            var view = new mln_runtime_event_batch_view
            {
                size = (uint)sizeof(mln_runtime_event_batch_view),
            };
            NativeStatus.Check(NativeMethods.mln_event_batch_get(batch, &view));
            List<RuntimeEvent> events;
            lock (mapGate)
            {
                events = RuntimeStructs.ReadBatch(view, this, MapForLocked);
            }

            return new RuntimeEventBatch(events, (ulong)view.remaining_count);
        }
        finally
        {
            NativeMethods.mln_event_batch_release(batch);
        }
    }

    /// <summary>Destroys the runtime on its owner thread.</summary>
    public void Close()
    {
        PreflightNoLiveOperations();
        state.Close();
        DisposeCallbackState();
        CloseNotificationSource();
    }

    /// <inheritdoc />
    public void Dispose()
    {
        PreflightNoLiveOperations();
        if (state.TryClose())
        {
            DisposeCallbackState();
            CloseNotificationSource();
        }
    }

    private void PreflightNoLiveOperations()
    {
        lock (operationGate)
        {
            if (liveOperations.Count != 0)
            {
                throw new InvalidStateException(
                    MaplibreStatus.InvalidState,
                    null,
                    "RuntimeHandle cannot close while an OperationHandle is live.",
                    null
                );
            }
        }
    }

    private void CloseNotificationSource()
    {
        var source = notificationSource;
        if (!source.IsNull)
        {
            NativeStatus.Check(NativeMethods.mln_notification_source_close(source));
            notificationSource = default;
        }
    }

    private void DisposeCallbackState()
    {
        lock (callbackGate)
        {
            var provider = resourceProviderState;
            var transform = resourceTransformState;
            var headerTransform = httpHeaderTransformState;
            resourceProviderState = null;
            resourceTransformState = null;
            httpHeaderTransformState = null;
            provider?.Dispose();
            transform?.Dispose();
            headerTransform?.Dispose();
        }
    }

    private static void DisposeAndSuppress(Exception error, IDisposable? disposable)
    {
        try
        {
            disposable?.Dispose();
        }
        catch (Exception cleanup)
        {
            error.Data["SuppressedCleanupException"] = cleanup;
        }
    }
}
