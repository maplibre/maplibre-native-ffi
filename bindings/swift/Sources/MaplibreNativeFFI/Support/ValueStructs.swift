internal import CMaplibreNativeC
import Foundation

/// Owns per-call byte storage whose pointers stay valid until the C call
/// returns.
final class NativeInputArena {
  private var buffers: [ContiguousArray<UInt8>] = []

  func view(_ text: String) -> mln_buffer_view {
    view(Data(text.utf8))
  }

  func view(_ data: Data) -> mln_buffer_view {
    buffers.append(ContiguousArray(data))
    return buffers.withUnsafeBufferPointer { storage in
      storage[storage.count - 1].withUnsafeBufferPointer { bytes in
        mln_buffer_view(data: bytes.baseAddress, size: bytes.count)
      }
    }
  }
}
