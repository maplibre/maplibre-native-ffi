internal import CMaplibreNativeC

public extension Maplibre {
  /// Returns the process-lifetime address of the v1 plugin registration
  /// function.
  /// Pass it to the plugin's own registration entry point before loading
  /// dependent styles.
  static func pluginRegisterFunctionV1() -> NativePointer {
    NativePointer(bitPattern: unsafeBitCast(
      mln_plugin_get_register_function_v1()!,
      to: UInt.self
    ))
  }
}
