using Maplibre.Native.Error;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Internal.Loader;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Log;

namespace Maplibre.Native;

/// <summary>Process-global MapLibre Native FFI entry points.</summary>
public static unsafe class Maplibre
{
    /// <summary>Loads the native library using the binding's standard lookup order.</summary>
    public static void LoadNativeLibrary()
    {
        NativeLibraryLoader.EnsureLoaded();
    }

    /// <summary>Loads the native library from an exact file path.</summary>
    public static void LoadNativeLibrary(string libraryPath)
    {
        NativeLibraryLoader.Load(libraryPath);
    }

    /// <summary>Returns the native C ABI contract version.</summary>
    public static uint CVersion()
    {
        NativeLibraryLoader.EnsureLoaded();
        return NativeMethods.mln_c_version();
    }

    /// <summary>Returns the render backends compiled into the native library.</summary>
    public static Render.RenderBackend SupportedRenderBackends()
    {
        NativeLibraryLoader.EnsureLoaded();
        return (Render.RenderBackend)NativeMethods.mln_supported_render_backend_mask();
    }

    /// <summary>Returns the OpenGL context providers compiled into the native library.</summary>
    public static Render.OpenGLContextProvider SupportedOpenGLContextProviders()
    {
        NativeLibraryLoader.EnsureLoaded();
        return (Render.OpenGLContextProvider)
            NativeMethods.mln_opengl_supported_context_provider_mask();
    }

    /// <summary>Reads MapLibre Native's process-global network status.</summary>
    public static global::Maplibre.Native.NetworkStatus NetworkStatus()
    {
        NativeLibraryLoader.EnsureLoaded();
        uint status = 0;
        NativeStatus.Check(NativeMethods.mln_network_status_get(&status));
        return global::Maplibre.Native.NetworkStatus.FromRaw(status);
    }

    /// <summary>Sets MapLibre Native's process-global network status.</summary>
    public static void SetNetworkStatus(global::Maplibre.Native.NetworkStatus status)
    {
        ArgumentNullException.ThrowIfNull(status);
        if (!status.IsKnown)
        {
            throw new InvalidArgumentException(
                MaplibreStatus.InvalidArgument,
                null,
                $"Unknown network status value {status.RawValue} cannot be set."
            );
        }

        NativeLibraryLoader.EnsureLoaded();
        NativeStatus.Check(NativeMethods.mln_network_status_set(status.RawValue));
    }

    /// <summary>Installs or replaces the process-global native log callback.</summary>
    public static void SetLogCallback(LogCallback callback)
    {
        LogCallbackState.Set(callback);
    }

    /// <summary>Clears the process-global native log callback.</summary>
    public static void ClearLogCallback()
    {
        LogCallbackState.Clear();
    }

    /// <summary>Configures severities that native logging may dispatch asynchronously.</summary>
    public static void SetAsyncLogSeverities(LogSeverityMask severities)
    {
        NativeLibraryLoader.EnsureLoaded();
        NativeStatus.Check(NativeMethods.mln_log_set_async_severity_mask((uint)severities));
    }

    /// <summary>Restores the native default async log severity mask.</summary>
    public static void RestoreDefaultAsyncLogSeverities()
    {
        NativeLibraryLoader.EnsureLoaded();
        NativeStatus.Check(
            NativeMethods.mln_log_set_async_severity_mask(
                (uint)mln_log_severity_mask.MLN_LOG_SEVERITY_MASK_DEFAULT
            )
        );
    }

    /// <summary>Converts a geographic coordinate to Spherical Mercator projected meters.</summary>
    public static ProjectedMeters ProjectedMetersForLatLng(LatLng coordinate)
    {
        NativeLibraryLoader.EnsureLoaded();
        var output = new mln_projected_meters();
        NativeStatus.Check(
            NativeMethods.mln_projected_meters_for_lat_lng(
                CoreStructs.ToNative(coordinate),
                &output
            )
        );
        return CoreStructs.FromNative(output);
    }

    /// <summary>Converts Spherical Mercator projected meters to a geographic coordinate.</summary>
    public static LatLng LatLngForProjectedMeters(ProjectedMeters meters)
    {
        NativeLibraryLoader.EnsureLoaded();
        var output = new mln_lat_lng();
        NativeStatus.Check(
            NativeMethods.mln_lat_lng_for_projected_meters(CoreStructs.ToNative(meters), &output)
        );
        return CoreStructs.FromNative(output);
    }
}
