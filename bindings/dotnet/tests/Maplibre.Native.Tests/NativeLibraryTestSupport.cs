using Xunit;

namespace Maplibre.Native.Tests;

internal static class NativeLibraryTestSupport
{
    internal static void SkipUnlessNativeLibraryIsAvailable()
    {
        var buildDir = Environment.GetEnvironmentVariable("MLN_FFI_BUILD_DIR");
        if (string.IsNullOrWhiteSpace(buildDir))
        {
            Assert.Skip("MLN_FFI_BUILD_DIR is not set; run through mise for native-library tests.");
        }

        var libraryPath = Path.Combine(buildDir!, PlatformLibraryFileName());
        if (!File.Exists(libraryPath))
        {
            Assert.Skip($"Native library is not built at {libraryPath}.");
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
