internal import CMaplibreNativeC
import Foundation

/// Owns per-call byte storage whose pointers stay valid until the C call
/// returns.
final class NativeInputArena {
  private var buffers: [UnsafeMutableRawBufferPointer] = []
  private var callbacks: [UnsafeMutableRawPointer] = []
  private var reads: [AnyObject] = []

  deinit {
    for callback in callbacks {
      releaseGeneratedCallback(callback)
    }
    for buffer in buffers {
      buffer.deallocate()
    }
  }

  func callback<Value>(_ value: Value) -> UnsafeMutableRawPointer {
    let pointer = Unmanaged.passRetained(GeneratedCallbackBox(value)).toOpaque()
    callbacks.append(pointer)
    return pointer
  }

  func borrow<Handle>(_ owner: NativeHandleBox<Handle>) throws -> UInt64 {
    let access = try owner.borrow()
    reads.append(access)
    return access.handle.raw
  }

  func submit(_ body: () throws -> mln_status) rethrows -> mln_status {
    let status = try body()
    if status == MLN_STATUS_OK { accept() }
    return status
  }

  /// Transfers every callback this arena registered to native ownership.
  func accept() {
    callbacks.removeAll()
  }

  static func count<Value: BinaryInteger>(_ count: Int) throws -> Value {
    guard let result = Value(exactly: count)
    else { throw NativeStringError("array exceeds native count range") }
    return result
  }

  func store<Value>(_ value: Value) -> UnsafePointer<Value> {
    let buffer = UnsafeMutableRawBufferPointer.allocate(
      byteCount: MemoryLayout<Value>.stride,
      alignment: MemoryLayout<Value>.alignment
    )
    buffer.storeBytes(of: value, as: Value.self)
    buffers.append(buffer)
    return UnsafePointer(buffer.baseAddress!
      .assumingMemoryBound(to: Value.self))
  }

  func array<Value>(_ values: [Value],
                    count: Int) throws -> UnsafePointer<Value>?
  {
    guard values.count == count
    else { throw NativeStringError("incorrect fixed array length") }
    return array(values)
  }

  func array<Value>(_ values: [Value]) -> UnsafePointer<Value>? {
    let buffer = UnsafeMutableRawBufferPointer.allocate(
      byteCount: max(1, MemoryLayout<Value>.stride * values.count),
      alignment: MemoryLayout<Value>.alignment
    )
    values.withUnsafeBytes { buffer.copyMemory(from: $0) }
    buffers.append(buffer)
    return UnsafePointer(buffer.baseAddress!
      .assumingMemoryBound(to: Value.self))
  }

  func cString(_ value: String) throws -> UnsafePointer<CChar> {
    guard !value.utf8.contains(0)
    else { throw NativeStringError("embedded null in C string") }
    return array(value.utf8CString.map { $0 })!
  }

  static func copyArray<Value>(_ pointer: UnsafePointer<Value>?,
                               count: Int) throws -> [Value]
  {
    guard count >= 0,
          count <= Int.max / max(1, MemoryLayout<Value>.stride)
    else {
      throw NativeStringError(
        "native array count is outside the addressable range"
      )
    }
    guard count > 0 else { return [] }
    guard let pointer
    else { throw NativeStringError("null native array with nonzero count") }
    return Array(UnsafeBufferPointer(start: pointer, count: count))
  }

  static func copyStrided<
    Value,
    Result,
    Count: BinaryInteger,
    Stride: BinaryInteger
  >(
    _ pointer: UnsafePointer<Value>?, count: Count, stride: Stride,
    convert: (Value, UnsafeRawBufferPointer) throws -> Result
  ) throws -> [Result] {
    guard let count = Int(exactly: count), count >= 0,
          let stride = Int(exactly: stride), stride >= MemoryLayout<Value>.size,
          count <= Int.max / max(1, stride)
    else {
      throw NativeStringError("invalid native record stride or count")
    }
    guard count > 0 else { return [] }
    guard let pointer
    else { throw NativeStringError("null native record array") }
    let bytes = UnsafeRawPointer(pointer)
    return try (0 ..< count).map { index in
      let item = bytes.advanced(by: index * stride)
      return try convert(
        item.loadUnaligned(as: Value.self),
        UnsafeRawBufferPointer(start: item, count: stride)
      )
    }
  }

  static func copyUTF8Slice<
    Size: BinaryInteger,
    Offset: BinaryInteger,
    Length: BinaryInteger
  >(
    data: UnsafeRawPointer?, size: Size, offset: Offset, length: Length
  ) throws -> String {
    guard let size = Int(exactly: size), let offset = Int(exactly: offset),
          let length = Int(exactly: length), size >= 0, offset >= 0,
          length >= 0,
          offset <= size, length <= size - offset, size == 0 || data != nil
    else {
      throw NativeStringError("native message slice exceeds its arena")
    }
    return try NativeString.copyUTF8(
      data: data?.advanced(by: offset),
      size: length
    )
  }

  static func copyDataSlice<
    Size: BinaryInteger,
    Offset: BinaryInteger,
    Length: BinaryInteger
  >(
    data: UnsafeRawPointer?, size: Size, offset: Offset, length: Length
  ) throws -> Data {
    guard let size = Int(exactly: size), let offset = Int(exactly: offset),
          let length = Int(exactly: length), size >= 0, offset >= 0,
          length >= 0,
          offset <= size, length <= size - offset, size == 0 || data != nil
    else {
      throw NativeStringError("native byte slice exceeds its arena")
    }
    return try NativeString.copyData(
      data: data?.advanced(by: offset),
      size: length
    )
  }

  func view(_ text: String) -> mln_buffer_view {
    view(Data(text.utf8))
  }

  func view(_ data: Data) -> mln_buffer_view {
    let buffer = UnsafeMutableRawBufferPointer.allocate(
      byteCount: max(1, data.count),
      alignment: MemoryLayout<UInt8>.alignment
    )
    data.copyBytes(to: buffer)
    buffers.append(buffer)
    return mln_buffer_view(data: buffer.baseAddress, size: data.count)
  }
}

final class GeneratedCallbackBox<Value> {
  let value: Value
  init(_ value: Value) {
    self.value = value
  }
}

/// A callback whose reentry policy admits calls on the handle that registered
/// it. The owner is weak so a native root does not keep its handle alive.
struct NativeOwnedCallback<Value> {
  weak var owner: AnyObject?
  let value: Value
}

func releaseGeneratedCallback(_ pointer: UnsafeMutableRawPointer?) {
  let admission = NativeCallbackGuard.enter(owner: nil, operations: [])
  defer { admission.end() }
  guard let pointer else { return }
  Unmanaged<AnyObject>.fromOpaque(pointer).release()
}

/// A generated open enum or flag set, which carries its native value as its
/// raw value so that values this binding does not name survive a round trip.
protocol NativeOpenValue {
  associatedtype Native
  var rawValue: Native { get }
  init(rawValue: Native)
}

extension NativeOpenValue {
  init(raw: Native) {
    self.init(rawValue: raw)
  }

  func nativeValue() -> Native {
    rawValue
  }
}
