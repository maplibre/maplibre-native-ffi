using Maplibre.NativeFfi.Internal.Loader;

namespace Maplibre.NativeFfi;

/// <summary>Process-global MapLibre Native FFI entry points.</summary>
/// <remarks>
/// A handle that the collector reclaims before it is closed or disposed is reported as a leak
/// through <see cref="System.Diagnostics.Trace.TraceWarning(string)" />, or on standard error
/// when the host has added no trace listener.
/// </remarks>
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
