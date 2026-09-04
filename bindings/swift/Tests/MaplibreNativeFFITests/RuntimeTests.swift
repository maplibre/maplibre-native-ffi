import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
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

private final class ResourceProviderCallCounter: @unchecked Sendable {
  private let lock = NSLock()
  private var count = 0

  func recordCall() {
    lock.withLock { count += 1 }
  }

  var callCount: Int {
    lock.withLock { count }
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

@Test func runtimeCreateRunDrainAndClose() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  try runtime.pump()
  _ = try runtime.drainEvents()
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

/// Requests a style URL whose scheme no file source serves, pumps until the
/// matching loading failure arrives, and returns its message. The failure
/// proves the request reached the network file source, where the
/// runtime-scoped resource provider applies.
private func loadProbeStyle(
  runtime: RuntimeHandle,
  map: MapHandle,
  styleURL: String
) throws -> String? {
  try map.setStyleURL(styleURL)
  return try pumpUntilEvent(
    runtime,
    waitingFor: "a loading failure for \(styleURL)"
  ) { event in
    event.type == .mapLoadingFailed && event.message.contains(styleURL)
  }?.message
}

@Test func runtimeResourceProviderIsConsultedUntilReplacedAndCleared() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64)
  )
  defer { try? map.close() }

  let firstCalls = ResourceProviderCallCounter()
  try runtime.setResourceProvider { _, _ in
    firstCalls.recordCall()
    return .passThrough
  }

  let firstFailure = try loadProbeStyle(
    runtime: runtime,
    map: map,
    styleURL: "jar:file:/packaged/first.json"
  )
  #expect(firstFailure?.contains("\"jar\"") == true)
  #expect(firstCalls.callCount > 0)

  // The previous provider stops being consulted once the call returns.
  let secondCalls = ResourceProviderCallCounter()
  try runtime.setResourceProvider { _, _ in
    secondCalls.recordCall()
    return .passThrough
  }
  let firstCallsAfterReplace = firstCalls.callCount

  let secondFailure = try loadProbeStyle(
    runtime: runtime,
    map: map,
    styleURL: "jar:file:/packaged/second.json"
  )
  #expect(secondFailure?.contains("\"jar\"") == true)
  #expect(secondCalls.callCount > 0)
  #expect(firstCalls.callCount == firstCallsAfterReplace)

  try runtime.clearResourceProvider()
  let secondCallsAfterClear = secondCalls.callCount

  let clearedFailure = try loadProbeStyle(
    runtime: runtime,
    map: map,
    styleURL: "jar:file:/packaged/third.json"
  )
  #expect(clearedFailure?.contains("\"jar\"") == true)
  #expect(firstCalls.callCount == firstCallsAfterReplace)
  #expect(secondCalls.callCount == secondCallsAfterClear)

  // Clearing an already cleared provider stays a successful no-op.
  try runtime.clearResourceProvider()
}

private let providerStyleJSON = #"{"version":8,"sources":{},"layers":[]}"#

private final class ResolvedURLCapture: @unchecked Sendable {
  private let lock = NSLock()
  private var url: String?

  func store(_ url: String) {
    lock.withLock { self.url = url }
  }

  var value: String? {
    lock.withLock { url }
  }
}

