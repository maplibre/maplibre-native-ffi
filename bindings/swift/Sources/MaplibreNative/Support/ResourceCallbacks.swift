internal import CMaplibreNativeC
import Foundation

struct NativeByteRange: Equatable {
  let start: UInt64
  let end: UInt64
}

struct NativeResourceRequest: Equatable {
  let requestedUrl: String
  let resolvedUrl: String
  let kind: UInt32
  let loadingMethod: UInt32
  let priority: UInt32
  let usage: UInt32
  let storagePolicy: UInt32
  let range: NativeByteRange?
  let priorModifiedUnixMilliseconds: Int64?
  let priorExpiresUnixMilliseconds: Int64?
  let priorEtag: String?
  let priorData: [UInt8]

  init(_ raw: mln_resource_request) throws {
    guard raw.requested_url != nil, raw.resolved_url != nil else {
      throw NativeStatusFailure(
        rawStatus: MLN_STATUS_INVALID_ARGUMENT.rawValue,
        diagnostic: "resource request requested_url or resolved_url is null"
      )
    }
    requestedUrl = NativeString.copyCString(raw.requested_url)
    resolvedUrl = NativeString.copyCString(raw.resolved_url)
    kind = raw.kind
    loadingMethod = raw.loading_method
    priority = raw.priority
    usage = raw.usage
    storagePolicy = raw.storage_policy
    range = raw.has_range ? NativeByteRange(
      start: raw.range_start,
      end: raw.range_end
    ) : nil
    priorModifiedUnixMilliseconds = raw.has_prior_modified ? raw
      .prior_modified_unix_ms : nil
    priorExpiresUnixMilliseconds = raw.has_prior_expires ? raw
      .prior_expires_unix_ms : nil
    priorEtag = raw.prior_etag.map { String(cString: $0) }
    if raw.prior_data_size > 0, let priorData = raw.prior_data {
      self.priorData = Array(UnsafeBufferPointer(
        start: priorData,
        count: raw.prior_data_size
      ))
    } else {
      priorData = []
    }
  }
}

struct NativeResourceResponseInput: Equatable {
  let status: UInt32
  let errorReason: UInt32
  let bytes: [UInt8]
  let errorMessage: String?
  let mustRevalidate: Bool
  let modifiedUnixMilliseconds: Int64?
  let expiresUnixMilliseconds: Int64?
  let etag: String?
  let retryAfterUnixMilliseconds: Int64?

  init(
    status: UInt32,
    errorReason: UInt32,
    bytes: [UInt8] = [],
    errorMessage: String? = nil,
    mustRevalidate: Bool = false,
    modifiedUnixMilliseconds: Int64? = nil,
    expiresUnixMilliseconds: Int64? = nil,
    etag: String? = nil,
    retryAfterUnixMilliseconds: Int64? = nil
  ) {
    self.status = status
    self.errorReason = errorReason
    self.bytes = bytes
    self.errorMessage = errorMessage
    self.mustRevalidate = mustRevalidate
    self.modifiedUnixMilliseconds = modifiedUnixMilliseconds
    self.expiresUnixMilliseconds = expiresUnixMilliseconds
    self.etag = etag
    self.retryAfterUnixMilliseconds = retryAfterUnixMilliseconds
  }

  func withNativeResponse<Result>(
    _ body: (UnsafePointer<mln_resource_response>) throws -> Result
  ) throws -> Result {
    try NativeString.withOptionalCString(errorMessage) { errorMessage in
      try NativeString.withOptionalCString(etag) { etag in
        try bytes.withUnsafeBufferPointer { bytes in
          var response = mln_resource_response()
          response.size = UInt32(MemoryLayout<mln_resource_response>.size)
          response.status = status
          response.error_reason = errorReason
          response.bytes = bytes.baseAddress
          response.byte_count = bytes.count
          response.error_message = errorMessage
          response.must_revalidate = mustRevalidate
          response.has_modified = modifiedUnixMilliseconds != nil
          response.modified_unix_ms = modifiedUnixMilliseconds ?? 0
          response.has_expires = expiresUnixMilliseconds != nil
          response.expires_unix_ms = expiresUnixMilliseconds ?? 0
          response.etag = etag
          response.has_retry_after = retryAfterUnixMilliseconds != nil
          response.retry_after_unix_ms = retryAfterUnixMilliseconds ?? 0
          return try withUnsafePointer(to: &response, body)
        }
      }
    }
  }
}

struct NativeResourceRequestHandleFunctions {
  let complete: @Sendable (
    NativeResourceRequestHandle,
    NativeResourceResponseInput
  ) throws
    -> Void
  let cancelled: @Sendable (NativeResourceRequestHandle) throws -> Bool
  let release: @Sendable (NativeResourceRequestHandle) -> Void

  static let native = Self(
    complete: { handle, response in
      try response.withNativeResponse { nativeResponse in
        try checkStatus(mln_resource_request_complete(
          handle.raw,
          nativeResponse
        ))
      }
    },
    cancelled: { handle in
      try NativeMemory.withTemporary(false) { cancelled in
        try checkStatus(mln_resource_request_cancelled(handle.raw, cancelled))
      }.value
    },
    release: { handle in
      mln_resource_request_release(handle.raw)
    }
  )
}

