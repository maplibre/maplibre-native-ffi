
internal import CMaplibreNativeC

public extension Maplibre {
  /// Loads a layer plugin shared library and registers its layer types.
  ///
  /// `path` names the plugin library and `entryPoint` names the function that
  /// the library exports. The loader opens the library, resolves the entry
  /// point, and calls it with the process-wide plugin register function, so
  /// the plugin binary never links this library.
  ///
  /// The call is process-wide: call it on any thread, before any style that
  /// uses the plugin's layer types loads. The library is never unloaded,
  /// because registration retains the plugin's callbacks for the process
  /// lifetime. Loading the same plugin again succeeds.
  ///
  /// Throws `MaplibreError` with kind `invalidArgument` when `path` or
  /// `entryPoint` is empty, and with kind `nativeError` when the operating
  /// system cannot load the library or resolve the entry point, or when the
  /// entry point reports a registration failure. The error's diagnostic
  /// carries the OS or plugin message.
  static func loadPlugin(path: String, entryPoint: String) throws {
    try mapNativeFailure {
      try NativeString.withStringView(path) { pathView in
        try NativeString.withStringView(entryPoint) { entryPointView in
          try checkStatus(mln_plugin_load_library(pathView, entryPointView))
        }
      }
    }
  }
}