/// BND-155: the default tile server's `maplibre:` scheme alias reaches the
/// provider as the alias, alongside the URL the built-in network path would
/// have fetched.
@Test func resourceProviderSeesSchemeAliasAndItsResolvedURL() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }

  let resolved = ResolvedURLCapture()
  try runtime.setResourceProvider { request, handle in
    guard request.requestedUrl == "maplibre://maps/style" else {
      return .passThrough
    }
    resolved.store(request.resolvedUrl)
    try? handle.complete(ResourceResponse(
      status: .ok,
      bytes: Data(providerStyleJSON.utf8)
    ))
    return .handle
  }

  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64)
  )
  defer { try? map.close() }

  try map.setStyleURL("maplibre://maps/style")
  let loaded = try pumpUntilEvent(
    runtime,
    waitingFor: "the provider-served style to load"
  ) { $0.type == .mapStyleLoaded }

  #expect(loaded != nil)
  #expect(resolved.value == "https://demotiles.maplibre.org/style.json")
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
    release: { _ in counters.released() },
    setCancelCallback: { _, _, _ in
      Issue.record("this test registers no cancel callback")
      return false
    }
  )
  let state = NativeResourceProviderState(handleFunctions: functions) { _, _ in
    Issue.record("malformed request should not reach provider callback")
    return MLN_RESOURCE_PROVIDER_DECISION_HANDLE.rawValue
  }

  var request = mln_resource_request()
  request.size = UInt32(MemoryLayout<mln_resource_request>.size)
  let decision = state.invokeForTesting(
    request: request,
    rawHandle: SyntheticHandles.resourceRequest(0x9).raw
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
    release: { _ in counters.released() },
    setCancelCallback: { _, _, _ in
      Issue.record("this test registers no cancel callback")
      return false
    }
  )
  let state = try NativeResourceRequestHandleState(
    handle: SyntheticHandles.resourceRequest(0x5),
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
    release: { _ in counters.released() },
    setCancelCallback: { _, _, _ in
      Issue.record("this test registers no cancel callback")
      return false
    }
  )
  let state = try NativeResourceRequestHandleState(
    handle: SyntheticHandles.resourceRequest(0x5),
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
    release: { _ in counters.released() },
    setCancelCallback: { _, _, _ in
      Issue.record("this test registers no cancel callback")
      return false
    }
  )
  let state = try NativeResourceRequestHandleState(
    handle: SyntheticHandles.resourceRequest(0x6),
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
    release: { _ in counters.released() },
    setCancelCallback: { _, _, _ in
      Issue.record("this test registers no cancel callback")
      return false
    }
  )
  let state =
    NativeResourceProviderState(handleFunctions: functions) { nativeRequest, nativeHandle in
      let request = ResourceRequest(native: nativeRequest)
      #expect(request.requestedUrl == "maplibre://tiles/2/1/1.pbf")
      #expect(request.resolvedUrl == "https://example.test/tile")
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
    .withCString("maplibre://tiles/2/1/1.pbf") { requestedURL in
      try NativeString.withCString("https://example.test/tile") { resolvedURL in
        try NativeString.withCString("etag") { etag in
          priorData.withUnsafeBufferPointer { priorData in
            var request = mln_resource_request()
            request.size = UInt32(MemoryLayout<mln_resource_request>.size)
            request.requested_url = requestedURL
            request.resolved_url = resolvedURL
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
              rawHandle: SyntheticHandles.resourceRequest(0x4).raw
            )
          }
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
    release: { _ in counters.released() },
    setCancelCallback: { _, _, _ in
      Issue.record("this test registers no cancel callback")
      return false
    }
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
      request.requested_url = url
      request.resolved_url = url
      return state.invokeForTesting(
        request: request,
        rawHandle: SyntheticHandles.resourceRequest(0x7).raw
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
    release: { _ in counters.released() },
    setCancelCallback: { _, _, _ in
      Issue.record("this test registers no cancel callback")
      return false
    }
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
      request.requested_url = url
      request.resolved_url = url
      return state.invokeForTesting(
        request: request,
        rawHandle: SyntheticHandles.resourceRequest(0x8).raw
      )
    }

  #expect(decision == MLN_RESOURCE_PROVIDER_DECISION_HANDLE.rawValue)
  #expect(counters.snapshot().complete == 1)
  #expect(counters.snapshot().release == 1)
}

private final class CancelProbe: @unchecked Sendable {
  private let lock = NSLock()
  private var storedHandle: ResourceRequestHandle?
  private var cancelCount = 0
  private var providerCalled = false

  func store(_ handle: ResourceRequestHandle) {
    lock.withLock {
      storedHandle = handle
      providerCalled = true
    }
  }

  var handle: ResourceRequestHandle? {
    lock.withLock { storedHandle }
  }

  var wasProviderCalled: Bool {
    lock.withLock { providerCalled }
  }

  func recordCancel() {
    lock.withLock { cancelCount += 1 }
  }

  var cancels: Int {
    lock.withLock { cancelCount }
  }
}

/// Pumps the runtime until the condition holds, and reports a timeout as a
/// failure the way `pumpUntilEvent` does.
private func pumpUntil(
  _ runtime: RuntimeHandle,
  waitingFor subject: String,
  timeout: TimeInterval = 10,
  condition: () throws -> Bool
) throws -> Bool {
  let deadline = Date().addingTimeInterval(timeout)
  while Date() < deadline {
    if try condition() { return true }
    try runtime.pump()
    _ = try runtime.drainEvents()
    Thread.sleep(forTimeInterval: 0.001)
  }
  if try condition() { return true }
  Issue.record("timed out waiting for \(subject)")
  return false
}

/// Pumps the runtime for a fixed number of turns, for a test that asserts an
/// event stays absent.
private func pumpTurns(
  _ runtime: RuntimeHandle,
  count: Int = 200
) throws {
  for _ in 0 ..< count {
    try runtime.pump()
    _ = try runtime.drainEvents()
    Thread.sleep(forTimeInterval: 0.001)
  }
}

/// Installs a provider that takes every request and keeps the handle, then
/// requests a style through it. `configure` runs inside the provider callback
/// with the handle, before the provider returns. The request stays open unless
/// `configure` completes it.
private func startCancelProbeRequest(
  runtime: RuntimeHandle,
  map: MapHandle,
  probe: CancelProbe,
  configure: @escaping @Sendable (ResourceRequestHandle) -> Void = { _ in }
) throws -> Bool {
  try runtime.setResourceProvider { _, handle in
    probe.store(handle)
    configure(handle)
    return .handle
  }
  try map.setStyleURL("custom://cancel-style.json")
  return try pumpUntil(
    runtime,
    waitingFor: "the provider to take the request"
  ) { probe.wasProviderCalled }
}

/// BND-198: closing a map discards its pending style request, and MapLibre
/// runs the registered cancel callback once for the request the provider still
/// holds. A second registration reports invalid state and leaves the first in
/// place, and a closed request rejects a registration.
@Test func resourceRequestCancelCallbackRunsWhenTheMapDiscardsTheRequest(
) throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64)
  )

  let probe = CancelProbe()
  let secondCalls = ResourceProviderCallCounter()
  let secondRegistration = ResourceCancellationResult()
  let configure: @Sendable (ResourceRequestHandle) -> Void = { handle in
    try? handle.setCancelCallback { probe.recordCancel() }
    secondRegistration.store(Result {
      try handle.setCancelCallback { secondCalls.recordCall() }
      return true
    })
  }
  #expect(try startCancelProbeRequest(
    runtime: runtime,
    map: map,
    probe: probe,
    configure: configure
  ))
  #expect(probe.cancels == 0)
  switch secondRegistration.load() {
  case let .failure(error as MaplibreError):
    #expect(error.kind == .invalidState)
  case let other:
    Issue
      .record(
        "a second registration should report invalid state: \(other.debugDescription)"
      )
  }

  try map.close()
  #expect(try pumpUntil(runtime, waitingFor: "the cancel callback") {
    probe.cancels > 0
  })
  try pumpTurns(runtime, count: 50)
  #expect(probe.cancels == 1)
  #expect(secondCalls.callCount == 0)

  let handle = try #require(probe.handle)
  #expect(try handle.isCancelled())
  #expect(throws: MaplibreError.self) {
    try handle.complete(ResourceResponse(status: .ok, bytes: emptyStyleJSON))
  }

  handle.close()
  do {
    try handle.setCancelCallback { probe.recordCancel() }
    Issue.record("a closed request should reject a cancel callback")
  } catch let error as MaplibreError {
    #expect(error.diagnostic.contains("closed"))
  }
  #expect(probe.cancels == 1)
}