final class NativeResourceRequestHandleState: @unchecked Sendable {
  private enum ProviderOwnership {
    case pending
    case nativeWillRelease
    case providerOwned
  }

  private let functions: NativeResourceRequestHandleFunctions
  private let condition = NSCondition()
  private var handle: NativeResourceRequestHandle?
  private var providerOwnership = ProviderOwnership.pending
  private var finalizedProviderDecision: UInt32?
  private var completed = false
  private var releaseRequested = false
  private var inFlightOperations = 0

  init(
    handle: NativeResourceRequestHandle,
    functions: NativeResourceRequestHandleFunctions = .native
  ) throws {
    guard !handle.isNull else {
      throw NativeStatusFailure(
        rawStatus: 0,
        diagnostic: "resource request handle is the null handle"
      )
    }
    self.handle = handle
    self.functions = functions
  }

  deinit {
    release()
  }

  func finishProviderDecision(_ decision: UInt32) -> UInt32 {
    let result = condition.withLock {
      while inFlightOperations > 0 {
        condition.wait()
      }
      if let finalizedProviderDecision {
        return (
          decision: finalizedProviderDecision,
          handle: takeReleasableHandleLocked()
        )
      }
      if completed || decision == MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        .rawValue
      {
        providerOwnership = .providerOwned
        finalizedProviderDecision = MLN_RESOURCE_PROVIDER_DECISION_HANDLE
          .rawValue
      } else {
        providerOwnership = .nativeWillRelease
        finalizedProviderDecision = decision
        handle = nil
        releaseRequested = true
      }
      return (
        decision: finalizedProviderDecision ?? decision,
        handle: takeReleasableHandleLocked()
      )
    }
    if let handle = result.handle {
      functions.release(handle)
    }
    return result.decision
  }

  func complete(_ response: NativeResourceResponseInput) throws {
    let handle = try beginCompletionOperation()
    do {
      try functions.complete(handle, response)
    } catch {
      finishNativeOperation()
      throw error
    }
    finishNativeOperation()
  }

  func isCancelled() throws -> Bool {
    let handle = try beginNativeOperation()
    defer { finishNativeOperation() }
    return try functions.cancelled(handle)
  }

  func release() {
    let handle = condition.withLock {
      releaseRequested = true
      while providerOwnership == .providerOwned, inFlightOperations > 0 {
        condition.wait()
      }
      return takeReleasableHandleLocked()
    }
    if let handle {
      functions.release(handle)
    }
  }

  private func beginNativeOperation() throws -> NativeResourceRequestHandle {
    try condition.withLock {
      guard !releaseRequested, let handle else {
        throw NativeStatusFailure(
          rawStatus: 0,
          diagnostic: "resource request handle is closed"
        )
      }
      inFlightOperations += 1
      return handle
    }
  }

  private func beginCompletionOperation() throws
    -> NativeResourceRequestHandle
  {
    try condition.withLock {
      guard !releaseRequested, let handle else {
        throw NativeStatusFailure(
          rawStatus: 0,
          diagnostic: "resource request handle is closed"
        )
      }
      guard !completed else {
        throw NativeStatusFailure(
          rawStatus: 0,
          diagnostic: "resource request handle is already completed"
        )
      }
      completed = true
      inFlightOperations += 1
      return handle
    }
  }

  private func finishNativeOperation() {
    let handle = condition.withLock {
      inFlightOperations -= 1
      let handle = takeReleasableHandleLocked()
      condition.broadcast()
      return handle
    }
    if let handle {
      functions.release(handle)
    }
  }

  private func takeReleasableHandleLocked() -> NativeResourceRequestHandle? {
    guard providerOwnership == .providerOwned, inFlightOperations == 0,
          completed || releaseRequested else { return nil }
    let releasable = handle
    handle = nil
    return releasable
  }
}

struct NativeResourceTransformRequest: Equatable {
  let kind: UInt32
  let url: String
}

private final class NativeResourceTransformBox: @unchecked Sendable {
  private let callback: @Sendable (NativeResourceTransformRequest) -> String?

  init(_ callback: @escaping @Sendable (NativeResourceTransformRequest)
    -> String?)
  {
    self.callback = callback
  }

  func invoke(
    kind: UInt32,
    url: UnsafePointer<CChar>?,
    outResponse: UnsafeMutablePointer<mln_resource_transform_response>?
  ) -> mln_status {
    guard let outResponse else { return MLN_STATUS_INVALID_ARGUMENT }
    outResponse.pointee
      .size = UInt32(MemoryLayout<mln_resource_transform_response>.size)
    outResponse.pointee.url = nil
    let request = NativeResourceTransformRequest(
      kind: kind,
      url: NativeString.copyCString(url)
    )
    guard let replacement = callback(request), !replacement.isEmpty else {
      return MLN_STATUS_OK
    }
    if replacement.utf8.contains(0) {
      return MLN_STATUS_INVALID_ARGUMENT
    }
    return replacement.withCString { replacementURL in
      mln_resource_transform_response_set_url(
        outResponse,
        replacementURL,
        replacement.utf8.count
      )
    }
  }
}

