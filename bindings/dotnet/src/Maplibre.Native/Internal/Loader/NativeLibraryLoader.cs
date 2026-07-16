using System.Reflection;
using System.Runtime.InteropServices;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;

namespace Maplibre.Native.Internal.Loader;

internal static class NativeLibraryLoader
{
    internal const uint ExpectedAbiVersion = 0;

    private const string LibraryPathSwitch = "Maplibre.Native.LibraryPath";
    private const string LibraryDirsSwitch = "Maplibre.Native.LibraryDirs";
    private const string LibraryPathEnvironment = "MAPLIBRE_NATIVE_FFI_LIBRARY_PATH";
    private const string LibraryDirsEnvironment = "MAPLIBRE_NATIVE_FFI_LIBRARY_DIRS";

    private static readonly object Gate = new();
    private static readonly HashSet<string> LoadedRuntimeDependencies = new(StringComparer.Ordinal);
    private static bool installed;
    private static bool abiValidated;
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
                    ValidateLoadedAbiLocked();
                    return;
                }

                throw new InvalidOperationException(
                    "The MapLibre Native library has already been resolved through the standard lookup order; call LoadNativeLibrary(path) before any native entry point."
                );
            }

            nint handle;
            try
            {
                LoadRuntimeDependenciesForPath(libraryPath);
                handle = NativeLibrary.Load(libraryPath);
            }
            catch (Exception error) when (error is DllNotFoundException or BadImageFormatException)
            {
                throw new UnsupportedFeatureException(
                    MaplibreStatus.Unsupported,
                    null,
                    $"MapLibre Native C library could not be loaded from '{libraryPath}'.",
                    error
                );
            }
            try
            {
                resolvedHandle = ValidateAndCacheResolvedHandle(handle);
            }
            catch
            {
                resolvedHandle = 0;
                explicitlyLoaded = false;
                abiValidated = false;
                throw;
            }
            explicitlyLoaded = true;
            EnsureResolverInstalledLocked();
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
                LoadRuntimeDependenciesForPath(path);
                if (File.Exists(path) && NativeLibrary.TryLoad(path, out var handle))
                {
                    return ValidateAndCacheResolvedHandle(handle);
                }
            }

            LoadRuntimeDependencies();
            if (NativeLibrary.TryLoad(libraryName, assembly, searchPath, out var standardHandle))
            {
                return ValidateAndCacheResolvedHandle(standardHandle);
            }

            return 0;
        }
    }

    private static void EnsureLoadedLocked()
    {
        EnsureResolverInstalledLocked();
        ResolveLibrary(NativeMethods.LibraryName, typeof(NativeMethods).Assembly, null);
        ValidateLoadedAbiLocked();
    }

    private static void EnsureResolverInstalledLocked()
    {
        if (installed)
        {
            return;
        }

        NativeLibrary.SetDllImportResolver(typeof(NativeMethods).Assembly, ResolveLibrary);
        installed = true;
    }

    private static unsafe uint ReadAbiVersion(nint handle)
    {
        try
        {
            var version = NativeLibrary.GetExport(handle, "mln_c_version");
            return ((delegate* unmanaged[Cdecl]<uint>)version)();
        }
        catch (EntryPointNotFoundException error)
        {
            throw new MaplibreException(
                MaplibreStatus.AbiMismatch,
                null,
                "MapLibre Native C library does not export the required mln_c_version entry point.",
                error
            );
        }
    }

    private static nint ValidateAndCacheResolvedHandle(nint handle)
    {
        try
        {
            ValidateAbiVersion(ReadAbiVersion(handle));
        }
        catch
        {
            NativeLibrary.Free(handle);
            throw;
        }

        resolvedHandle = handle;
        abiValidated = true;
        return resolvedHandle;
    }

    private static void ValidateLoadedAbiLocked()
    {
        if (abiValidated)
        {
            return;
        }

        uint actualVersion;
        try
        {
            actualVersion = NativeMethods.mln_c_version();
        }
        catch (DllNotFoundException error)
        {
            throw new UnsupportedFeatureException(
                MaplibreStatus.Unsupported,
                null,
                "MapLibre Native C library could not be loaded.",
                error
            );
        }
        catch (EntryPointNotFoundException error)
        {
            throw new MaplibreException(
                MaplibreStatus.AbiMismatch,
                null,
                "MapLibre Native C library does not export the required mln_c_version entry point.",
                error
            );
        }

        ValidateAbiVersion(actualVersion);
        abiValidated = true;
    }

    internal static void ValidateAbiVersion(uint actualVersion)
    {
        if (actualVersion == ExpectedAbiVersion)
        {
            return;
        }

        throw new MaplibreException(
            MaplibreStatus.AbiMismatch,
            null,
            $"MapLibre Native C ABI version {actualVersion} is incompatible with this binding; expected {ExpectedAbiVersion}.",
            null
        );
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
    }

    private static void LoadRuntimeDependencies()
    {
        foreach (var directory in CandidateLibraryDirs())
        {
            LoadRuntimeDependencies(directory);
        }
    }

    private static void LoadRuntimeDependenciesForPath(string libraryPath)
    {
        var directory = Path.GetDirectoryName(libraryPath);
        if (!string.IsNullOrWhiteSpace(directory))
        {
            LoadRuntimeDependencies(directory);
        }

        LoadRuntimeDependencies();
    }

    private static void LoadRuntimeDependencies(string directory)
    {
        foreach (var fileName in RuntimeDependencyFileNames())
        {
            var path = Path.Combine(directory, fileName);
            if (!File.Exists(path))
            {
                continue;
            }

            var fullPath = Path.GetFullPath(path);
            if (!LoadedRuntimeDependencies.Add(fullPath))
            {
                continue;
            }

            try
            {
                NativeLibrary.Load(fullPath);
            }
            catch (Exception error) when (error is DllNotFoundException or BadImageFormatException)
            {
                LoadedRuntimeDependencies.Remove(fullPath);
                throw new UnsupportedFeatureException(
                    MaplibreStatus.Unsupported,
                    null,
                    $"MapLibre Native runtime dependency could not be loaded from '{fullPath}'.",
                    error
                );
            }
        }
    }

    private static IEnumerable<string> CandidateLibraryDirs()
    {
        foreach (var directory in SplitLibraryDirs(AppContext.GetData(LibraryDirsSwitch) as string))
        {
            yield return directory;
        }

        foreach (
            var directory in SplitLibraryDirs(
                Environment.GetEnvironmentVariable(LibraryDirsEnvironment)
            )
        )
        {
            yield return directory;
        }
    }

    private static IEnumerable<string> SplitLibraryDirs(string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            yield break;
        }

        foreach (
            var directory in value.Split(
                Path.PathSeparator,
                StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries
            )
        )
        {
            yield return directory;
        }
    }

    private static string[] RuntimeDependencyFileNames()
    {
        if (OperatingSystem.IsWindows())
        {
            return ["vulkan-1.dll", "libEGL.dll", "libGLESv2.dll", "EGL.dll", "GLESv2.dll"];
        }

        if (OperatingSystem.IsMacOS())
        {
            return ["libvulkan.1.dylib", "libvulkan.dylib", "libEGL.dylib", "libGLESv2.dylib"];
        }

        return
        [
            "libvulkan.so.1",
            "libvulkan.so",
            "libEGL.so.1",
            "libEGL.so",
            "libGLESv2.so.2",
            "libGLESv2.so",
        ];
    }
}
