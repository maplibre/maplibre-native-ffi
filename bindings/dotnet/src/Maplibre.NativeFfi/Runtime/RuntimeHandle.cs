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
    mln_completion* completion
);

internal unsafe delegate mln_status RuntimeSetResourceTransform(
    MlnRuntime runtime,
    mln_resource_transform* transform,
    mln_completion* completion
);

/// <summary>Any-thread runtime handle with autonomous native execution.</summary>
public sealed unsafe class RuntimeHandle : IDisposable, IAsyncDisposable
{
    private static readonly RuntimeSetResourceProvider DefaultSetResourceProvider = static (
        runtime,
        provider,
        completion
    ) => NativeMethods.mln_runtime_set_resource_provider(runtime, provider, completion);
    private static readonly RuntimeSetResourceTransform DefaultSetResourceTransform = static (
        runtime,
        transform,
        completion
    ) => NativeMethods.mln_runtime_set_resource_transform(runtime, transform, completion);

    [ThreadStatic]
    private static RuntimeSetResourceProvider? setResourceProviderForTest;

    [ThreadStatic]
    private static RuntimeSetResourceTransform? setResourceTransformForTest;

    private readonly Lock mapGate = new();
    private readonly Dictionary<ulong, WeakReference<Map.MapHandle>> liveMaps = [];
    private readonly NativeHandleState<MlnRuntime> state;
    private volatile Task teardown = Task.CompletedTask;

    private RuntimeHandle(MlnRuntime handle)
    {
        state = new NativeHandleState<MlnRuntime>(handle, StartRelease, nameof(RuntimeHandle));
    }

    /// <summary>Creates a runtime.</summary>
    public static RuntimeHandle Create(RuntimeOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        NativeLibraryLoader.EnsureLoaded();
        using var nativeOptions = options.ToNative();
        var value = nativeOptions.Value;
        MlnRuntime runtime = default;
        NativeStatus.Check(NativeMethods.mln_runtime_create(&value, &runtime));
        return new RuntimeHandle(runtime);
    }

    internal MlnRuntime Handle => state.Handle;

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Installs or replaces the runtime-scoped resource provider callback.</summary>
    public Task<CommandCompletion> SetResourceProviderAsync(ResourceProviderCallback callback)
    {
        var replacement = new ResourceProviderState(callback);
        try
        {
            return NativeCompletion.SubmitCommand(completion =>
            {
                var descriptor = replacement.Descriptor;
                return SetResourceProviderNative(Handle, &descriptor, completion);
            });
        }
        catch (Exception error)
        {
            DisposeAndSuppress(error, replacement);
            throw;
        }
    }

    /// <summary>Installs or replaces the runtime-scoped resource transform callback.</summary>
    public Task<CommandCompletion> SetResourceTransformAsync(ResourceTransformCallback callback)
    {
        var replacement = new ResourceTransformState(callback);
        try
        {
            return NativeCompletion.SubmitCommand(completion =>
            {
                var descriptor = replacement.Descriptor;
                return SetResourceTransformNative(Handle, &descriptor, completion);
            });
        }
        catch (Exception error)
        {
            DisposeAndSuppress(error, replacement);
            throw;
        }
    }

