import CMaplibreNativeC
import Foundation
import Testing

@testable import MaplibreNative
@testable import MaplibreNativeSupport

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

@Test func runtimeCreateRunPollAndClose() throws {
  let runtime = try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  try runtime.runOnce()
  _ = try runtime.pollEvent()
  try runtime.close()

  #expect(runtime.isClosed)
}

@Test func runtimeResourceTransformCanInstallAndClear() throws {
  let runtime = try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }

  try runtime.setResourceTransform { request in
    request.url.replacingOccurrences(of: "example.test", with: "example.invalid")
  }
  try runtime.clearResourceTransform()
}

@Test func runtimeResourceProviderCanInstallPassThroughCallback() throws {
  let runtime = try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }

  try runtime.setResourceProvider { _, _ in
    .passThrough
  }
}

@Test func resourceTransformCallbackCopiesRequestAndStoresReplacement() {
  let state = NativeResourceTransformState { request in
    #expect(request.kind == 3)
    #expect(request.url == "https://example.test/tile")
    return "https://example.invalid/tile"
  }

  let result = state.invokeForTesting(kind: 3, url: "https://example.test/tile")

  #expect(result.status == 0)
  #expect(result.replacement == "https://example.invalid/tile")
}

@Test func resourceRequestHandleRejectsSecondCompletionBeforeCallingNative() throws {
  let counters = ResourceCounters()
  let functions = NativeResourceRequestHandleFunctions(
    complete: { _, _ in counters.completed() },
    cancelled: { _ in false },
    release: { _ in counters.released() }
  )
  let state = try NativeResourceRequestHandleState(pointer: OpaquePointer(bitPattern: 0x5), functions: functions)

  try state.complete(NativeResourceResponseInput(status: ResourceResponseStatus.ok.rawValue, errorReason: ResourceErrorReason.none.rawValue))
  do {
    try state.complete(NativeResourceResponseInput(status: ResourceResponseStatus.ok.rawValue, errorReason: ResourceErrorReason.none.rawValue))
    Issue.record("second completion should throw")
  } catch let failure as NativeStatusFailure {
    #expect(failure.diagnostic.contains("already completed"))
  }

  state.markProviderReturnedHandle()

  #expect(counters.snapshot().complete == 1)
  #expect(counters.snapshot().release == 1)
}

@Test func resourceRequestReleaseWaitsForCancellationCheck() throws {
  let counters = ResourceCounters()
  let cancellationStarted = DispatchSemaphore(value: 0)
  let allowCancellationReturn = DispatchSemaphore(value: 0)
  let cancellationFinished = DispatchSemaphore(value: 0)
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
  let state = try NativeResourceRequestHandleState(pointer: OpaquePointer(bitPattern: 0x6), functions: functions)
  state.markProviderReturnedHandle()

  let cancellationResult = ResourceCancellationResult()
  DispatchQueue.global().async {
    cancellationResult.store(Result { try state.isCancelled() })
    cancellationFinished.signal()
  }

  #expect(cancellationStarted.wait(timeout: .now() + .seconds(2)) == .success)
  DispatchQueue.global().async {
    state.release()
    releaseFinished.signal()
  }

  #expect(releaseFinished.wait(timeout: .now() + .milliseconds(100)) == .timedOut)
  #expect(counters.snapshot().release == 0)

  allowCancellationReturn.signal()
  #expect(cancellationFinished.wait(timeout: .now() + .seconds(2)) == .success)
  #expect(releaseFinished.wait(timeout: .now() + .seconds(2)) == .success)

  switch cancellationResult.load() {
  case .success(let isCancelled):
    #expect(isCancelled)
  case .failure(let error):
    Issue.record("unexpected cancellation failure: \(error)")
  case nil:
    Issue.record("cancellation did not finish")
  }
  #expect(counters.snapshot().cancel == 1)
  #expect(counters.snapshot().release == 1)
}

@Test func resourceProviderCallbackCopiesRequestAndCompletesHandledRequest() throws {
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
  let state = NativeResourceProviderState(handleFunctions: functions) { nativeRequest, nativeHandle in
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
    try? handle.complete(ResourceResponse(status: .ok, bytes: Data("ok".utf8)))
    return 1
  }

  let priorData: [UInt8] = [1, 2, 3]
  let decision = try NativeString.withCString("https://example.test/tile") { url in
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
        return state.invokeForTesting(request: request, handle: OpaquePointer(bitPattern: 0x4))
      }
    }
  }

  #expect(decision == 1)
  #expect(counters.snapshot().complete == 1)
  #expect(counters.snapshot().release == 1)
}
