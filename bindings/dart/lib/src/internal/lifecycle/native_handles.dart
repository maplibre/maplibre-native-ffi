/// A handle the C API issued.
///
/// The C API spells every handle as one integer type, so each kind gets its own
/// extension type, generated alongside its owner in generated_operations.dart,
/// to keep the kinds distinct at compile time. The value names one object for
/// the life of the process, carries no ownership, and is safe to copy, compare,
/// hash, and send between isolates. Zero is the null handle.
extension type const NativeHandle(int raw) implements Object {
  /// Whether this is the null handle.
  bool get isNull => raw == 0;
}
