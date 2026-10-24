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
    /// it should return quickly. The binding refuses every native call that a handler makes with
    /// <see cref="InvalidOperationException" />. An exception that a handler throws is discarded.
    /// </remarks>
    public static event EventHandler<CallbackExceptionEventArgs>? CallbackException;

    internal static EventHandler<CallbackExceptionEventArgs>? CallbackExceptionHandlers =>
        CallbackException;

    /// <summary>
    /// Raised when the collector reclaims a handle that was never closed or disposed. With no
    /// handler, the binding writes the report to standard error.
    /// </summary>
    /// <remarks>
    /// A handler runs on the finalizer thread, so it should return quickly. An exception that a
    /// handler throws is discarded.
    /// </remarks>
    public static event EventHandler<LeakReportedEventArgs>? LeakReported;

    internal static EventHandler<LeakReportedEventArgs>? LeakReportedHandlers => LeakReported;

    /// <summary>
    /// Loads the native library and checks its C ABI version. The library is the one that .NET's
    /// native library probing finds, such as a runtime package's.
    /// </summary>
    public static void LoadNativeLibrary()
    {
        NativeLibraryLoader.EnsureLoaded();
    }

    /// <summary>
    /// Loads the native library from an exact file path. Call it before any other native call,
    /// which otherwise loads the library through <see cref="LoadNativeLibrary()" />.
    /// </summary>
    public static void LoadNativeLibrary(string libraryPath)
    {
        NativeLibraryLoader.Load(libraryPath);
    }
}
