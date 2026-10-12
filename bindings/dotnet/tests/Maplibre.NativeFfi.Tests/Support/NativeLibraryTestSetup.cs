using System.Runtime.CompilerServices;

namespace Maplibre.NativeFfi.Tests;

internal static class NativeLibraryTestSetup
{
    [ModuleInitializer]
    internal static void LoadNativeLibrary()
    {
        global::Maplibre.NativeFfi.Maplibre.LoadNativeLibrary();
    }
}
