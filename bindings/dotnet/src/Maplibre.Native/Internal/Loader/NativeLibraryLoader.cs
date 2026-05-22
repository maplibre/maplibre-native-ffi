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

    internal static void EnsureLoaded()
    {
        if (installed)
        {
            return;
        }

        lock (Gate)
        {
            if (installed)
            {
                return;
            }

            NativeLibrary.SetDllImportResolver(
                typeof(NativeMethods).Assembly,
                ResolveLibrary);
            installed = true;
        }
    }

    private static nint ResolveLibrary(
        string libraryName,
        Assembly assembly,
        DllImportSearchPath? searchPath)
    {
        if (libraryName != NativeMethods.LibraryName)
        {
            return 0;
        }

        foreach (var path in CandidatePaths())
        {
            if (File.Exists(path) && NativeLibrary.TryLoad(path, out var handle))
            {
                return handle;
            }
        }

        return 0;
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

    private static string PlatformLibraryFileName()
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