    /// <summary>Installs or replaces headers added to built-in HTTP requests.</summary>
    public Task<CommandCompletion> SetHttpHeaderTransformAsync(HttpHeaderTransformCallback callback)
    {
        var replacement = new HttpHeaderTransformState(callback);
        try
        {
            return NativeCompletion.SubmitCommand(completion =>
            {
                var descriptor = replacement.Descriptor;
                return NativeMethods.mln_runtime_set_http_header_transform(
                    Handle,
                    &descriptor,
                    completion
                );
            });
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
    public Task<CommandCompletion> ClearResourceProviderAsync() =>
        NativeCompletion.SubmitCommand(completion =>
            NativeMethods.mln_runtime_clear_resource_provider(Handle, completion)
        );

    /// <summary>Clears the runtime-scoped resource transform callback.</summary>
    public Task<CommandCompletion> ClearResourceTransformAsync() =>
        NativeCompletion.SubmitCommand(completion =>
            NativeMethods.mln_runtime_clear_resource_transform(Handle, completion)
        );

    /// <summary>Clears headers added to built-in HTTP requests.</summary>
    public Task<CommandCompletion> ClearHttpHeaderTransformAsync() =>
        NativeCompletion.SubmitCommand(completion =>
            NativeMethods.mln_runtime_clear_http_header_transform(Handle, completion)
        );

    /// <summary>Starts an ambient cache maintenance operation.</summary>
    public Task RunAmbientCacheOperationAsync(AmbientCacheOperation operation) =>
        NativeCompletion.SubmitUnit(completion =>
            NativeMethods.mln_runtime_run_ambient_cache_operation(
                Handle,
                (uint)operation,
                completion
            )
        );

    /// <summary>Starts a change to this runtime's maximum ambient cache size.</summary>
    /// <remarks>
    /// MapLibre evicts ambient resources to fit the new budget, so lowering it discards cached
    /// resources. Offline regions are unaffected.
    /// </remarks>
    public Task SetMaximumAmbientCacheSizeAsync(ulong size) =>
        NativeCompletion.SubmitUnit(completion =>
            NativeMethods.mln_runtime_set_maximum_ambient_cache_size(Handle, size, completion)
        );

    /// <summary>Starts an offline region creation operation.</summary>
    public Task<OfflineRegionInfo> CreateOfflineRegionAsync(
        OfflineRegionDefinition definition,
        byte[] metadata
    )
    {
        ArgumentNullException.ThrowIfNull(metadata);
        using var nativeDefinition = NativeOfflineRegionDefinition.From(definition);
        return NativeCompletion.Submit(
            completion =>
            {
                var definitionValue = nativeDefinition.Value;
                fixed (byte* metadataPointer = metadata)
                {
                    return NativeMethods.mln_runtime_offline_region_create(
                        Handle,
                        &definitionValue,
                        metadata.Length == 0 ? null : metadataPointer,
                        (nuint)metadata.Length,
                        completion
                    );
                }
            },
            ReadOfflineRegion
        );
    }

    /// <summary>Starts an offline region lookup operation.</summary>
    public Task<OfflineRegionInfo?> GetOfflineRegionAsync(long id) =>
        NativeCompletion.Submit(
            completion => NativeMethods.mln_runtime_offline_region_get(Handle, id, completion),
            ReadOptionalOfflineRegion
        );

    /// <summary>Starts an offline region list operation.</summary>
    public Task<IReadOnlyList<OfflineRegionInfo>> ListOfflineRegionsAsync() =>
        NativeCompletion.Submit(
            completion => NativeMethods.mln_runtime_offline_regions_list(Handle, completion),
            ReadOfflineRegions
        );

    /// <summary>Starts an offline region database merge operation.</summary>
    public Task<IReadOnlyList<OfflineRegionInfo>> MergeOfflineRegionsDatabaseAsync(string path)
    {
        ArgumentNullException.ThrowIfNull(path);
        using var nativePath = NativeUtf8String.FromNullableString(path, nameof(path));
        return NativeCompletion.Submit(
            completion =>
                NativeMethods.mln_runtime_offline_regions_merge_database(
                    Handle,
                    nativePath.Pointer,
                    completion
                ),
            ReadOfflineRegions
        );
    }

    /// <summary>Starts an offline region metadata update operation.</summary>
    public Task<OfflineRegionInfo> UpdateOfflineRegionMetadataAsync(long id, byte[] metadata)
    {
        ArgumentNullException.ThrowIfNull(metadata);
        return NativeCompletion.Submit(
            completion =>
            {
                fixed (byte* metadataPointer = metadata)
                {
                    return NativeMethods.mln_runtime_offline_region_update_metadata(
                        Handle,
                        id,
                        metadata.Length == 0 ? null : metadataPointer,
                        (nuint)metadata.Length,
                        completion
                    );
                }
            },
            ReadOfflineRegion
        );
    }

    /// <summary>Starts an offline region status lookup operation.</summary>
    public Task<OfflineRegionStatus> GetOfflineRegionStatusAsync(long id) =>
        NativeCompletion.Submit(
            completion =>
                NativeMethods.mln_runtime_offline_region_get_status(Handle, id, completion),
            ReadOfflineRegionStatus
        );

    /// <summary>Starts an offline region observed-state update operation.</summary>
    public Task SetOfflineRegionObservedAsync(long id, bool observed) =>
        NativeCompletion.SubmitUnit(completion =>
            NativeMethods.mln_runtime_offline_region_set_observed(
                Handle,
                id,
                observed ? (byte)1 : (byte)0,
                completion
            )
        );

    /// <summary>Starts an offline region download-state update operation.</summary>
    public Task SetOfflineRegionDownloadStateAsync(
        long id,
        OfflineRegionDownloadState downloadState
    )
    {
        return NativeCompletion.SubmitUnit(completion =>
            NativeMethods.mln_runtime_offline_region_set_download_state(
                Handle,
                id,
                (uint)downloadState,
                completion
            )
        );
    }

    /// <summary>Starts an offline region invalidation operation.</summary>
    public Task InvalidateOfflineRegionAsync(long id) =>
        NativeCompletion.SubmitUnit(completion =>
            NativeMethods.mln_runtime_offline_region_invalidate(Handle, id, completion)
        );

    /// <summary>Starts an offline region delete operation.</summary>
    public Task DeleteOfflineRegionAsync(long id) =>
        NativeCompletion.SubmitUnit(completion =>
            NativeMethods.mln_runtime_offline_region_delete(Handle, id, completion)
        );

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

    /// <summary>Waits until all previously accepted runtime work reaches a terminal state.</summary>
    public Task BarrierAsync() =>
        NativeCompletion.SubmitUnit(completion =>
            NativeMethods.mln_runtime_barrier(Handle, completion)
        );

    /// <summary>Releases the runtime's public native handle and waits for native teardown.</summary>
    /// <remarks>
    /// The wait covers this runtime's threads and its released maps' teardown, so a host that
    /// returns from this call may exit the process. Call it from a host thread: a MapLibre callback
    /// that waits here blocks the teardown it waits for. Use <see cref="CloseAsync" /> to release
    /// the handle without blocking.
    /// </remarks>
    public void Close() => CloseAsync().GetAwaiter().GetResult();

    /// <summary>Releases the runtime's public native handle without blocking.</summary>
    /// <remarks>
    /// The returned task completes after every earlier accepted submission, including released
    /// maps' teardown, has finished and the runtime's threads and resources are gone. A host that
    /// awaits it may exit the process. The handle is consumed once this method returns, so a
    /// rejected release throws before there is a task to await.
    /// </remarks>
    public Task CloseAsync()
    {
        if (!IsClosed)
        {
            state.Close();
        }

        return teardown;
    }

    /// <inheritdoc />
    public void Dispose() => Close();

    /// <inheritdoc />
    public ValueTask DisposeAsync() => new(CloseAsync());

    private mln_status StartRelease(MlnRuntime handle)
    {
        teardown = NativeCompletion.SubmitUnit(completion =>
            NativeMethods.mln_runtime_release(handle, completion)
        );
        return mln_status.MLN_STATUS_OK;
    }

    private static OfflineRegionInfo ReadOfflineRegion(mln_completion_result* result)
    {
        if (result->value is null || result->value_count != 1)
        {
            throw new InvalidOperationException("Native offline region completion had no value.");
        }
        return OfflineStructs.ReadInfo(*(mln_offline_region_info*)result->value);
    }

    private static OfflineRegionInfo? ReadOptionalOfflineRegion(mln_completion_result* result) =>
        result->value_count == 0 ? null : ReadOfflineRegion(result);

    private static IReadOnlyList<OfflineRegionInfo> ReadOfflineRegions(
        mln_completion_result* result
    )
    {
        if (result->value_count != 0 && result->value is null)
        {
            throw new InvalidOperationException(
                "Native offline region completion had a null array."
            );
        }
        var regions = new OfflineRegionInfo[checked((int)result->value_count)];
        var values = (mln_offline_region_info*)result->value;
        for (nuint index = 0; index < result->value_count; index++)
        {
            regions[(int)index] = OfflineStructs.ReadInfo(values[index]);
        }
        return regions;
    }

    private static OfflineRegionStatus ReadOfflineRegionStatus(mln_completion_result* result)
    {
        if (result->value is null || result->value_count != 1)
        {
            throw new InvalidOperationException("Native offline status completion had no value.");
        }
        return OfflineStructs.ReadStatus(*(mln_offline_region_status*)result->value);
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
