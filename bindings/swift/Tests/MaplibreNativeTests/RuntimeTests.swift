import CMaplibreNativeC
import Foundation
import Testing

@testable import MaplibreNative
@testable import MaplibreNativeSupport

private final class ResourceCounters: @unchecked Sendable {
  private let lock = NSLock()
  private var completeCount = 0
  private var releaseCount = 0

  func completed() {
    lock.withLock { completeCount += 1 }
  }

  func released() {
    lock.withLock { releaseCount += 1 }
  }

  func snapshot() -> (complete: Int, release: Int) {
    lock.withLock { (completeCount, releaseCount) }
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
