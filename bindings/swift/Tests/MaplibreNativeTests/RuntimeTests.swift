import CMaplibreNativeC
import Foundation
@testable import MaplibreNative
import Testing

private final class ResourceCounters: @unchecked Sendable {
  private let lock = NSLock()
  private var completeCount = 0
  private var cancelCount = 0
  private var releaseCount = 0

  func completed() {
    lock.withLock { completeCount += 1 }
  }

  func cancelled() {
    lock.withLock { cancelCount += 1 }
  }

  func released() {
    lock.withLock { releaseCount += 1 }
  }

  func snapshot() -> (complete: Int, cancel: Int, release: Int) {
    lock.withLock { (completeCount, cancelCount, releaseCount) }
  }
}

private final class ResourceCancellationResult: @unchecked Sendable {
  private let lock = NSLock()
  private var result: Result<Bool, Error>?

  func store(_ result: Result<Bool, Error>) {
    lock.withLock { self.result = result }
  }

  func load() -> Result<Bool, Error>? {
    lock.withLock { result }
  }
}

private final class ResourceHandleStateCapture: @unchecked Sendable {
  private let lock = NSLock()
  private var state: NativeResourceRequestHandleState?

  func store(_ state: NativeResourceRequestHandleState) {
    lock.withLock { self.state = state }
  }

  func load() -> NativeResourceRequestHandleState? {
    lock.withLock { state }
  }
}

@Test func runtimeCreateRunPollAndClose() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  try runtime.runOnce()
  _ = try runtime.pollEvent()
  try runtime.close()

  #expect(runtime.isClosed)
}

@Test func runtimeResourceTransformCanInstallAndClear() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }

  try runtime.setResourceTransform { request in
    request.url.replacingOccurrences(
      of: "example.test",
      with: "example.invalid"
    )
  }
  try runtime.clearResourceTransform()
}

@Test func runtimeResourceProviderCanInstallPassThroughCallback() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }

  try runtime.setResourceProvider { _, _ in
    .passThrough
  }
}

@Test func resourceTransformCallbackCopiesRequestWithoutReplacement() {
  let state = NativeResourceTransformState { request in
    #expect(request.kind == 3)
    #expect(request.url == "https://example.test/tile")
    return nil
  }

  let result = state.invokeForTesting(kind: 3, url: "https://example.test/tile")

  #expect(result.status == 0)
  #expect(result.replacement == nil)
}

@Test func resourceTransformInvokeForTestingCannotExerciseReplacementPath() {
  let state = NativeResourceTransformState { _ in
    "https://example.invalid/tile"
  }

  let result = state.invokeForTesting(kind: 3, url: "https://example.test/tile")

  #expect(result.status == MLN_STATUS_INVALID_STATE.rawValue)
  #expect(result.replacement == nil)
}

@Test func resourceProviderParseFailureFinalizesHandleState() {
  let counters = ResourceCounters()
  let functions = NativeResourceRequestHandleFunctions(
    complete: { _, _ in counters.completed() },
    cancelled: { _ in false },
    release: { _ in counters.released() }
  )
  let state = NativeResourceProviderState(handleFunctions: functions) { _, _ in
    Issue.record("malformed request should not reach provider callback")
    return MLN_RESOURCE_PROVIDER_DECISION_HANDLE.rawValue
  }

  var request = mln_resource_request()
  request.size = UInt32(MemoryLayout<mln_resource_request>.size)
  let decision = state.invokeForTesting(
    request: request,
    handle: OpaquePointer(bitPattern: 0x9)
  )

  #expect(decision == UInt32.max)
  #expect(counters.snapshot().release == 0)
}

@Test func resourceRequestHandleRejectsSecondCompletionBeforeCallingNative(
) throws {
  let counters = ResourceCounters()
  let functions = NativeResourceRequestHandleFunctions(
    complete: { _, _ in counters.completed() },
    cancelled: { _ in false },
    release: { _ in counters.released() }
  )
  let state = try NativeResourceRequestHandleState(
    pointer: OpaquePointer(bitPattern: 0x5),
    functions: functions
  )

  try state.complete(NativeResourceResponseInput(
    status: ResourceResponseStatus.ok.rawValue,
    errorReason: ResourceErrorReason.none.rawValue
  ))
  do {
    try state.complete(NativeResourceResponseInput(
      status: ResourceResponseStatus.ok.rawValue,
      errorReason: ResourceErrorReason.none.rawValue
    ))
    Issue.record("second completion should throw")
  } catch let failure as NativeStatusFailure {
    #expect(failure.diagnostic.contains("already completed"))
  }

  _ = state
    .finishProviderDecision(MLN_RESOURCE_PROVIDER_DECISION_HANDLE.rawValue)

  #expect(counters.snapshot().complete == 1)
  #expect(counters.snapshot().release == 1)
}

