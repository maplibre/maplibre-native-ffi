using Maplibre.NativeFfi.Internal.Loader;

namespace Maplibre.NativeFfi;

/// <summary>Process-global MapLibre Native FFI entry points.</summary>
public static unsafe partial class Maplibre
{
    /// <summary>
    /// Raised when a native callback throws, or when the binding cannot convert a callback's
    /// arguments. Native cannot receive the exception, so it receives the callback's failure value
    /// instead. With no handler, the binding writes the exception to standard error.
    /// </summary>
    /// <remarks>
    /// A handler runs on the native thread that called the callback, before native continues, so
    /// it should return quickly. An exception that a handler throws is discarded.
    /// </remarks>
    public static event EventHandler<CallbackExceptionEventArgs>? CallbackException;

    internal static EventHandler<CallbackExceptionEventArgs>? CallbackExceptionHandlers =>
        CallbackException;

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