private func resourceTransformTrampoline(
  userData: UnsafeMutableRawPointer?,
  kind: UInt32,
  url: UnsafePointer<CChar>?,
  outResponse: UnsafeMutablePointer<mln_resource_transform_response>?
) -> mln_status {
  guard let userData else { return MLN_STATUS_INVALID_ARGUMENT }
  let box = Unmanaged<NativeResourceTransformBox>.fromOpaque(userData)
    .takeUnretainedValue()
  return box.invoke(kind: kind, url: url, outResponse: outResponse)
}

final class NativeResourceTransformState: @unchecked Sendable {
  private let retainedBox: Unmanaged<NativeResourceTransformBox>

  init(_ callback: @escaping @Sendable (NativeResourceTransformRequest)
    -> String?)
  {
    retainedBox = Unmanaged.passRetained(NativeResourceTransformBox(callback))
  }

  deinit {
    retainedBox.release()
  }

  func invokeForTesting(kind: UInt32,
                        url: String) -> (status: Int32, replacement: String?)
  {
    var response = mln_resource_transform_response()
    let status = url.withCString { url in
      retainedBox.takeUnretainedValue().invoke(
        kind: kind,
        url: url,
        outResponse: &response
      )
    }
    return (status.rawValue, response.url.map { String(cString: $0) })
  }

  func withDescriptor<Result>(
    _ body: (UnsafePointer<mln_resource_transform>) throws -> Result
  ) throws -> Result {
    var transform = mln_resource_transform()
    transform.size = UInt32(MemoryLayout<mln_resource_transform>.size)
    transform.callback = resourceTransformTrampoline
    transform.user_data = retainedBox.toOpaque()
    return try withUnsafePointer(to: &transform, body)
  }
}

private final class NativeResourceProviderBox: @unchecked Sendable {
  private let callback: @Sendable (
    NativeResourceRequest,
    NativeResourceRequestHandleState
  ) -> UInt32
  private let handleFunctions: NativeResourceRequestHandleFunctions

  init(
    handleFunctions: NativeResourceRequestHandleFunctions,
    callback: @escaping @Sendable (
      NativeResourceRequest,
      NativeResourceRequestHandleState
    ) -> UInt32
  ) {
    self.handleFunctions = handleFunctions
    self.callback = callback
  }

  func invoke(
    request: UnsafePointer<mln_resource_request>?,
    rawHandle: mln_resource_request_handle
  ) -> UInt32 {
    guard let request else {
      return UInt32.max
    }

    var state: NativeResourceRequestHandleState?
    do {
      let createdState = try NativeResourceRequestHandleState(
        handle: NativeResourceRequestHandle(raw: rawHandle),
        functions: handleFunctions
      )
      state = createdState
      let copiedRequest = try NativeResourceRequest(request.pointee)
      let decision = callback(copiedRequest, createdState)
      return createdState.finishProviderDecision(decision)
    } catch {
      _ = state?.finishProviderDecision(UInt32.max)
      return UInt32.max
    }
  }
}

private func resourceProviderTrampoline(
  userData: UnsafeMutableRawPointer?,
  request: UnsafePointer<mln_resource_request>?,
  rawHandle: mln_resource_request_handle
) -> UInt32 {
  guard let userData else { return UInt32.max }
  let box = Unmanaged<NativeResourceProviderBox>.fromOpaque(userData)
    .takeUnretainedValue()
  return box.invoke(request: request, rawHandle: rawHandle)
}

final class NativeResourceProviderState: @unchecked Sendable {
  private let retainedBox: Unmanaged<NativeResourceProviderBox>

  init(
    handleFunctions: NativeResourceRequestHandleFunctions = .native,
    _ callback: @escaping @Sendable (
      NativeResourceRequest,
      NativeResourceRequestHandleState
    ) -> UInt32
  ) {
    retainedBox = Unmanaged.passRetained(
      NativeResourceProviderBox(
        handleFunctions: handleFunctions,
        callback: callback
      )
    )
  }

  deinit {
    retainedBox.release()
  }

  func invokeForTesting(request: mln_resource_request,
                        rawHandle: mln_resource_request_handle) -> UInt32
  {
    withUnsafePointer(to: request) { request in
      retainedBox.takeUnretainedValue().invoke(
        request: request,
        rawHandle: rawHandle
      )
    }
  }

  func withDescriptor<Result>(
    _ body: (UnsafePointer<mln_resource_provider>) throws -> Result
  ) throws -> Result {
    var provider = mln_resource_provider()
    provider.size = UInt32(MemoryLayout<mln_resource_provider>.size)
    provider.callback = resourceProviderTrampoline
    provider.user_data = retainedBox.toOpaque()
    return try withUnsafePointer(to: &provider, body)
  }
}