@Test func resourceRequestHandleKeepsFailedCompletionTerminal() throws {
  let counters = ResourceCounters()
  let functions = NativeResourceRequestHandleFunctions(
    complete: { _, _ in
      counters.completed()
      throw NativeStatusFailure(
        rawStatus: MLN_STATUS_INVALID_STATE.rawValue,
        diagnostic: "resource request can no longer accept a response"
      )
    },
    cancelled: { _ in false },
    release: { _ in counters.released() }
  )
  let state = try NativeResourceRequestHandleState(
    pointer: OpaquePointer(bitPattern: 0x5),
    functions: functions
  )

  do {
    try state.complete(NativeResourceResponseInput(
      status: ResourceResponseStatus.ok.rawValue,
      errorReason: ResourceErrorReason.none.rawValue
    ))
    Issue.record("failed native completion should throw")
  } catch let failure as NativeStatusFailure {
    #expect(failure.diagnostic.contains("no longer accept"))
  }
  do {
    try state.complete(NativeResourceResponseInput(
      status: ResourceResponseStatus.ok.rawValue,
      errorReason: ResourceErrorReason.none.rawValue
    ))
    Issue.record("second completion should throw before calling native")
  } catch let failure as NativeStatusFailure {
    #expect(failure.diagnostic.contains("already completed"))
  }

  _ = state
    .finishProviderDecision(MLN_RESOURCE_PROVIDER_DECISION_HANDLE.rawValue)

  #expect(counters.snapshot().complete == 1)
  #expect(counters.snapshot().release == 1)
}

@Test func resourceRequestReleaseWaitsForCancellationCheck() throws {
  let counters = ResourceCounters()
  let cancellationStarted = DispatchSemaphore(value: 0)
  let allowCancellationReturn = DispatchSemaphore(value: 0)
  let cancellationFinished = DispatchSemaphore(value: 0)
  let releaseStarted = DispatchSemaphore(value: 0)
  let releaseFinished = DispatchSemaphore(value: 0)
  let functions = NativeResourceRequestHandleFunctions(
    complete: { _, _ in counters.completed() },
    cancelled: { _ in
      counters.cancelled()
      cancellationStarted.signal()
      _ = allowCancellationReturn.wait(timeout: .now() + .seconds(5))
      return true
    },
    release: { _ in counters.released() }
  )
  let state = try NativeResourceRequestHandleState(
    pointer: OpaquePointer(bitPattern: 0x6),
    functions: functions
  )
  _ = state
    .finishProviderDecision(MLN_RESOURCE_PROVIDER_DECISION_HANDLE.rawValue)

  let cancellationResult = ResourceCancellationResult()
  Thread {
    cancellationResult.store(Result { try state.isCancelled() })
    cancellationFinished.signal()
  }.start()

  #expect(cancellationStarted.wait(timeout: .now() + .seconds(5)) == .success)
  Thread {
    releaseStarted.signal()
    state.release()
    releaseFinished.signal()
  }.start()

  #expect(releaseStarted.wait(timeout: .now() + .seconds(5)) == .success)
  #expect(releaseFinished
    .wait(timeout: .now() + .milliseconds(100)) == .timedOut)
  #expect(counters.snapshot().release == 0)

  allowCancellationReturn.signal()
  #expect(cancellationFinished.wait(timeout: .now() + .seconds(5)) == .success)
  #expect(releaseFinished.wait(timeout: .now() + .seconds(5)) == .success)

  switch cancellationResult.load() {
  case let .success(isCancelled):
    #expect(isCancelled)
  case let .failure(error):
    Issue.record("unexpected cancellation failure: \(error)")
  case nil:
    Issue.record("cancellation did not finish")
  }
  #expect(counters.snapshot().cancel == 1)
  #expect(counters.snapshot().release == 1)
}

