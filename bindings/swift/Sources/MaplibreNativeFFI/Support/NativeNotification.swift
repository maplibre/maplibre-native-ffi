internal import CMaplibreNativeC
import Foundation

private final class NativeNotificationContext: @unchecked Sendable {
  weak var receiver: NativeNotificationReceiver?
}

/// Bridges one receiver-scoped native notification source to Swift tasks.
final class NativeNotificationReceiver: @unchecked Sendable {
  let source: mln_notification_source

  private let callbackContext: NativeNotificationContext
  private let lock = NSLock()
  private let drainLock = NSLock()
  private var drainScheduled = false
  private var drainRequested = false
  private var completed: Set<mln_operation> = []
  private var waiters: [mln_operation: CheckedContinuation<Void, Error>] = [:]
  private var terminalError: Error?
  private var runtimeEventsHandler: (@Sendable () -> Void)?
  private var renderFramesHandlers: [UInt64: @Sendable () -> Void] = [:]
  private var driverWorkHandlers: [UInt64: @Sendable () -> Void] = [:]

  init() throws {
    var source: mln_notification_source = 0
    try checkStatus(mln_notification_source_create(&source))
    self.source = source
    let callbackContext = NativeNotificationContext()
    self.callbackContext = callbackContext
    callbackContext.receiver = self

    let context = Unmanaged.passUnretained(callbackContext).toOpaque()
    do {
      try checkStatus(mln_notification_source_set_callback(
        source,
        { context in
          guard let context else { return }
          let receiver = Unmanaged<NativeNotificationContext>
            .fromOpaque(context).takeUnretainedValue()
            .receiver
          receiver?.scheduleDrain()
        },
        context
      ))
    } catch {
      _ = mln_notification_source_close(source)
      throw error
    }
  }

  deinit {
    // A dropped runtime intentionally leaves its native handle live for the
    // leak reporter, but this receiver must not leave that runtime pointing at
    // freed Swift storage.
    try? close()
  }

  func wait(for operation: NativeOperationHandle) async throws {
    let alreadyCompleted = try NativeMemory.withTemporary(false) { completed in
      try checkStatus(mln_operation_poll(operation.raw, completed))
    }.value
    if alreadyCompleted { return }

    try await withCheckedThrowingContinuation {
      (continuation: CheckedContinuation<Void, Error>) in
      let needsDrain = lock.withLock { () -> Bool in
        if let terminalError {
          continuation.resume(throwing: terminalError)
          return false
        } else if completed.remove(operation.raw) != nil {
          continuation.resume()
          return false
        } else {
          precondition(waiters[operation.raw] == nil)
          waiters[operation.raw] = continuation
          return true
        }
      }
      if needsDrain { scheduleDrain() }
    }
  }

  func setRuntimeEventsHandler(_ handler: (@Sendable () -> Void)?) {
    lock.withLock { runtimeEventsHandler = handler }
    if handler != nil {
      scheduleDrain()
    }
  }

  func setRenderFramesHandler(
    for session: NativeRenderSessionHandle,
    _ handler: (@Sendable () -> Void)?
  ) {
    lock.withLock { renderFramesHandlers[session.raw] = handler }
    if handler != nil { scheduleDrain() }
  }

  func setDriverWorkHandler(
    for session: NativeRenderSessionHandle,
    _ handler: (@Sendable () -> Void)?
  ) {
    lock.withLock { driverWorkHandlers[session.raw] = handler }
    if handler != nil { scheduleDrain() }
  }

  func forget(_ operation: NativeOperationHandle) {
    _ = lock.withLock { completed.remove(operation.raw) }
  }

  func close() throws {
    try checkStatus(mln_notification_source_clear_callback(source))
    drainLock.withLock {}
    lock.withLock {
      runtimeEventsHandler = nil
      renderFramesHandlers.removeAll()
      driverWorkHandlers.removeAll()
      drainScheduled = false
      drainRequested = false
    }
    try checkStatus(mln_notification_source_close(source))
  }

