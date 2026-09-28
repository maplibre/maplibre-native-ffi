using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Internal.Loader;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Map;

namespace Maplibre.NativeFfi;

/// <summary>Process-global MapLibre Native FFI entry points.</summary>
public static unsafe partial class Maplibre
{
    /// <summary>Loads the native library using the binding's standard lookup order.</summary>
    public static void LoadNativeLibrary()
    {
        NativeLibraryLoader.EnsureLoaded();
    }

    /// <summary>Loads the native library from an exact file path.</summary>
    public static void LoadNativeLibrary(string libraryPath)
    {
        NativeLibraryLoader.Load(libraryPath);
    }
}
