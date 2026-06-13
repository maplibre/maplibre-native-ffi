using System.Runtime.CompilerServices;
using Maplibre.Native.Internal.Loader;

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

        var libraryPath = Path.Combine(buildDir, NativeLibraryLoader.PlatformLibraryFileName());
        if (!File.Exists(libraryPath))
        {
            throw new FileNotFoundException("Native library is not built.", libraryPath);
        }

        global::Maplibre.Native.Maplibre.LoadNativeLibrary(libraryPath);
    }
}
