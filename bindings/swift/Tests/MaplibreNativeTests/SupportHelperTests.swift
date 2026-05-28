import CMaplibreNativeC
import Foundation
import Testing

@testable import MaplibreNative


private final class LockedBox<Value>: @unchecked Sendable {
  private let lock = NSLock()
  private var value: Value

  init(_ value: Value) {
    self.value = value
  }

  func update(_ body: (inout Value) -> Void) {
    lock.withLock {
      body(&value)
    }
  }

  func read<Result>(_ body: (Value) -> Result) -> Result {
    lock.withLock {
      body(value)
    }
  }
}

@Test func nativeCStringRejectsEmbeddedNul() throws {
  do {
    _ = try NativeString.withCString("a\0b") { _ in true }
    Issue.record("embedded NUL should throw")
  } catch let error as NativeStringError {
    #expect(error.message.contains("embedded NUL"))
  }
}

@Test func nativeStringViewUsesUtf8ByteCountAndAllowsNulBytes() throws {
  let observed = try NativeString.withStringView("é\0") { view in
    let bytes = UnsafeBufferPointer(
      start: UnsafeRawPointer(view.data).assumingMemoryBound(to: UInt8.self),
      count: view.size
    )
    return Array(bytes)
  }

  #expect(observed == Array("é\0".utf8))
}

@Test func nativeMemoryTemporaryReturnsMutatedValue() throws {
  let output = try NativeMemory.withTemporary(UInt32(1)) { pointer in
    pointer.pointee = 42
    return "done"
  }

  #expect(output.value == 42)
  #expect(output.result == "done")
}

@Test func nativeDescriptorMaterializerProvidesScopedPointer() throws {
  struct TinyDescriptor: Equatable {
    var size: UInt32
    var value: UInt32
  }

  let descriptor = NativeDescriptorMaterializer(TinyDescriptor(size: 8, value: 99))
  let copied = try descriptor.withNativeDescriptor { pointer in
    pointer.pointee
  }

  #expect(copied == TinyDescriptor(size: 8, value: 99))
}

@Test func nativeResultGuardReleasesExactlyOnce() throws {
  let releases = LockedBox(0)
  let pointer = OpaquePointer(bitPattern: 0x1)
  let guardHandle = try NativeResultGuard(typeName: "test_result", pointer: pointer) { _ in
    releases.update { $0 += 1 }
  }

  guardHandle.close()
  guardHandle.close()

  #expect(releases.read { $0 } == 1)
}

@Test func nativeHandleStateCloseIsIdempotentAfterSuccess() throws {
  let closes = LockedBox(0)
  let state = try NativeHandleState(typeName: "test_handle", pointer: OpaquePointer(bitPattern: 0x2))

  try state.closeOnce { _ in
    closes.update { $0 += 1 }
  }
  try state.closeOnce { _ in
    closes.update { $0 += 1 }
  }

  #expect(state.isClosed)
  #expect(closes.read { $0 } == 1)
}

@Test func nativeHandleStateConcurrentCloseDestroysOnce() throws {
  let closes = LockedBox(0)
  let state = try NativeHandleState(typeName: "test_handle", pointer: OpaquePointer(bitPattern: 0x5))

  DispatchQueue.concurrentPerform(iterations: 16) { _ in
    try? state.closeOnce { _ in
      closes.update { $0 += 1 }
    }
  }

  #expect(state.isClosed)
  #expect(closes.read { $0 } == 1)
}

@Test func nativeHandleStateAllowsRetryAfterFailedClose() throws {
  struct CloseFailure: Error {}

  let closes = LockedBox(0)
  let state = try NativeHandleState(typeName: "test_handle", pointer: OpaquePointer(bitPattern: 0x4))

  do {
    try state.closeOnce { _ in
      closes.update { $0 += 1 }
      throw CloseFailure()
    }
    Issue.record("failed close should throw")
  } catch is CloseFailure {}

  #expect(!state.isClosed)

  try state.closeOnce { _ in
    closes.update { $0 += 1 }
  }

  #expect(state.isClosed)
  #expect(closes.read { $0 } == 2)
}

@Test func nativeHandleStateReportsLeaksWithoutDestroying() throws {
  let leaks = LockedBox([NativeHandleLeak]())

  try NativeHandleLeakTestSupport.withHandler({ leak in
    leaks.update { $0.append(leak) }
  }) {
    do {
      _ = try NativeHandleState(typeName: "leaky_handle", pointer: OpaquePointer(bitPattern: 0x3))
    }

    #expect(leaks.read { $0 } == [NativeHandleLeak(typeName: "leaky_handle", address: 0x3)])
  }
}