/// BND-198: the cancel callback may close its own request. Native release
/// returns at once from inside the callback instead of waiting for it.
@Test func resourceRequestCancelCallbackMayCloseTheRequest() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64)
  )

  let probe = CancelProbe()
  #expect(try startCancelProbeRequest(
    runtime: runtime,
    map: map,
    probe: probe
  ) { handle in
    try? handle.setCancelCallback {
      probe.recordCancel()
      handle.close()
    }
  })

  try map.close()
  #expect(try pumpUntil(runtime, waitingFor: "the cancel callback") {
    probe.cancels > 0
  })

  let handle = try #require(probe.handle)
  do {
    try handle.setCancelCallback { probe.recordCancel() }
    Issue.record("a closed request should reject a cancel callback")
  } catch let error as MaplibreError {
    #expect(error.diagnostic.contains("closed"))
  }
  #expect(probe.cancels == 1)
}

/// BND-198: registering on a request that MapLibre already cancelled runs the
/// callback on the calling thread before the registration returns.
@Test func resourceRequestCancelCallbackRunsInlineForACancelledRequest(
) throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64)
  )

  let probe = CancelProbe()
  #expect(try startCancelProbeRequest(
    runtime: runtime,
    map: map,
    probe: probe
  ))
  let handle = try #require(probe.handle)

  try map.close()
  #expect(try pumpUntil(runtime, waitingFor: "the request to be cancelled") {
    try handle.isCancelled()
  })

  let callbackThread = ResolvedURLCapture()
  try handle.setCancelCallback {
    probe.recordCancel()
    callbackThread.store(Thread.current.description)
  }
  #expect(probe.cancels == 1)
  #expect(callbackThread.value == Thread.current.description)
  handle.close()
}

