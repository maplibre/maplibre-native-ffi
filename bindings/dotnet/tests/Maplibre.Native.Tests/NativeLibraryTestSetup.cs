using System.Runtime.CompilerServices;

namespace Maplibre.Native.Tests;

internal static class NativeLibraryTestSetup
{
    [ModuleInitializer]
    internal static void LoadNativeLibrary()
    {
        global::Maplibre.Native.Maplibre.LoadNativeLibrary();
    }
}
