internal import CMaplibreNativeC
import Foundation

/// Mutable host-side receiver state owned by one native wake descriptor.
final class NativeWakeState: @unchecked Sendable {
  private let lock = NSLock()
  private var handler: (@Sendable () -> Void)?
  private var retainedContext: UnsafeMutableRawPointer?

  func setHandler(_ handler: (@Sendable () -> Void)?) {
    lock.withLock { self.handler = handler }
  }

  func makeDescriptor() -> mln_wake {
    let context = Unmanaged.passRetained(self).toOpaque()
    lock.withLock {
      precondition(retainedContext == nil)
      retainedContext = context
    }
    var wake = mln_wake()
    wake.size = UInt32(MemoryLayout<mln_wake>.size)
    wake.callback = { userData in
      guard let userData else { return }
      let state = Unmanaged<NativeWakeState>.fromOpaque(userData)
        .takeUnretainedValue()
      state.lock.withLock { state.handler }?()
    }
    wake.user_data = context
    wake.release_user_data = { userData in
      guard let userData else { return }
      let state = Unmanaged<NativeWakeState>.fromOpaque(userData)
        .takeUnretainedValue()
      state.lock.withLock { state.retainedContext = nil }
      Unmanaged<NativeWakeState>.fromOpaque(userData).release()
    }
    return wake
  }

  /// Releases a descriptor that an owning native call rejected.
  func releaseRejectedDescriptor() {
    let context = lock.withLock { () -> UnsafeMutableRawPointer? in
      defer { retainedContext = nil }
      return retainedContext
    }
    if let context {
      Unmanaged<NativeWakeState>.fromOpaque(context).release()
    }
  }
}
