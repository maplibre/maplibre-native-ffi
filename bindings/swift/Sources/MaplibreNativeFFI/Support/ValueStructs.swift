internal import CMaplibreNativeC
import Foundation

/// Owns per-call byte storage whose pointers stay valid until the C call
/// returns.
final class NativeInputArena {
  private var buffers: [UnsafeMutableRawBufferPointer] = []

  deinit {
    for buffer in buffers {
      buffer.deallocate()
    }
  }

  func view(_ text: String) -> mln_buffer_view {
    view(Data(text.utf8))
  }

  func view(_ data: Data) -> mln_buffer_view {
    guard !data.isEmpty else {
      return mln_buffer_view(data: nil, size: 0)
    }
    let buffer = UnsafeMutableRawBufferPointer.allocate(
      byteCount: data.count,
      alignment: MemoryLayout<UInt8>.alignment
    )
    data.copyBytes(to: buffer)
    buffers.append(buffer)
    return mln_buffer_view(data: buffer.baseAddress, size: buffer.count)
  }
}
