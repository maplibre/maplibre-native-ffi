using System.Reflection;
using System.Runtime.InteropServices;

namespace Maplibre.Native.Examples.DotnetMap;

internal static class NativeLibraryResolver
{
    private static readonly string[] NativeLibraryPathEnvironmentVariables =
    [
        OperatingSystem.IsWindows() ? "PATH" : OperatingSystem.IsMacOS() ? "DYLD_LIBRARY_PATH" : "LD_LIBRARY_PATH",
    ];
    private static readonly object RegistrationLock = new();
    private static bool registered;

    public static void Register()
    {
        lock (RegistrationLock)
        {
            if (registered)
            {
                return;
            }

            NativeLibrary.SetDllImportResolver(
                typeof(NativeLibraryResolver).Assembly,
                ResolveNativeLibrary
            );
            registered = true;
        }
    }

    public static string[] VulkanLibraryCandidates() => LibraryCandidates("vulkan").ToArray();

    private static nint ResolveNativeLibrary(
        string libraryName,
        Assembly assembly,
        DllImportSearchPath? searchPath
    )
    {
        foreach (var candidate in LibraryCandidates(libraryName))
        {
            if (NativeLibrary.TryLoad(candidate, assembly, searchPath, out var handle))
            {
                return handle;
            }
        }

        return 0;
    }

    private static IEnumerable<string> LibraryCandidates(string libraryName)
    {
        string[] names = libraryName switch
        {
            "glfw" when OperatingSystem.IsWindows() => ["glfw3.dll", "glfw3"],
            "glfw" when OperatingSystem.IsMacOS() => ["libglfw.3.dylib", "glfw"],
            "glfw" => ["libglfw.so.3", "glfw"],
            "vulkan" when OperatingSystem.IsWindows() => ["vulkan-1.dll", "vulkan-1"],
            "vulkan" when OperatingSystem.IsMacOS() =>
            [
                "libvulkan.1.dylib",
                "libvulkan.dylib",
                "libMoltenVK.dylib",
                "vulkan",
            ],
            "vulkan" => ["libvulkan.so.1", "vulkan"],
            "EGL" when OperatingSystem.IsLinux() => ["libEGL.so.1", "libEGL.so", "EGL"],
            _ => [],
        };

        foreach (var name in names)
        {
            foreach (var directory in CandidateLibraryDirectories())
            {
                var path = Path.Combine(directory, name);
                if (File.Exists(path))
                {
                    yield return path;
                }
            }

            yield return name;
        }
    }

    private static IEnumerable<string> CandidateLibraryDirectories()
    {
        foreach (var environmentVariable in NativeLibraryPathEnvironmentVariables)
        {
            var rawValue = Environment.GetEnvironmentVariable(environmentVariable);
            if (string.IsNullOrWhiteSpace(rawValue))
            {
                continue;
            }

            foreach (var directory in rawValue.Split(Path.PathSeparator))
            {
                if (!string.IsNullOrWhiteSpace(directory) && Directory.Exists(directory))
                {
                    yield return directory;
                }
            }
        }
    }
}
