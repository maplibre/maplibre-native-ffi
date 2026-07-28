using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Internal.Handle;
using Maplibre.Native.Internal.Loader;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Offline;
using Maplibre.Native.Resource;

namespace Maplibre.Native.Runtime;

internal unsafe delegate mln_status RuntimeSetResourceProvider(
    mln_runtime* runtime,
    mln_resource_provider* provider
);

internal unsafe delegate mln_status RuntimeSetResourceTransform(
    mln_runtime* runtime,
    mln_resource_transform* transform
);

internal unsafe delegate mln_status RuntimeTakeOfflineRegionStatusResult(
    mln_runtime* runtime,
    ulong operationId,
    mln_offline_region_status* outStatus
);

/// <summary>Owner-thread runtime handle for MapLibre Native work and event polling.</summary>
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
        static (runtime, operationId, outStatus) =>
            NativeMethods.mln_runtime_offline_region_get_status_take_result(
                runtime,
                operationId,
                outStatus
            );

    [ThreadStatic]
    private static RuntimeSetResourceProvider? setResourceProviderForTest;

    [ThreadStatic]
    private static RuntimeSetResourceTransform? setResourceTransformForTest;

    [ThreadStatic]
    private static RuntimeTakeOfflineRegionStatusResult? takeOfflineRegionStatusForTest;

    private readonly Lock callbackGate = new();
    private readonly Lock mapGate = new();
    private readonly Dictionary<nint, WeakReference<Map.MapHandle>> liveMaps = [];
    private readonly NativeHandleState<mln_runtime> state;
    private ResourceProviderState? resourceProviderState;
    private ResourceTransformState? resourceTransformState;

    private RuntimeHandle(mln_runtime* handle)
    {
        state = new NativeHandleState<mln_runtime>(
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
        mln_runtime* runtime = null;

        NativeStatus.Check(NativeMethods.mln_runtime_create(&value, &runtime));
        return new RuntimeHandle(runtime);
    }

    internal mln_runtime* Pointer => state.Pointer;

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
                NativeStatus.Check(SetResourceProviderNative(Pointer, &descriptor));
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
                NativeStatus.Check(SetResourceTransformNative(Pointer, &descriptor));
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
            NativeStatus.Check(NativeMethods.mln_runtime_clear_resource_provider(Pointer));
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
            NativeStatus.Check(NativeMethods.mln_runtime_clear_resource_transform(Pointer));
            var previous = resourceTransformState;
            resourceTransformState = null;
            previous?.Dispose();
        }
    }

    /// <summary>Starts an ambient cache maintenance operation.</summary>
    public OfflineOperationHandle StartAmbientCacheOperation(AmbientCacheOperation operation)
    {
        ulong operationId = 0;
        NativeStatus.Check(
            NativeMethods.mln_runtime_run_ambient_cache_operation_start(
                Pointer,
                (uint)operation,
                &operationId
            )
        );
        return OfflineOperation(
            operationId,
            OfflineOperationKind.AmbientCache,
            OfflineOperationResultKind.None
        );
    }

    /// <summary>Starts an offline region creation operation.</summary>
    public OfflineOperationHandle StartCreateOfflineRegion(
        OfflineRegionDefinition definition,
        byte[] metadata
    )
    {
        ArgumentNullException.ThrowIfNull(metadata);
        using var nativeDefinition = NativeOfflineRegionDefinition.From(definition);
        var definitionValue = nativeDefinition.Value;
        ulong operationId = 0;
        fixed (byte* metadataPointer = metadata)
        {
            NativeStatus.Check(
                NativeMethods.mln_runtime_offline_region_create_start(
                    Pointer,
                    &definitionValue,
                    metadata.Length == 0 ? null : metadataPointer,
                    (nuint)metadata.Length,
                    &operationId
                )
            );
        }
        return OfflineOperation(
            operationId,
            OfflineOperationKind.RegionCreate,
            OfflineOperationResultKind.Region
        );
    }

    /// <summary>Starts an offline region lookup operation.</summary>
    public OfflineOperationHandle StartOfflineRegion(long id)
    {
        ulong operationId = 0;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_get_start(Pointer, id, &operationId)
        );
        return OfflineOperation(
            operationId,
            OfflineOperationKind.RegionGet,
            OfflineOperationResultKind.OptionalRegion
        );
    }

    /// <summary>Starts an offline region list operation.</summary>
    public OfflineOperationHandle StartOfflineRegions()
    {
        ulong operationId = 0;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_regions_list_start(Pointer, &operationId)
        );
        return OfflineOperation(
            operationId,
            OfflineOperationKind.RegionsList,
            OfflineOperationResultKind.RegionList
        );
    }

    /// <summary>Starts an offline region database merge operation.</summary>
    public OfflineOperationHandle StartMergeOfflineRegionsDatabase(string path)
    {
        ArgumentNullException.ThrowIfNull(path);
        using var nativePath = NativeUtf8String.FromNullableString(path, nameof(path));
        ulong operationId = 0;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_regions_merge_database_start(
                Pointer,
                nativePath.Pointer,
                &operationId
            )
        );
        return OfflineOperation(
            operationId,
            OfflineOperationKind.RegionsMergeDatabase,
            OfflineOperationResultKind.RegionList
        );
    }

    /// <summary>Starts an offline region metadata update operation.</summary>
    public OfflineOperationHandle StartUpdateOfflineRegionMetadata(long id, byte[] metadata)
    {
        ArgumentNullException.ThrowIfNull(metadata);
        ulong operationId = 0;
        fixed (byte* metadataPointer = metadata)
        {
            NativeStatus.Check(
                NativeMethods.mln_runtime_offline_region_update_metadata_start(
                    Pointer,
                    id,
                    metadata.Length == 0 ? null : metadataPointer,
                    (nuint)metadata.Length,
                    &operationId
                )
            );
        }
        return OfflineOperation(
            operationId,
            OfflineOperationKind.RegionUpdateMetadata,
            OfflineOperationResultKind.Region
        );
    }

    /// <summary>Starts an offline region status lookup operation.</summary>
    public OfflineOperationHandle StartOfflineRegionStatus(long id)
    {
        ulong operationId = 0;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_get_status_start(Pointer, id, &operationId)
        );
        return OfflineOperation(
            operationId,
            OfflineOperationKind.RegionGetStatus,
            OfflineOperationResultKind.RegionStatus
        );
    }

    /// <summary>Starts an offline region observed-state update operation.</summary>
    public OfflineOperationHandle StartSetOfflineRegionObserved(long id, bool observed)
    {
        ulong operationId = 0;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_set_observed_start(
                Pointer,
                id,
                observed ? (byte)1 : (byte)0,
                &operationId
            )
        );
        return OfflineOperation(
            operationId,
            OfflineOperationKind.RegionSetObserved,
            OfflineOperationResultKind.None
        );
    }

    /// <summary>Starts an offline region download-state update operation.</summary>
    public OfflineOperationHandle StartSetOfflineRegionDownloadState(
        long id,
        OfflineRegionDownloadState downloadState
    )
    {
        ulong operationId = 0;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_set_download_state_start(
                Pointer,
                id,
                (uint)downloadState,
                &operationId
            )
        );
        return OfflineOperation(
            operationId,
            OfflineOperationKind.RegionSetDownloadState,
            OfflineOperationResultKind.None
        );
    }

    /// <summary>Starts an offline region invalidation operation.</summary>
    public OfflineOperationHandle StartInvalidateOfflineRegion(long id)
    {
        ulong operationId = 0;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_invalidate_start(Pointer, id, &operationId)
        );
        return OfflineOperation(
            operationId,
            OfflineOperationKind.RegionInvalidate,
            OfflineOperationResultKind.None
        );
    }

    /// <summary>Starts an offline region delete operation.</summary>
    public OfflineOperationHandle StartDeleteOfflineRegion(long id)
    {
        ulong operationId = 0;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_delete_start(Pointer, id, &operationId)
        );
        return OfflineOperation(
            operationId,
            OfflineOperationKind.RegionDelete,
            OfflineOperationResultKind.None
        );
    }

    public OfflineRegionInfo TakeCreateOfflineRegionResult(OfflineOperationHandle operation)
    {
        ArgumentNullException.ThrowIfNull(operation);
        var operationId = operation.RequireLive(
            this,
            OfflineOperationKind.RegionCreate,
            OfflineOperationResultKind.Region
        );
        mln_offline_region_snapshot* snapshot = null;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_create_take_result(
                Pointer,
                operationId,
                &snapshot
            )
        );
        operation.MarkConsumed();
        return OfflineStructs.ReadSnapshot(snapshot);
    }

    public OfflineRegionInfo? TakeOfflineRegionResult(OfflineOperationHandle operation)
    {
        ArgumentNullException.ThrowIfNull(operation);
        var operationId = operation.RequireLive(
            this,
            OfflineOperationKind.RegionGet,
            OfflineOperationResultKind.OptionalRegion
        );
        mln_offline_region_snapshot* snapshot = null;
        bool found = false;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_get_take_result(
                Pointer,
                operationId,
                &snapshot,
                &found
            )
        );
        operation.MarkConsumed();
        return found ? OfflineStructs.ReadSnapshot(snapshot) : null;
    }

    public IReadOnlyList<OfflineRegionInfo> TakeOfflineRegionsResult(
        OfflineOperationHandle operation
    )
    {
        ArgumentNullException.ThrowIfNull(operation);
        var operationId = operation.RequireLive(
            this,
            OfflineOperationKind.RegionsList,
            OfflineOperationResultKind.RegionList
        );
        mln_offline_region_list* list = null;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_regions_list_take_result(Pointer, operationId, &list)
        );
        operation.MarkConsumed();
        return OfflineStructs.ReadList(list);
    }

    public IReadOnlyList<OfflineRegionInfo> TakeMergeOfflineRegionsDatabaseResult(
        OfflineOperationHandle operation
    )
    {
        ArgumentNullException.ThrowIfNull(operation);
        var operationId = operation.RequireLive(
            this,
            OfflineOperationKind.RegionsMergeDatabase,
            OfflineOperationResultKind.RegionList
        );
        mln_offline_region_list* list = null;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_regions_merge_database_take_result(
                Pointer,
                operationId,
                &list
            )
        );
        operation.MarkConsumed();
        return OfflineStructs.ReadList(list);
    }

    public OfflineRegionInfo TakeUpdateOfflineRegionMetadataResult(OfflineOperationHandle operation)
    {
        ArgumentNullException.ThrowIfNull(operation);
        var operationId = operation.RequireLive(
            this,
            OfflineOperationKind.RegionUpdateMetadata,
            OfflineOperationResultKind.Region
        );
        mln_offline_region_snapshot* snapshot = null;
        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_region_update_metadata_take_result(
                Pointer,
                operationId,
                &snapshot
            )
        );
        operation.MarkConsumed();
        return OfflineStructs.ReadSnapshot(snapshot);
    }

    public OfflineRegionStatus TakeOfflineRegionStatusResult(OfflineOperationHandle operation)
    {
        ArgumentNullException.ThrowIfNull(operation);
        var operationId = operation.RequireLive(
            this,
            OfflineOperationKind.RegionGetStatus,
            OfflineOperationResultKind.RegionStatus
        );
        var status = new mln_offline_region_status
        {
            size = (uint)sizeof(mln_offline_region_status),
        };
        NativeStatus.Check(TakeOfflineRegionStatusNative(Pointer, operationId, &status));
        operation.MarkConsumed();
        return OfflineStructs.ReadStatus(status);
    }

    internal void DiscardOfflineOperation(OfflineOperationHandle operation)
    {
        ArgumentNullException.ThrowIfNull(operation);
        if (operation.IsClosed)
        {
            return;
        }

        var operationId = operation.RequireLive(this);
        mln_runtime* runtime;
        try
        {
            runtime = Pointer;
        }
        catch (Error.InvalidStateException) when (IsClosed)
        {
            // The runtime already owns cleanup for its pending operations.
            operation.MarkConsumed();
            return;
        }

        NativeStatus.Check(
            NativeMethods.mln_runtime_offline_operation_discard(runtime, operationId)
        );
        operation.MarkConsumed();
    }

    private OfflineOperationHandle OfflineOperation(
        ulong operationId,
        OfflineOperationKind kind,
        OfflineOperationResultKind resultKind
    ) => new(this, operationId, kind, resultKind);

    internal void RegisterMap(Map.MapHandle map)
    {
        ArgumentNullException.ThrowIfNull(map);
        lock (mapGate)
        {
            liveMaps[map.NativeAddress] = new WeakReference<Map.MapHandle>(map);
        }
    }

    internal void UnregisterMap(Map.MapHandle map)
    {
        ArgumentNullException.ThrowIfNull(map);
        lock (mapGate)
        {
            if (
                liveMaps.TryGetValue(map.NativeAddress, out var reference)
                && reference.TryGetTarget(out var target)
                && ReferenceEquals(target, map)
            )
            {
                liveMaps.Remove(map.NativeAddress);
            }
        }
    }

    private Map.MapHandle? MapFor(nint address)
    {
        if (address == 0)
        {
            return null;
        }

        lock (mapGate)
        {
            if (!liveMaps.TryGetValue(address, out var reference))
            {
                return null;
            }

            if (reference.TryGetTarget(out var map))
            {
                return map;
            }

            liveMaps.Remove(address);
            return null;
        }
    }

    /// <summary>Advances this runtime.</summary>
    /// <remarks>
    /// The call parks the owner thread when <paramref name="timeout" /> allows it,
    /// then drains the owner-thread task queues. Drain the queued runtime events
    /// with <see cref="PollEvent" /> afterwards.
    /// <para>
    /// <paramref name="timeout" /> sets the park bound. <see cref="TimeSpan.Zero" />
    /// drains and returns; hosts pumping from a frame callback pass it. A positive
    /// value parks for up to that long; hosts that own their pump thread pass one
    /// and take their cadence from the runtime's own work. A negative value parks
    /// until a wake arrives.
    /// </para>
    /// <para>
    /// The drain runs every task queued when it begins plus every task those
    /// enqueue, so a single call can span a full style parse.
    /// </para>
    /// <para>
    /// A wake is a latch. One pump consumes one latch, and work arriving during the
    /// drain latches the next wake, so a pump that finds no new work is ordinary.
    /// Style, tile, offline, and resource responses latch a wake, as does
    /// <see cref="WakeSource.Signal" />. A queued unread runtime event also returns
    /// the call without parking. Timers and ready file descriptors latch a wake when
    /// they queue owner-thread work, so pass a bounded timeout to cap park latency.
    /// </para>
    /// <para>
    /// A non-zero timeout blocks the calling thread. Call it outside any lock that a
    /// thread signalling a <see cref="WakeSource" /> takes.
    /// </para>
    /// </remarks>
    public void Pump(TimeSpan timeout)
    {
        var timeoutMilliseconds = timeout < TimeSpan.Zero ? -1L : (long)timeout.TotalMilliseconds;
        NativeStatus.Check(NativeMethods.mln_runtime_pump(Pointer, timeoutMilliseconds));
    }

    /// <summary>
    /// Acquires a wake source that releases this runtime's parked owner thread. The
    /// returned source is usable from any thread, and the caller disposes it.
    /// </summary>
    public WakeSource AcquireWakeSource()
    {
        mln_wake_source* source = null;
        NativeStatus.Check(NativeMethods.mln_runtime_wake_source_acquire(Pointer, &source));
        return new WakeSource(source);
    }

    /// <summary>
    /// Polls and copies the next runtime event, when one is queued, and returns
    /// <see langword="null" /> when the queue is empty.
    /// </summary>
    /// <remarks>
    /// Polling also advances binding-owned state: on a
    /// <see cref="RuntimeEventType.MapStyleLoaded" /> event this binding releases
    /// the map's detached custom geometry sources, closing the upcall stubs for
    /// sources the new style dropped. That release happens when the event is
    /// polled, so drain the queue to keep dropped sources from lingering. It also
    /// needs a resolvable <see cref="RuntimeEvent.MapSource" />; see that member
    /// for the reference caveat.
    /// </remarks>
    public RuntimeEvent? PollEvent()
    {
        var raw = RuntimeStructs.EmptyNativeEvent();
        var hasEvent = false;
        NativeStatus.Check(NativeMethods.mln_runtime_poll_event(Pointer, &raw, &hasEvent));
        if (!hasEvent)
        {
            return null;
        }

        var runtimeEvent = RuntimeStructs.ReadEvent(raw, this, MapFor);
        if (runtimeEvent.Type == RuntimeEventType.MapStyleLoaded)
        {
            runtimeEvent.MapSource?.ReleaseDetachedCustomGeometrySources();
        }
        return runtimeEvent;
    }

    /// <summary>Destroys the runtime on its owner thread.</summary>
    public void Close()
    {
        state.Close();
        DisposeCallbackState();
    }

    /// <inheritdoc />
    public void Dispose()
    {
        if (state.TryClose())
        {
            DisposeCallbackState();
        }
    }

    private void DisposeCallbackState()
    {
        lock (callbackGate)
        {
            var provider = resourceProviderState;
            var transform = resourceTransformState;
            resourceProviderState = null;
            resourceTransformState = null;
            provider?.Dispose();
            transform?.Dispose();
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
