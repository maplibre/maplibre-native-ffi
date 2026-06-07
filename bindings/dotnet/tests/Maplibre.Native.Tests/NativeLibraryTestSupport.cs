using Xunit;

namespace Maplibre.Native.Tests;

internal static class NativeLibraryTestSupport
{
    internal static void SkipUnlessNativeLibraryIsAvailable()
    {
        var isCi = string.Equals(
            Environment.GetEnvironmentVariable("CI"),
            "true",
            StringComparison.OrdinalIgnoreCase
        );
        var buildDir = Environment.GetEnvironmentVariable("MLN_FFI_BUILD_DIR");
        if (string.IsNullOrWhiteSpace(buildDir))
        {
            if (isCi)
            {
                Assert.Fail("MLN_FFI_BUILD_DIR is not set in CI.");
            }

            Assert.Skip("MLN_FFI_BUILD_DIR is not set; run through mise for native-library tests.");
        }

        var libraryPath = Path.Combine(buildDir!, PlatformLibraryFileName());
        if (!File.Exists(libraryPath))
        {
            if (isCi)
            {
                Assert.Fail($"Native library is not built at {libraryPath}.");
            }

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
