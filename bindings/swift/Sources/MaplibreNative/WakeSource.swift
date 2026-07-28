internal import CMaplibreNativeC

/// Releases a runtime owner thread parked in `RuntimeHandle.pump(timeout:)`.
///
/// A wake source is usable from any thread, which a host's task submission and
/// shutdown paths rely on. It stays usable after its runtime closes, and
/// signalling it then does nothing.
///
/// The unchecked conformance rests on two invariants: `NativeHandleState`
/// serializes pointer reads against close under its own lock, and the C API
/// documents `mln_wake_source_signal` and `mln_wake_source_destroy` as callable
/// from any thread against wake state that carries its own synchronization.
public final class WakeSource: @unchecked Sendable {
  private let state: NativeHandleState

  init(pointer: OpaquePointer?) throws {
    state = try NativeHandleState(typeName: "WakeSource", pointer: pointer)
  }

  public var isClosed: Bool {
    state.isClosed
  }

  /// Sets the runtime's wake flag and releases the parked owner thread.
  ///
  /// A signal raised while the owner thread is running sets the wake flag, so
  /// the next `RuntimeHandle.pump(timeout:)` returns without parking.
  /// Signalling after the runtime closes succeeds and does nothing.
  public func signal() throws {
    try mapNativeFailure {
      try checkStatus(mln_wake_source_signal(state.requireLive()))
    }
  }

  /// Releases the wake source.
  public func close() throws {
    try mapNativeFailure {
      try state.closeOnce { pointer in
        mln_wake_source_destroy(pointer)
      }
    }
  }
}
