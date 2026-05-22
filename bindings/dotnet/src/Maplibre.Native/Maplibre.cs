using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Loader;
using Maplibre.Native.Internal.Status;

namespace Maplibre.Native;

/// <summary>Process-global MapLibre Native FFI entry points.</summary>
public static class Maplibre
{
    /// <summary>Returns the native C ABI contract version.</summary>
    public static uint CVersion()
    {
        NativeLibraryLoader.EnsureLoaded();
        return NativeMethods.MlnCVersion();
    }

    /// <summary>Returns the render backends compiled into the native library.</summary>
    public static RenderBackend SupportedRenderBackends()
    {
        NativeLibraryLoader.EnsureLoaded();
        return (RenderBackend)NativeMethods.MlnSupportedRenderBackendMask();
    }

    /// <summary>Reads MapLibre Native's process-global network status.</summary>
    public static global::Maplibre.Native.NetworkStatus NetworkStatus()
    {
        NativeLibraryLoader.EnsureLoaded();
        NativeStatus.Check(NativeMethods.MlnNetworkStatusGet(out var status));
        return global::Maplibre.Native.NetworkStatus.FromRaw(status);
    }

    /// <summary>Sets MapLibre Native's process-global network status.</summary>
    public static void SetNetworkStatus(global::Maplibre.Native.NetworkStatus status)
    {
        ArgumentNullException.ThrowIfNull(status);
        if (!status.IsKnown)
        {
            throw new InvalidArgumentException(
                MaplibreStatus.Unknown,
                null,
                $"Unknown network status value {status.RawValue} cannot be set.");
        }

        NativeLibraryLoader.EnsureLoaded();
        NativeStatus.Check(NativeMethods.MlnNetworkStatusSet(status.RawValue));
    }
}
