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
    mln_resource_provider* provider,
    ulong* outCommandId
);

internal unsafe delegate mln_status RuntimeSetResourceTransform(
    MlnRuntime runtime,
    mln_resource_transform* transform,
    ulong* outCommandId
);

internal unsafe delegate mln_status RuntimeTakeOfflineRegionStatusResult(
    MlnRuntime runtime,
    MlnOperation operationId,
    mln_offline_region_status* outStatus
);
internal unsafe delegate mln_status RuntimeOperationStart(MlnOperation* outOperation);

/// <summary>Any-thread runtime handle with autonomous native execution.</summary>
public sealed unsafe class RuntimeHandle : IDisposable
{
    private static MlnOperation StartOperation(RuntimeOperationStart start)
    {
        MlnOperation operation = default;
        NativeStatus.Check(start(&operation));
        return operation;
    }

    private static MlnOperation StartCreate(mln_runtime_options options)
    {
        MlnOperation operation = default;
        NativeStatus.Check(NativeMethods.mln_runtime_create_start(&options, &operation));
        return operation;
    }

    private static readonly RuntimeSetResourceProvider DefaultSetResourceProvider = static (
        runtime,
        provider,
        commandId
    ) => NativeMethods.mln_runtime_set_resource_provider(runtime, provider, commandId);
    private static readonly RuntimeSetResourceTransform DefaultSetResourceTransform = static (
        runtime,
        transform,
        commandId
    ) => NativeMethods.mln_runtime_set_resource_transform(runtime, transform, commandId);
    private static readonly RuntimeTakeOfflineRegionStatusResult DefaultTakeOfflineRegionStatus =
        static (_, operationId, outStatus) =>
            NativeMethods.mln_runtime_offline_region_get_status_take_result(operationId, outStatus);

    [ThreadStatic]
    private static RuntimeSetResourceProvider? setResourceProviderForTest;

    [ThreadStatic]
    private static RuntimeSetResourceTransform? setResourceTransformForTest;

    [ThreadStatic]
    private static RuntimeTakeOfflineRegionStatusResult? takeOfflineRegionStatusForTest;

    private readonly Lock mapGate = new();
    private readonly Lock operationGate = new();
    private readonly HashSet<OperationHandle> liveOperations = [];
    private readonly Dictionary<ulong, WeakReference<Map.MapHandle>> liveMaps = [];
    private readonly NativeHandleState<MlnRuntime> state;
    private readonly NotificationReceiver notificationReceiver;

    private RuntimeHandle(MlnRuntime handle, NotificationReceiver notificationReceiver)
    {
        this.notificationReceiver = notificationReceiver;
        state = new NativeHandleState<MlnRuntime>(
            handle,
            static _ => mln_status.MLN_STATUS_OK,
            nameof(RuntimeHandle)
        );
    }