@Test func resourceProviderCallbackCopiesRequestAndCompletesHandledRequest(
) throws {
  let counters = ResourceCounters()
  let functions = NativeResourceRequestHandleFunctions(
    complete: { _, response in
      counters.completed()
      #expect(response.status == ResourceResponseStatus.ok.rawValue)
      #expect(response.bytes == Array("ok".utf8))
    },
    cancelled: { _ in false },
    release: { _ in counters.released() }
  )
  let state =
    NativeResourceProviderState(handleFunctions: functions) { nativeRequest, nativeHandle in
      let request = ResourceRequest(native: nativeRequest)
      #expect(request.url == "https://example.test/tile")
      #expect(request.kind == .tile)
      #expect(request.loadingMethod == .networkOnly)
      #expect(request.priority == .low)
      #expect(request.usage == .offline)
      #expect(request.storagePolicy == .volatile)
      #expect(request.range == ByteRange(start: 7, end: 11))
      #expect(request.priorEtag == "etag")
      #expect(request.priorData == Data([1, 2, 3]))

      let handle = ResourceRequestHandle(state: nativeHandle)
      try? handle.complete(ResourceResponse(
        status: .ok,
        bytes: Data("ok".utf8)
      ))
      return 1
    }

  let priorData: [UInt8] = [1, 2, 3]
  let decision = try NativeString
    .withCString("https://example.test/tile") { url in
      try NativeString.withCString("etag") { etag in
        priorData.withUnsafeBufferPointer { priorData in
          var request = mln_resource_request()
          request.size = UInt32(MemoryLayout<mln_resource_request>.size)
          request.url = url
          request.kind = 3
          request.loading_method = 2
          request.priority = 1
          request.usage = 1
          request.storage_policy = 1
          request.has_range = true
          request.range_start = 7
          request.range_end = 11
          request.prior_etag = etag
          request.prior_data = priorData.baseAddress
          request.prior_data_size = priorData.count
          return state.invokeForTesting(
            request: request,
            handle: OpaquePointer(bitPattern: 0x4)
          )
        }
      }
    }

  #expect(decision == 1)
  #expect(counters.snapshot().complete == 1)
  #expect(counters.snapshot().release == 1)
}

@Test func resourceProviderPassThroughClosesEscapedHandleState() throws {
  let counters = ResourceCounters()
  let functions = NativeResourceRequestHandleFunctions(
    complete: { _, _ in counters.completed() },
    cancelled: { _ in false },
    release: { _ in counters.released() }
  )
  let escapedState = ResourceHandleStateCapture()
  let state =
    NativeResourceProviderState(handleFunctions: functions) { _, nativeHandle in
      escapedState.store(nativeHandle)
      return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH.rawValue
    }

  let decision = try NativeString
    .withCString("https://example.test/tile") { url in
      var request = mln_resource_request()
      request.size = UInt32(MemoryLayout<mln_resource_request>.size)
      request.url = url
      return state.invokeForTesting(
        request: request,
        handle: OpaquePointer(bitPattern: 0x7)
      )
    }

  #expect(decision == MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH.rawValue)
  do {
    try escapedState.load()?.complete(NativeResourceResponseInput(
      status: ResourceResponseStatus.ok.rawValue,
      errorReason: ResourceErrorReason.none.rawValue
    ))
    Issue.record("pass-through handle should be closed")
  } catch let failure as NativeStatusFailure {
    #expect(failure.diagnostic.contains("closed"))
  }
  #expect(counters.snapshot().complete == 0)
  #expect(counters.snapshot().release == 0)
}

@Test func resourceProviderInlineCompletionForcesHandleDecision() throws {
  let counters = ResourceCounters()
  let functions = NativeResourceRequestHandleFunctions(
    complete: { _, response in
      counters.completed()
      #expect(response.status == ResourceResponseStatus.ok.rawValue)
    },
    cancelled: { _ in false },
    release: { _ in counters.released() }
  )
  let state =
    NativeResourceProviderState(handleFunctions: functions) { _, nativeHandle in
      try? nativeHandle.complete(NativeResourceResponseInput(
        status: ResourceResponseStatus.ok.rawValue,
        errorReason: ResourceErrorReason.none.rawValue
      ))
      return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH.rawValue
    }

  let decision = try NativeString
    .withCString("https://example.test/tile") { url in
      var request = mln_resource_request()
      request.size = UInt32(MemoryLayout<mln_resource_request>.size)
      request.url = url
      return state.invokeForTesting(
        request: request,
        handle: OpaquePointer(bitPattern: 0x8)
      )
    }

  #expect(decision == MLN_RESOURCE_PROVIDER_DECISION_HANDLE.rawValue)
  #expect(counters.snapshot().complete == 1)
  #expect(counters.snapshot().release == 1)
}
