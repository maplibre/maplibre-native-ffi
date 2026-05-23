using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Internal.Handle;
using Maplibre.Native.Internal.Loader;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Offline;
using Maplibre.Native.Resource;

namespace Maplibre.Native.Runtime;

/// <summary>Owner-thread runtime handle for MapLibre Native work and event polling.</summary>
public sealed unsafe class RuntimeHandle : IDisposable
{
    private readonly Lock callbackGate = new();
    private readonly NativeHandleState<mln_runtime> state;
    private ResourceProviderState? resourceProviderState;
    private ResourceTransformState? resourceTransformState;

    private RuntimeHandle(mln_runtime* handle)
    {
        state = new NativeHandleState<mln_runtime>(
            handle,
            static handle => NativeMethods.mln_runtime_destroy(handle),
            nameof(RuntimeHandle));
    }

    /// <summary>Creates a runtime on the current thread.</summary>
    public static RuntimeHandle Create(RuntimeOptions? options = null)
    {
        NativeLibraryLoader.EnsureLoaded();
        options ??= new RuntimeOptions();
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
                NativeStatus.Check(NativeMethods.mln_runtime_set_resource_provider(Pointer, &descriptor));
                var previous = resourceProviderState;
                resourceProviderState = replacement;
                previous?.Dispose();
            }
            catch
            {
                replacement.Dispose();
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
                NativeStatus.Check(NativeMethods.mln_runtime_set_resource_transform(Pointer, &descriptor));
                var previous = resourceTransformState;
                resourceTransformState = replacement;
                previous?.Dispose();
            }
            catch
            {
                replacement.Dispose();
                throw;
            }
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
        NativeStatus.Check(NativeMethods.mln_runtime_run_ambient_cache_operation_start(Pointer, (uint)operation, &operationId));
        return OfflineOperation(operationId, OfflineOperationKind.AmbientCache, OfflineOperationResultKind.None);
    }

    /// <summary>Starts an offline region lookup operation.</summary>
    public OfflineOperationHandle StartOfflineRegion(long id)
    {
        ulong operationId = 0;
        NativeStatus.Check(NativeMethods.mln_runtime_offline_region_get_start(Pointer, id, &operationId));
        return OfflineOperation(operationId, OfflineOperationKind.RegionGet, OfflineOperationResultKind.OptionalRegion);
    }

    /// <summary>Starts an offline region list operation.</summary>
    public OfflineOperationHandle StartOfflineRegions()
    {
        ulong operationId = 0;
        NativeStatus.Check(NativeMethods.mln_runtime_offline_regions_list_start(Pointer, &operationId));
        return OfflineOperation(operationId, OfflineOperationKind.RegionsList, OfflineOperationResultKind.RegionList);
    }

    /// <summary>Starts an offline region database merge operation.</summary>
    public OfflineOperationHandle StartMergeOfflineRegionsDatabase(string path)
    {
        ArgumentNullException.ThrowIfNull(path);
        using var nativePath = NativeUtf8String.FromNullableString(path, nameof(path));
        ulong operationId = 0;
        NativeStatus.Check(NativeMethods.mln_runtime_offline_regions_merge_database_start(Pointer, nativePath.Pointer, &operationId));
        return OfflineOperation(operationId, OfflineOperationKind.RegionsMergeDatabase, OfflineOperationResultKind.RegionList);
    }

    /// <summary>Starts an offline region status lookup operation.</summary>
    public OfflineOperationHandle StartOfflineRegionStatus(long id)
    {
        ulong operationId = 0;
        NativeStatus.Check(NativeMethods.mln_runtime_offline_region_get_status_start(Pointer, id, &operationId));
        return OfflineOperation(operationId, OfflineOperationKind.RegionGetStatus, OfflineOperationResultKind.RegionStatus);
    }

    /// <summary>Starts an offline region observed-state update operation.</summary>
    public OfflineOperationHandle StartSetOfflineRegionObserved(long id, bool observed)
    {
        ulong operationId = 0;
        NativeStatus.Check(NativeMethods.mln_runtime_offline_region_set_observed_start(Pointer, id, observed ? (byte)1 : (byte)0, &operationId));
        return OfflineOperation(operationId, OfflineOperationKind.RegionSetObserved, OfflineOperationResultKind.None);
    }

    /// <summary>Starts an offline region download-state update operation.</summary>
    public OfflineOperationHandle StartSetOfflineRegionDownloadState(long id, OfflineRegionDownloadState downloadState)
    {
        ulong operationId = 0;
        NativeStatus.Check(NativeMethods.mln_runtime_offline_region_set_download_state_start(Pointer, id, (uint)downloadState, &operationId));
        return OfflineOperation(operationId, OfflineOperationKind.RegionSetDownloadState, OfflineOperationResultKind.None);
    }

    /// <summary>Starts an offline region invalidation operation.</summary>
    public OfflineOperationHandle StartInvalidateOfflineRegion(long id)
    {
        ulong operationId = 0;
        NativeStatus.Check(NativeMethods.mln_runtime_offline_region_invalidate_start(Pointer, id, &operationId));
        return OfflineOperation(operationId, OfflineOperationKind.RegionInvalidate, OfflineOperationResultKind.None);
    }

    /// <summary>Starts an offline region delete operation.</summary>
    public OfflineOperationHandle StartDeleteOfflineRegion(long id)
    {
        ulong operationId = 0;
        NativeStatus.Check(NativeMethods.mln_runtime_offline_region_delete_start(Pointer, id, &operationId));
        return OfflineOperation(operationId, OfflineOperationKind.RegionDelete, OfflineOperationResultKind.None);
    }

    internal void DiscardOfflineOperation(OfflineOperationHandle operation)
    {
        ArgumentNullException.ThrowIfNull(operation);
        var operationId = operation.RequireLive(this);
        NativeStatus.Check(NativeMethods.mln_runtime_offline_operation_discard(Pointer, operationId));
        operation.MarkConsumed();
    }

    private OfflineOperationHandle OfflineOperation(ulong operationId, OfflineOperationKind kind, OfflineOperationResultKind resultKind) =>
        new(this, operationId, kind, resultKind);

    /// <summary>Runs one pending owner-thread task for this runtime.</summary>
    public void RunOnce()
    {
        NativeStatus.Check(NativeMethods.mln_runtime_run_once(Pointer));
    }

    /// <summary>Polls and copies the next runtime event, when one is queued.</summary>
    public RuntimeEvent? PollEvent()
    {
        var raw = RuntimeStructs.EmptyNativeEvent();
        var hasEvent = false;
        NativeStatus.Check(NativeMethods.mln_runtime_poll_event(Pointer, &raw, &hasEvent));
        return hasEvent ? RuntimeStructs.ReadEvent(raw) : null;
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
}