    /// <summary>Creates a runtime asynchronously.</summary>
    public static Task<RuntimeHandle> CreateAsync(
        RuntimeOptions options,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(options);
        cancellationToken.ThrowIfCancellationRequested();
        NativeLibraryLoader.EnsureLoaded();
        using var nativeOptions = options.ToNative();
        var value = nativeOptions.Value;
        var receiver = new NotificationReceiver();
        value.notification_source = receiver.Source;
        MlnOperation operation = default;
        try
        {
            operation = StartCreate(value);
        }
        catch
        {
            NativeMethods.mln_operation_release(operation);
            receiver.Dispose();
            throw;
        }
        return OperationAwaiter.WaitThen(
            receiver.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                MlnRuntime runtime = default;
                NativeStatus.Check(
                    NativeMethods.mln_runtime_create_take_result(operation, &runtime)
                );
                return new RuntimeHandle(runtime, receiver);
            },
            () => NativeMethods.mln_operation_release(operation),
            receiver.Dispose
        );
    }

    internal MlnRuntime Handle => state.Handle;

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Installs or replaces the runtime-scoped resource provider callback.</summary>
    public ulong SetResourceProvider(ResourceProviderCallback callback)
    {
        var replacement = new ResourceProviderState(callback);
        try
        {
            var descriptor = replacement.Descriptor;
            ulong commandId = 0;
            NativeStatus.Check(SetResourceProviderNative(Handle, &descriptor, &commandId));
            return commandId;
        }
        catch (Exception error)
        {
            DisposeAndSuppress(error, replacement);
            throw;
        }
    }

    /// <summary>Installs or replaces the runtime-scoped resource transform callback.</summary>
    public ulong SetResourceTransform(ResourceTransformCallback callback)
    {
        var replacement = new ResourceTransformState(callback);
        try
        {
            var descriptor = replacement.Descriptor;
            ulong commandId = 0;
            NativeStatus.Check(SetResourceTransformNative(Handle, &descriptor, &commandId));
            return commandId;
        }
        catch (Exception error)
        {
            DisposeAndSuppress(error, replacement);
            throw;
        }
    }

    /// <summary>Installs or replaces headers added to built-in HTTP requests.</summary>
    public ulong SetHttpHeaderTransform(HttpHeaderTransformCallback callback)
    {
        var replacement = new HttpHeaderTransformState(callback);
        try
        {
            var descriptor = replacement.Descriptor;
            ulong commandId = 0;
            NativeStatus.Check(
                NativeMethods.mln_runtime_set_http_header_transform(Handle, &descriptor, &commandId)
            );
            return commandId;
        }
        catch (Exception error)
        {
            DisposeAndSuppress(error, replacement);
            throw;
        }
    }

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
    public ulong ClearResourceProvider()
    {
        ulong commandId = 0;
        NativeStatus.Check(NativeMethods.mln_runtime_clear_resource_provider(Handle, &commandId));
        return commandId;
    }

    /// <summary>Clears the runtime-scoped resource transform callback.</summary>
    public ulong ClearResourceTransform()
    {
        ulong commandId = 0;
        NativeStatus.Check(NativeMethods.mln_runtime_clear_resource_transform(Handle, &commandId));
        return commandId;
    }

    /// <summary>Clears headers added to built-in HTTP requests.</summary>
    public ulong ClearHttpHeaderTransform()
    {
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_runtime_clear_http_header_transform(Handle, &commandId)
        );
        return commandId;
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
        notificationReceiver.ForgetOperation(operation.NativeId);
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

    /// <summary>Drains every queued runtime event into one batch of copies.</summary>
    /// <remarks>
    /// The batch reports the events that this runtime and its maps queued under their masks.
    /// Managed copies preserve queue order after the owned native batch is released.
    /// </remarks>
    public RuntimeEventBatch DrainEvents()
    {
        MlnEventBatch batch = default;
        NativeStatus.Check(NativeMethods.mln_runtime_drain_events(Handle, &batch));
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

            return new RuntimeEventBatch(events);
        }
        finally
        {
            NativeMethods.mln_event_batch_release(batch);
        }
    }

    /// <summary>Selects which runtime-originated event types this runtime queues.</summary>
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
        return notificationReceiver.DrainReadyEndpoints();
    }

    /// <summary>Waits until all previously accepted runtime work reaches a terminal state.</summary>
    public Task BarrierAsync(CancellationToken cancellationToken = default)
    {
        var operation = StartOperation(outOperation =>
            NativeMethods.mln_runtime_barrier_start(Handle, outOperation)
        );
        return OperationAwaiter.WaitThen(
            notificationReceiver.WaitForOperationAsync(operation, cancellationToken),
            () => CheckOperationCompletion(operation),
            () => NativeMethods.mln_operation_release(operation)
        );
    }

    /// <summary>Closes the runtime after its autonomous worker stops.</summary>
    public Task CloseAsync(CancellationToken cancellationToken = default)
    {
        if (IsClosed)
        {
            return Task.CompletedTask;
        }
        cancellationToken.ThrowIfCancellationRequested();
        PreflightNoLiveOperations();
        var operation = StartOperation(outOperation =>
            NativeMethods.mln_runtime_close_start(Handle, outOperation)
        );
        return OperationAwaiter.WaitThen(
            notificationReceiver.WaitForOperationAsync(operation),
            () =>
            {
                CheckOperationCompletion(operation);
                state.Close();
            },
            () =>
            {
                NativeMethods.mln_operation_release(operation);
                notificationReceiver.Dispose();
            }
        );
    }

    /// <inheritdoc />
    public void Dispose() => CloseAsync().ConfigureAwait(false).GetAwaiter().GetResult();

    internal Task WaitForOperationAsync(
        MlnOperation operation,
        CancellationToken cancellationToken = default
    ) => notificationReceiver.WaitForOperationAsync(operation, cancellationToken);

    internal static void CheckOperationCompletion(MlnOperation operation)
    {
        mln_status status = default;
        NativeStatus.Check(NativeMethods.mln_operation_get_status(operation, &status));
        if (status != mln_status.MLN_STATUS_OK)
        {
            nuint size = 0;
            NativeStatus.Check(
                NativeMethods.mln_operation_copy_diagnostic(operation, null, 0, &size)
            );
            var bytes = new byte[checked((int)size)];
            fixed (byte* pointer = bytes)
            {
                nuint copied = 0;
                NativeStatus.Check(
                    NativeMethods.mln_operation_copy_diagnostic(
                        operation,
                        (sbyte*)pointer,
                        (nuint)bytes.Length,
                        &copied
                    )
                );
            }
            NativeStatus.Check((int)status, System.Text.Encoding.UTF8.GetString(bytes));
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
