internal import CMaplibreNativeC
import Foundation

enum OperationResultKind: Equatable {
  case none
  case createdRegion
  case optionalRegion
  case regionList
  case mergedRegionList
  case updatedRegionMetadata
  case regionStatus
}

private enum OperationResultState {
  case available
  case consuming
  case consumed
}

/// An owned asynchronous operation.
///
/// Close an operation after taking or discarding its result. Deinitialization
/// also releases an operation that remains open.
public final class OperationHandle: @unchecked Sendable {
  private let handle: NativeHandleBox<NativeOperationHandle>
  private let runtime: RuntimeHandle
  private let resultKind: OperationResultKind
  private let lifecycleLock = NSLock()
  private var registered = false
  private var resultState = OperationResultState.available

  init(
    runtime: RuntimeHandle,
    handle nativeHandle: NativeOperationHandle,
    resultKind: OperationResultKind
  ) throws {
    self.runtime = runtime
    self.resultKind = resultKind
    handle = try NativeHandleBox(
      typeName: "OperationHandle",
      handle: nativeHandle
    )
    do {
      try runtime.registerOperation()
      registered = true
    } catch {
      mln_operation_release(nativeHandle.raw)
      throw error
    }
  }

  deinit {
    try? close()
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  /// Reports whether the operation reached a terminal disposition.
  public func poll() throws -> Bool {
    try mapNativeFailure {
      try handle.withLive { operation in
        try NativeMemory.withTemporary(false) { completed in
          try checkStatus(mln_operation_poll(operation.raw, completed))
        }.value
      }
    }
  }

  /// Waits for completion, using milliseconds for the timeout.
  ///
  /// A negative timeout waits without a deadline. Zero performs a
  /// nonblocking check.
  public func wait(timeoutMilliseconds: Int64 = -1) throws -> Bool {
    try mapNativeFailure {
      try handle.withLive { operation in
        try NativeMemory.withTemporary(false) { completed in
          try checkStatus(mln_operation_wait(
            operation.raw,
            timeoutMilliseconds,
            completed
          ))
        }.value
      }
    }
  }

  /// Requests cancellation. The operation remains open for inspection.
  public func cancel() throws {
    try mapNativeFailure {
      try handle.withLive { operation in
        try checkStatus(mln_operation_cancel(operation.raw))
      }
    }
  }

  /// Returns the raw terminal native status.
  public func status() throws -> Int32 {
    try mapNativeFailure {
      try handle.withLive { operation in
        try NativeMemory.withTemporary(MLN_STATUS_OK) { status in
          try checkStatus(mln_operation_get_status(operation.raw, status))
        }.value.rawValue
      }
    }
  }

  /// Copies the terminal diagnostic into Swift-owned storage.
  public func diagnostic() throws -> String {
    try mapNativeFailure {
      try handle.withLive { operation in
        var required = 0
        try checkStatus(mln_operation_copy_diagnostic(
          operation.raw,
          nil,
          0,
          &required
        ))
        guard required > 0 else { return "" }

        var bytes = [CChar](repeating: 0, count: required)
        var copied = 0
        try bytes.withUnsafeMutableBufferPointer { buffer in
          try checkStatus(mln_operation_copy_diagnostic(
            operation.raw,
            buffer.baseAddress,
            buffer.count,
            &copied
          ))
        }
        guard copied <= bytes.count else {
          throw NativeStatusFailure.swiftNativeError(
            "operation diagnostic size exceeded caller buffer"
          )
        }
        return try bytes.withUnsafeBufferPointer { buffer in
          try NativeString.copyUTF8(
            data: buffer.baseAddress.map(UnsafeRawPointer.init),
            size: copied
          )
        }
      }
    }
  }

  /// Discards an untaken result. The operation remains open for terminal
  /// status and diagnostic inspection.
  public func discard() throws {
    try beginResultConsumption(expectedResultKind: nil)
    do {
      try mapNativeFailure {
        try handle.withLive { operation in
          try checkStatus(mln_operation_discard_result(operation.raw))
        }
      }
      finishResultConsumption(transferred: true)
    } catch {
      finishResultConsumption(transferred: false)
      throw error
    }
  }

  /// Releases the operation. Releasing a pending operation requests
  /// cancellation when the native operation supports it.
  public func close() throws {
    try handle.closeOnce { operation in
      mln_operation_release(operation.raw)
    }
    lifecycleLock.withLock {
      guard registered else { return }
      registered = false
      runtime.unregisterOperation()
    }
  }

  func take<Result>(
    from expectedRuntime: RuntimeHandle,
    resultKind expectedResultKind: OperationResultKind,
    _ body: (NativeOperationHandle, () -> Void) throws -> Result
  ) throws -> Result {
    guard runtime === expectedRuntime else {
      throw MaplibreError(
        kind: .invalidState,
        rawStatus: nil,
        diagnostic: "OperationHandle belongs to a different RuntimeHandle"
      )
    }
    try beginResultConsumption(expectedResultKind: expectedResultKind)

    var transferred = false
    do {
      let result = try mapNativeFailure {
        try handle.withLive { operation in
          try body(operation) {
            transferred = true
            finishResultConsumption(transferred: true)
          }
        }
      }
      if !transferred {
        finishResultConsumption(transferred: false)
        throw MaplibreError(
          kind: .nativeError,
          rawStatus: nil,
          diagnostic: "typed operation result was not transferred"
        )
      }
      return result
    } catch {
      if !transferred {
        finishResultConsumption(transferred: false)
      }
      throw error
    }
  }

  private func beginResultConsumption(
    expectedResultKind: OperationResultKind?
  ) throws {
    try lifecycleLock.withLock {
      if let expectedResultKind, resultKind != expectedResultKind {
        throw MaplibreError(
          kind: .invalidState,
          rawStatus: nil,
          diagnostic: "OperationHandle has a different result type"
        )
      }
      guard case .available = resultState else {
        throw MaplibreError(
          kind: .invalidState,
          rawStatus: nil,
          diagnostic: "OperationHandle result is already consumed"
        )
      }
      resultState = .consuming
    }
  }

  private func finishResultConsumption(transferred: Bool) {
    lifecycleLock.withLock {
      guard case .consuming = resultState else { return }
      resultState = transferred ? .consumed : .available
    }
  }
}
