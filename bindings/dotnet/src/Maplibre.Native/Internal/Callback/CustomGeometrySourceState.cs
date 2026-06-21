using System.Runtime.InteropServices;
using Maplibre.Native.Error;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Loader;
using Maplibre.Native.Style;

namespace Maplibre.Native.Internal.Callback;

internal sealed unsafe class CustomGeometrySourceState : IDisposable
{
    private readonly Lock gate = new();
    private readonly CustomGeometrySourceOptions options;
    private readonly CustomGeometrySourceCallback fetchTile;
    private readonly CustomGeometrySourceCallback? cancelTile;
    private GCHandle handle;
    private bool closed;
    private bool handleFreed;
    private int activeCallbacks;

    internal CustomGeometrySourceState(CustomGeometrySourceOptions options)
    {
        this.options = options ?? throw new ArgumentNullException(nameof(options));
        fetchTile =
            options.FetchTile
            ?? throw new ArgumentException(
                "Custom geometry source FetchTile callback is required.",
                nameof(options)
            );
        cancelTile = options.CancelTile;
        handle = GCHandle.Alloc(this);
    }

    internal mln_custom_geometry_source_options Descriptor
    {
        get
        {
            ValidateDescriptorOptions();
            NativeLibraryLoader.EnsureLoaded();
            var descriptor = NativeMethods.mln_custom_geometry_source_options_default();
            descriptor.fetch_tile = &FetchTile;
            descriptor.cancel_tile = &CancelTile;
            descriptor.user_data = (void*)GCHandle.ToIntPtr(handle);
            if (options.MinimumZoom is { } minimumZoom)
            {
                descriptor.fields |= (uint)
                    mln_custom_geometry_source_option_field.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM;
                descriptor.min_zoom = minimumZoom;
            }
            if (options.MaximumZoom is { } maximumZoom)
            {
                descriptor.fields |= (uint)
                    mln_custom_geometry_source_option_field.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM;
                descriptor.max_zoom = maximumZoom;
            }
            if (options.Tolerance is { } tolerance)
            {
                descriptor.fields |= (uint)
                    mln_custom_geometry_source_option_field.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE;
                descriptor.tolerance = tolerance;
            }
            if (options.TileSize is { } tileSize)
            {
                descriptor.fields |= (uint)
                    mln_custom_geometry_source_option_field.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE;
                descriptor.tile_size = tileSize;
            }
            if (options.Buffer is { } buffer)
            {
                descriptor.fields |= (uint)
                    mln_custom_geometry_source_option_field.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER;
                descriptor.buffer = (uint)buffer;
            }
            if (options.Clip is { } clip)
            {
                descriptor.fields |= (uint)
                    mln_custom_geometry_source_option_field.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP;
                descriptor.clip = clip ? (byte)1 : (byte)0;
            }
            if (options.Wrap is { } wrap)
            {
                descriptor.fields |= (uint)
                    mln_custom_geometry_source_option_field.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
                descriptor.wrap = wrap ? (byte)1 : (byte)0;
            }
            return descriptor;
        }
    }

    private void ValidateDescriptorOptions()
    {
        if (options.Buffer < 0)
        {
            throw new InvalidArgumentException(
                MaplibreStatus.InvalidArgument,
                null,
                "Custom geometry source buffer must be non-negative.",
                null
            );
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

    private static CustomGeometrySourceState? FromUserData(void* userData)
    {
        try
        {
            return (CustomGeometrySourceState?)GCHandle.FromIntPtr((nint)userData).Target;
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
