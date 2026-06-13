using System.Reflection;
using System.Runtime.InteropServices;
using Maplibre.Native.Internal.C;

namespace Maplibre.Native.Internal.Loader;

internal static class NativeLibraryLoader
{
    private const string LibraryPathSwitch = "Maplibre.Native.LibraryPath";
    private const string LibraryPathEnvironment = "MAPLIBRE_NATIVE_FFI_LIBRARY_PATH";
    private const string BuildDirEnvironment = "MLN_FFI_BUILD_DIR";

    private static readonly object Gate = new();
    private static bool installed;
    private static nint resolvedHandle;
    private static bool explicitlyLoaded;

    internal static void Load(string libraryPath)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(libraryPath);
        lock (Gate)
        {
            if (resolvedHandle != 0)
            {
                if (explicitlyLoaded)
                {
                    return;
                }

                throw new InvalidOperationException(
                    "The MapLibre Native library has already been resolved through the standard lookup order; call LoadNativeLibrary(path) before any native entry point."
                );
            }

            resolvedHandle = NativeLibrary.Load(libraryPath);
            explicitlyLoaded = true;
            EnsureLoadedLocked();
        }
    }

    internal static void EnsureLoaded()
    {
        lock (Gate)
        {
            EnsureLoadedLocked();
        }
    }

    private static nint ResolveLibrary(
        string libraryName,
        Assembly assembly,
        DllImportSearchPath? searchPath
    )
    {
        if (libraryName != NativeMethods.LibraryName)
        {
            return 0;
        }

        lock (Gate)
        {
            if (resolvedHandle != 0)
            {
                return resolvedHandle;
            }

            foreach (var path in CandidatePaths())
            {
                if (File.Exists(path) && NativeLibrary.TryLoad(path, out var handle))
                {
                    resolvedHandle = handle;
                    return resolvedHandle;
                }
            }

            if (NativeLibrary.TryLoad(libraryName, assembly, searchPath, out var standardHandle))
            {
                resolvedHandle = standardHandle;
                return resolvedHandle;
            }

            return 0;
        }
    }

    private static void EnsureLoadedLocked()
    {
        if (installed)
        {
            return;
        }

        NativeLibrary.SetDllImportResolver(typeof(NativeMethods).Assembly, ResolveLibrary);
        installed = true;
        ResolveLibrary(NativeMethods.LibraryName, typeof(NativeMethods).Assembly, null);
    }

    private static IEnumerable<string> CandidatePaths()
    {
        var switchPath = AppContext.GetData(LibraryPathSwitch) as string;
        if (!string.IsNullOrWhiteSpace(switchPath))
        {
            yield return switchPath;
        }

        var environmentPath = Environment.GetEnvironmentVariable(LibraryPathEnvironment);
        if (!string.IsNullOrWhiteSpace(environmentPath))
        {
            yield return environmentPath;
        }

        var buildDir = Environment.GetEnvironmentVariable(BuildDirEnvironment);
        if (!string.IsNullOrWhiteSpace(buildDir))
        {
            yield return Path.Combine(buildDir, PlatformLibraryFileName());
        }
    }

    internal static string PlatformLibraryFileName()
    {
        if (OperatingSystem.IsWindows())
        {
            return "maplibre-native-c.dll";
        }

        if (OperatingSystem.IsMacOS())
        {
            return "libmaplibre-native-c.dylib";
        }

        return "libmaplibre-native-c.so";
    }
}