/// BND-198: MapLibre retires a request the provider answered, and that
/// teardown leaves the cancel callback alone.
@Test func resourceRequestCancelCallbackSkipsACompletedRequest() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64)
  )

  let probe = CancelProbe()
  #expect(try startCancelProbeRequest(
    runtime: runtime,
    map: map,
    probe: probe
  ) { handle in
    try? handle.setCancelCallback { probe.recordCancel() }
    try? handle.complete(ResourceResponse(
      status: .ok,
      bytes: Data(providerStyleJSON.utf8)
    ))
  })
  #expect(try pumpUntilEvent(runtime, waitingFor: "the provider-served style") {
    $0.type == .mapStyleLoaded
  } != nil)

  try map.close()
  try pumpTurns(runtime)

  #expect(probe.cancels == 0)
  // Completing a request retires the binding's handle along with it.
  let handle = try #require(probe.handle)
  #expect(throws: MaplibreError.self) {
    try handle.setCancelCallback { probe.recordCancel() }
  }
}

private final class CancelRegistrationRecorder: @unchecked Sendable {
  typealias Registration = (
    callback: mln_resource_request_cancel_callback?,
    userData: UnsafeMutableRawPointer?
  )

  private let lock = NSLock()
  private var registrations: [Registration] = []

  func record(
    _ callback: mln_resource_request_cancel_callback?,
    _ userData: UnsafeMutableRawPointer?
  ) {
    lock.withLock { registrations.append((callback, userData)) }
  }

  var count: Int {
    lock.withLock { registrations.count }
  }

  func token(_ index: Int) -> UInt? {
    lock.withLock {
      index < registrations.count
        ? registrations[index].userData.map { UInt(bitPattern: $0) }
        : nil
    }
  }

