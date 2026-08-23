internal import CMaplibreNativeC
import Foundation

private protocol AnyNativeCompletionState: AnyObject {
  func complete(_ result: UnsafePointer<mln_completion_result>)
}

private final class NativeCompletionState<Value: Sendable>:
  AnyNativeCompletionState, @unchecked Sendable
{
  private let lock = NSCondition()
  private let convert: (UnsafePointer<mln_completion_result>) throws -> Value
  private let acceptErrorStatus: Bool
  private var result: Result<Value, Error>?
  private var waiter: CheckedContinuation<Value, Error>?

  init(
    acceptErrorStatus: Bool = false,
    convert: @escaping (UnsafePointer<mln_completion_result>) throws -> Value
  ) {
    self.acceptErrorStatus = acceptErrorStatus
    self.convert = convert
  }

  func complete(_ native: UnsafePointer<mln_completion_result>) {
    let converted: Result<Value, Error>
    do {
      let raw = native.pointee
      guard raw.status == MLN_STATUS_OK || acceptErrorStatus else {
        let diagnostic = try NativeString.copyUTF8(
          data: raw.diagnostic.data,
          size: raw.diagnostic.size
        )
        throw NativeStatusFailure(
          rawStatus: raw.status.rawValue,
          diagnostic: diagnostic
        )
      }
      converted = try .success(convert(native))
    } catch {
      converted = .failure(error)
    }

    let waiter = lock.withLock { () -> CheckedContinuation<Value, Error>? in
      if let waiter = self.waiter {
        self.waiter = nil
        return waiter
      }
      result = converted
      lock.broadcast()
      return nil
    }
    waiter?.resume(with: converted)
  }

  func value() async throws -> Value {
    try await withCheckedThrowingContinuation { continuation in
      let completed = lock.withLock { () -> Result<Value, Error>? in
        if let result {
          self.result = nil
          return result
        }
        precondition(waiter == nil)
        waiter = continuation
        return nil
      }
      if let completed { continuation.resume(with: completed) }
    }
  }

  /// Blocks the calling thread until the completion runs.
  ///
  /// Native runs the completion on its own thread, so this suits a host that
  /// must not return before native work finishes, such as one tearing a runtime
  /// down before process exit.
  func valueBlocking() throws -> Value {
    lock.lock()
    defer { lock.unlock() }
    while result == nil {
      lock.wait()
    }
    let completed = result
    result = nil
    return try completed!.get()
  }
}

struct NativeFuture<Value: Sendable> {
  fileprivate let state: NativeCompletionState<Value>

  func value() async throws -> Value {
    try await state.value()
  }

  func valueBlocking() throws -> Value {
    try state.valueBlocking()
  }
}

enum NativeCompletion {
  /// Starts native work synchronously and returns its one-shot async value.
  static func start<Value: Sendable>(
    _ call: (UnsafePointer<mln_completion>) -> mln_status,
    acceptErrorStatus: Bool = false,
    convert: @escaping (UnsafePointer<mln_completion_result>) throws -> Value
  ) throws -> NativeFuture<Value> {
    let state = NativeCompletionState(
      acceptErrorStatus: acceptErrorStatus,
      convert: convert
    )
    let retained = Unmanaged<AnyObject>.passRetained(state)
    var descriptor = mln_completion()
    descriptor.size = UInt32(MemoryLayout<mln_completion>.size)
    descriptor.callback = { userData, result in
      guard let userData, let result else { return }
      let state = Unmanaged<AnyObject>.fromOpaque(userData)
        .takeUnretainedValue() as! AnyNativeCompletionState
      state.complete(result)
    }
    descriptor.user_data = retained.toOpaque()
    descriptor.release_user_data = { userData in
      guard let userData else { return }
      Unmanaged<AnyObject>.fromOpaque(userData).release()
    }

    let status = withUnsafePointer(to: &descriptor, call)
    if status != MLN_STATUS_OK {
      retained.release()
      try checkStatus(status)
    }
    return NativeFuture(state: state)
  }

  static func submit<Value: Sendable>(
    _ call: (UnsafePointer<mln_completion>) -> mln_status,
    convert: @escaping (UnsafePointer<mln_completion_result>) throws -> Value
  ) async throws -> Value {
    try await start(call, convert: convert).value()
  }

  static func startUnit(
    _ call: (UnsafePointer<mln_completion>) -> mln_status
  ) throws -> NativeFuture<Void> {
    try start(call) { _ in () }
  }

  static func unit(
    _ call: (UnsafePointer<mln_completion>) -> mln_status
  ) async throws {
    try await startUnit(call).value()
  }

  static func startCommand(
    _ call: (UnsafePointer<mln_completion>) -> mln_status
  ) throws -> NativeFuture<CommandCompletion> {
    try start(call, acceptErrorStatus: true) { result in
      try CommandCompletion(
        disposition: CommandDisposition.fromNative(result.pointee.disposition),
        generation: result.pointee.generation,
        rawStatus: result.pointee.status.rawValue,
        diagnostic: NativeString.copyUTF8(
          data: result.pointee.diagnostic.data,
          size: result.pointee.diagnostic.size
        )
      )
    }
  }

  static func command(
    _ call: (UnsafePointer<mln_completion>) -> mln_status
  ) async throws -> CommandCompletion {
    try await startCommand(call).value()
  }

  static func value<Value>(
    _ result: UnsafePointer<mln_completion_result>,
    as _: Value.Type = Value.self
  ) throws -> Value {
    guard result.pointee.value_count == 1,
          let value = result.pointee.value
    else {
      throw NativeStatusFailure.swiftNativeError(
        "native completion returned no value"
      )
    }
    return value.load(as: Value.self)
  }

  static func values<Value>(
    _ result: UnsafePointer<mln_completion_result>,
    as _: Value.Type = Value.self
  ) throws -> UnsafeBufferPointer<Value> {
    guard result.pointee.value_count > 0 else {
      return UnsafeBufferPointer(start: nil, count: 0)
    }
    guard let value = result.pointee.value else {
      throw NativeStatusFailure.swiftNativeError(
        "native completion returned a null array"
      )
    }
    return UnsafeBufferPointer(
      start: value.assumingMemoryBound(to: Value.self),
      count: result.pointee.value_count
    )
  }

  static func data(
    _ result: UnsafePointer<mln_completion_result>
  ) throws -> Data {
    let view: mln_buffer_view = try value(result)
    guard view.size > 0 else { return Data() }
    guard let bytes = view.data else {
      throw NativeStatusFailure.swiftNativeError(
        "native completion returned a null buffer"
      )
    }
    return Data(bytes: bytes, count: view.size)
  }

  static func dataView(_ view: mln_buffer_view) throws -> Data {
    guard view.size > 0 else { return Data() }
    guard let bytes = view.data else {
      throw NativeStatusFailure.swiftNativeError(
        "native completion returned a null buffer"
      )
    }
    return Data(bytes: bytes, count: view.size)
  }

  static func string(
    _ result: UnsafePointer<mln_completion_result>
  ) throws -> String {
    let view: mln_buffer_view = try value(result)
    return try NativeString.copyUTF8(data: view.data, size: view.size)
  }
}
