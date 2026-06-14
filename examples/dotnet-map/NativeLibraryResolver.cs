using System.Reflection;
using System.Runtime.InteropServices;

namespace Maplibre.Native.Examples.DotnetMap;

internal static class NativeLibraryResolver
{
    private static bool registered;

    public static void Register()
    {
        if (registered)
        {
            return;
        }

        registered = true;
        NativeLibrary.SetDllImportResolver(
            typeof(NativeLibraryResolver).Assembly,
            ResolveNativeLibrary
        );
    }

    private static nint ResolveNativeLibrary(
        string libraryName,
        Assembly assembly,
        DllImportSearchPath? searchPath
    )
    {
        string[] candidates = libraryName switch
        {
            "glfw" when OperatingSystem.IsWindows() => ["glfw3"],
            "glfw" when OperatingSystem.IsMacOS() => ["libglfw.3.dylib", "glfw"],
            "glfw" => ["libglfw.so.3", "glfw"],
            "vulkan" when OperatingSystem.IsWindows() => ["vulkan-1"],
            "vulkan" when OperatingSystem.IsMacOS() =>
            [
                "libvulkan.1.dylib",
                "libvulkan.dylib",
                "libMoltenVK.dylib",
                "vulkan",
            ],
            "vulkan" => ["libvulkan.so.1", "vulkan"],
            _ => [],
        };

        foreach (var candidate in candidates)
        {
            if (NativeLibrary.TryLoad(candidate, assembly, searchPath, out var handle))
            {
                return handle;
            }
        }

        return 0;
    }
}
