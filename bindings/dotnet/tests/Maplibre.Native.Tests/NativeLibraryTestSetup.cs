using System.Runtime.CompilerServices;

namespace Maplibre.Native.Tests;

internal static class NativeLibraryTestSetup
{
    [ModuleInitializer]
    internal static void LoadNativeLibrary()
    {
        var buildDir = Environment.GetEnvironmentVariable("MLN_FFI_BUILD_DIR");
        if (string.IsNullOrWhiteSpace(buildDir))
        {
            throw new InvalidOperationException(
                "MLN_FFI_BUILD_DIR is required; run .NET native-library tests through mise."
            );
        }

        var libraryPath = Path.Combine(buildDir, PlatformLibraryFileName());
        if (!File.Exists(libraryPath))
        {
            throw new FileNotFoundException("Native library is not built.", libraryPath);
        }

        global::Maplibre.Native.Maplibre.LoadNativeLibrary(libraryPath);
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