  /// Calls a registration the way the MapLibre thread does.
  func invoke(_ index: Int) {
    let registration = lock.withLock {
      index < registrations.count ? registrations[index] : nil
    }
    guard let registration else {
      Issue.record("no registration at index \(index)")
      return
    }
    registration.callback?(registration.userData)
  }
}

private func makeRecordedHandleState(
  recorder: CancelRegistrationRecorder,
  counters: ResourceCounters,
  ordinal: UInt64,
  alreadyCancelled: Bool = false
) throws -> NativeResourceRequestHandleState {
  let state = try NativeResourceRequestHandleState(
    handle: SyntheticHandles.resourceRequest(ordinal),
    functions: NativeResourceRequestHandleFunctions(
      complete: { _, _ in counters.completed() },
      cancelled: { _ in alreadyCancelled },
      release: { _ in counters.released() },
      setCancelCallback: { _, callback, userData in
        recorder.record(callback, userData)
        return alreadyCancelled
      }
    )
  )
  _ = state
    .finishProviderDecision(MLN_RESOURCE_PROVIDER_DECISION_HANDLE.rawValue)
  return state
}

/// BND-198: the `user_data` that crosses into C is a registry token rather
/// than a pointer into the binding's memory. The token resolves the request
/// weakly, so a repeated invocation runs the callback once, and a token whose
/// request is gone is a no-op instead of a dangling access.
@Test func resourceRequestCancelTokenResolvesWeaklyAndRunsOnce() throws {
  let recorder = CancelRegistrationRecorder()
  let counters = ResourceCounters()
  let calls = ResourceProviderCallCounter()

  var state: NativeResourceRequestHandleState? = try makeRecordedHandleState(
    recorder: recorder,
    counters: counters,
    ordinal: 0xA
  )
  try state?.setCancelCallback { calls.recordCall() }
  let token = try #require(recorder.token(0))
  #expect(token != 0)
  #expect(ResourceRequestCancelRegistry.shared.contains(token))

  recorder.invoke(0)
  recorder.invoke(0)
  #expect(calls.callCount == 1)
  #expect(!ResourceRequestCancelRegistry.shared.contains(token))

  // A registration whose request is released and freed leaves nothing behind
  // for a token to reach.
  state = try makeRecordedHandleState(
    recorder: recorder,
    counters: counters,
    ordinal: 0xB
  )
  try state?.setCancelCallback { calls.recordCall() }
  let secondToken = try #require(recorder.token(1))
  #expect(secondToken != token)
  state?.release()
  #expect(!ResourceRequestCancelRegistry.shared.contains(secondToken))
  state = nil
  recorder.invoke(1)
  #expect(calls.callCount == 1)
  #expect(counters.snapshot().release == 2)
}

/// BND-198: when the C API reports that the request is already cancelled, the
/// binding runs the callback inside the registration call with no lock held,
/// so a release from inside the callback completes without waiting on the
/// registration that carries it.
@Test func resourceRequestCancelCallbackClosesFromInsideRegistration() throws {
  let recorder = CancelRegistrationRecorder()
  let counters = ResourceCounters()
  let state = try makeRecordedHandleState(
    recorder: recorder,
    counters: counters,
    ordinal: 0xC,
    alreadyCancelled: true
  )

  let registrationFinished = DispatchSemaphore(value: 0)
  let cancels = ResourceProviderCallCounter()
  Thread {
    try? state.setCancelCallback {
      cancels.recordCall()
      state.release()
    }
    registrationFinished.signal()
  }.start()

  #expect(registrationFinished
    .wait(timeout: .now() + .seconds(5)) == .success)
  #expect(cancels.callCount == 1)
  #expect(counters.snapshot().release == 1)
  let token = try #require(recorder.token(0))
  #expect(!ResourceRequestCancelRegistry.shared.contains(token))

  do {
    try state.setCancelCallback {}
    Issue.record("a released request should reject a registration")
  } catch let failure as NativeStatusFailure {
    #expect(failure.diagnostic.contains("closed"))
  }
  #expect(recorder.count == 1)
}