  private func scheduleDrain() {
    let shouldSchedule = lock.withLock { () -> Bool in
      guard !waiters.isEmpty || runtimeEventsHandler != nil
        || !renderFramesHandlers.isEmpty || !driverWorkHandlers.isEmpty
      else { return false }
      if drainScheduled {
        drainRequested = true
        return false
      }
      drainScheduled = true
      return true
    }
    guard shouldSchedule else { return }
    Task.detached {
      while true {
        self.drainLock.withLock { self.drainReadyLocked() }
        let drainAgain = self.lock.withLock { () -> Bool in
          guard !self.waiters.isEmpty || self.runtimeEventsHandler != nil
            || !self.renderFramesHandlers.isEmpty
            || !self.driverWorkHandlers.isEmpty
          else {
            self.drainScheduled = false
            self.drainRequested = false
            return false
          }
          if self.drainRequested {
            self.drainRequested = false
            return true
          }
          self.drainScheduled = false
          return false
        }
        if !drainAgain { return }
      }
    }
  }

  private func drainReadyLocked() {
    do {
      var batch: mln_ready_batch = 0
      try checkStatus(mln_notification_source_drain_ready(source, &batch))
      defer { mln_ready_batch_release(batch) }

      var view = mln_ready_batch_view()
      view.size = UInt32(MemoryLayout<mln_ready_batch_view>.size)
      try checkStatus(mln_ready_batch_get(batch, &view))
      guard view.endpoint_count > 0, let endpoints = view.endpoints else {
        return
      }

      let stride = Int(view.endpoint_size)
      let base = UnsafeRawPointer(endpoints)
      for index in 0 ..< view.endpoint_count {
        let endpoint = base.advanced(by: index * stride)
          .assumingMemoryBound(to: mln_ready_endpoint.self).pointee
        if endpoint.kind == MLN_NOTIFICATION_ENDPOINT_OPERATION.rawValue {
          complete(operation: endpoint.id)
        } else if endpoint.kind ==
          MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS.rawValue
        {
          lock.withLock { runtimeEventsHandler }?()
        } else if endpoint.kind ==
          MLN_NOTIFICATION_ENDPOINT_RENDER_FRAMES.rawValue
        {
          lock.withLock { renderFramesHandlers[endpoint.id] }?()
        } else if endpoint.kind ==
          MLN_NOTIFICATION_ENDPOINT_DRIVER_WORK.rawValue
        {
          lock.withLock { driverWorkHandlers[endpoint.id] }?()
        }
      }
    } catch {
      fail(error)
    }
  }

  private func complete(operation: mln_operation) {
    let waiter = lock.withLock { () -> CheckedContinuation<Void, Error>? in
      if let waiter = waiters.removeValue(forKey: operation) {
        return waiter
      }
      completed.insert(operation)
      return nil
    }
    waiter?.resume()
  }

  private func fail(_ error: Error) {
    let pending = lock.withLock { () -> [CheckedContinuation<Void, Error>] in
      guard terminalError == nil else { return [] }
      terminalError = error
      let pending = Array(waiters.values)
      waiters.removeAll()
      return pending
    }
    for waiter in pending {
      waiter.resume(throwing: error)
    }
  }
}

enum NativeOperation {
  static func waitForSuccess(
    _ operation: NativeOperationHandle,
    receiver: NativeNotificationReceiver
  ) async throws {
    try await receiver.wait(for: operation)
    try checkSuccess(operation)
  }

  static func waitForSuccessBlocking(
    _ operation: NativeOperationHandle
  ) throws {
    var completed = false
    try checkStatus(mln_operation_wait(operation.raw, -1, &completed))
    precondition(completed)
    try checkSuccess(operation)
  }

  private static func checkSuccess(
    _ operation: NativeOperationHandle
  ) throws {
    let status = try NativeMemory.withTemporary(MLN_STATUS_OK) { outStatus in
      try checkStatus(mln_operation_get_status(operation.raw, outStatus))
    }.value
    guard status == MLN_STATUS_OK else {
      throw try NativeStatusFailure(
        rawStatus: status.rawValue,
        diagnostic: diagnostic(operation)
      )
    }
  }

  private static func diagnostic(_ operation: NativeOperationHandle) throws
    -> String
  {
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
    return try bytes.withUnsafeBufferPointer { buffer in
      try NativeString.copyUTF8(
        data: buffer.baseAddress.map(UnsafeRawPointer.init),
        size: copied
      )
    }
  }
}
