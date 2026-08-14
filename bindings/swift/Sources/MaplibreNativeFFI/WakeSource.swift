import Foundation

internal import CMaplibreNativeC

/// Releases a runtime owner thread parked in
/// `RuntimeHandle.pump(timeout:budget:)`.
///
/// A wake source is usable from any thread, and stays usable after its runtime
/// closes. The unchecked conformance rests on `NativeHandleState` ordering
/// signal against close, and on the C API allowing signal and destroy from any
/// thread.
public final class WakeSource: @unchecked Sendable {
  private let state: NativeHandleState<NativeWakeSourceHandle>

  init(handle: NativeWakeSourceHandle) throws {
    state = try NativeHandleState(typeName: "WakeSource", handle: handle)
  }

  public var isClosed: Bool {
    state.isClosed
  }

  /// Sets the runtime's wake flag and releases the parked owner thread. A
  /// signal raised while the owner thread runs makes the next
  /// `RuntimeHandle.pump(timeout:budget:)` return without parking. Signalling
  /// after
  /// the runtime closes succeeds and does nothing.
  public func signal() throws {
    try mapNativeFailure {
      try state.withLive { source in
        try checkStatus(mln_wake_source_signal(source.raw))
      }
    }
  }

  /// Releases the wake source.
  public func close() throws {
    try mapNativeFailure {
      try state.closeOnce { source in
        mln_wake_source_destroy(source.raw)
      }
    }
  }
}
