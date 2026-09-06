using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Loader;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Internal.Callback;

internal sealed unsafe class CustomMvtVectorSourceState : IDisposable
{
    private readonly Lock gate = new();
    private readonly CustomMvtVectorSourceOptions options;
    private readonly CustomMvtVectorSourceCallback fetchTile;
    private readonly CustomMvtVectorSourceCallback? cancelTile;
    private GCHandle handle;
    private bool closed;
    private bool handleFreed;
    private int activeCallbacks;

    internal CustomMvtVectorSourceState(CustomMvtVectorSourceOptions options)
    {
        this.options = options ?? throw new ArgumentNullException(nameof(options));
        fetchTile =
            options.FetchTile
            ?? throw new ArgumentException(
                "Custom MVT vector source FetchTile callback is required.",
                nameof(options)
            );
        cancelTile = options.CancelTile;
        handle = GCHandle.Alloc(this);
    }

    internal mln_custom_mvt_vector_source_options Descriptor
    {
        get
        {
            NativeLibraryLoader.EnsureLoaded();
            var descriptor = NativeMethods.mln_custom_mvt_vector_source_options_default();
            descriptor.fetch_tile = &FetchTile;
            descriptor.cancel_tile = &CancelTile;
            descriptor.release_user_data = &ReleaseUserData;
            descriptor.user_data = (void*)GCHandle.ToIntPtr(handle);
            if (options.MinimumZoom is { } minimumZoom)
            {
                descriptor.fields |= (uint)
                    mln_custom_mvt_vector_source_option_field.MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
                descriptor.min_zoom = minimumZoom;
            }
            if (options.MaximumZoom is { } maximumZoom)
            {
                descriptor.fields |= (uint)
                    mln_custom_mvt_vector_source_option_field.MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
                descriptor.max_zoom = maximumZoom;
            }
            return descriptor;
        }
    }

    internal bool IsHandleAllocatedForTest => handle.IsAllocated;

    internal void FetchForTest(CanonicalTileId tileId) => InvokeFetch(tileId);

    internal void CancelForTest(CanonicalTileId tileId) => InvokeCancel(tileId);

    [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
    private static void FetchTile(void* userData, mln_canonical_tile_id tileId)
    {
        var state = FromUserData(userData);
        state?.InvokeFetch(FromNative(tileId));
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
    private static void CancelTile(void* userData, mln_canonical_tile_id tileId)
    {
        var state = FromUserData(userData);
        state?.InvokeCancel(FromNative(tileId));
    }

    // The C API invokes this once, on the runtime's native scheduler thread, after MapLibre stops referencing this
    // state, so the stubs go with the source whether it was removed, dropped by a style load, or
    // retired with the map.
    [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
    private static void ReleaseUserData(void* userData)
    {
        var state = FromUserData(userData);
        state?.Dispose();
    }

    private static CustomMvtVectorSourceState? FromUserData(void* userData)
    {
        try
        {
            return (CustomMvtVectorSourceState?)GCHandle.FromIntPtr((nint)userData).Target;
        }
        catch
        {
            return null;
        }
    }

    private static CanonicalTileId FromNative(mln_canonical_tile_id tileId) =>
        new(tileId.z, tileId.x, tileId.y);

    private void InvokeFetch(CanonicalTileId tileId)
    {
        if (!EnterCallback())
        {
            return;
        }

        try
        {
            fetchTile.Invoke(tileId);
        }
        catch
        {
            // Native callbacks must not unwind through the C ABI.
        }
        finally
        {
            ExitCallback();
        }
    }

    private void InvokeCancel(CanonicalTileId tileId)
    {
        if (!EnterCallback())
        {
            return;
        }

        try
        {
            cancelTile?.Invoke(tileId);
        }
        catch
        {
            // Native callbacks must not unwind through the C ABI.
        }
        finally
        {
            ExitCallback();
        }
    }

    private bool EnterCallback()
    {
        lock (gate)
        {
            if (closed)
            {
                return false;
            }

            activeCallbacks++;
            return true;
        }
    }

    private void ExitCallback()
    {
        lock (gate)
        {
            activeCallbacks--;
            FreeHandleIfReadyLocked();
        }
    }

    public void Dispose()
    {
        lock (gate)
        {
            closed = true;
            FreeHandleIfReadyLocked();
        }
    }

    private void FreeHandleIfReadyLocked()
    {
        if (!closed || activeCallbacks != 0 || handleFreed || !handle.IsAllocated)
        {
            return;
        }

        handle.Free();
        handleFreed = true;
    }
}
