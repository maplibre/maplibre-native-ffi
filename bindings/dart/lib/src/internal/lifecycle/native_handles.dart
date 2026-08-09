/// A handle the C API issued.
///
/// The C API spells every handle as one integer type, so each kind gets its own
/// extension type here to keep the kinds distinct at compile time. The value
/// names one object for the life of the process, carries no ownership, and is
/// safe to copy, compare, hash, and send between isolates. Zero is the null
/// handle.
extension type const NativeHandle(int raw) implements Object {
  /// Whether this is the null handle.
  bool get isNull => raw == 0;
}

/// Runtime handle id.
extension type const NativeRuntime(int raw) implements NativeHandle {}

/// Map handle id.
extension type const NativeMap(int raw) implements NativeHandle {}

/// Map projection handle id.
extension type const NativeMapProjection(int raw) implements NativeHandle {}

/// Render session handle id.
extension type const NativeRenderSession(int raw) implements NativeHandle {}

/// Offline region snapshot handle id.
extension type const NativeOfflineRegionSnapshot(int raw)
    implements NativeHandle {}

/// Offline region list handle id.
extension type const NativeOfflineRegionList(int raw) implements NativeHandle {}

/// Style id list handle id.
extension type const NativeStyleIdList(int raw) implements NativeHandle {}

/// Style string list handle id.
extension type const NativeStyleStringList(int raw) implements NativeHandle {}

/// Wake source handle id.
extension type const NativeWakeSource(int raw) implements NativeHandle {}

/// Resource request handle id.
extension type const NativeResourceRequest(int raw) implements NativeHandle {}
