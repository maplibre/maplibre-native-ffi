import Foundation

public enum ResourceKind: Sendable, Hashable {
  case unknown(UInt32)
  case style
  case source
  case tile
  case glyphs
  case spriteImage
  case spriteJSON
  case image

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 1: .style
    case 2: .source
    case 3: .tile
    case 4: .glyphs
    case 5: .spriteImage
    case 6: .spriteJSON
    case 7: .image
    default: .unknown(rawValue)
    }
  }
}

public enum ResourceLoadingMethod: Sendable, Hashable {
  case all
  case cacheOnly
  case networkOnly
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 0: .all
    case 1: .cacheOnly
    case 2: .networkOnly
    default: .unknown(rawValue)
    }
  }
}

public enum ResourcePriority: Sendable, Hashable {
  case regular
  case low
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 0: .regular
    case 1: .low
    default: .unknown(rawValue)
    }
  }
}

public enum ResourceUsage: Sendable, Hashable {
  case online
  case offline
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 0: .online
    case 1: .offline
    default: .unknown(rawValue)
    }
  }
}

public enum ResourceStoragePolicy: Sendable, Hashable {
  case permanent
  case volatile
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 0: .permanent
    case 1: .volatile
    default: .unknown(rawValue)
    }
  }
}

public enum ResourceResponseStatus: UInt32, Sendable, Hashable {
  case ok = 0
  case error = 1
  case noContent = 2
  case notModified = 3
}

public enum ResourceErrorReason: Sendable, Hashable {
  case none
  case notFound
  case server
  case connection
  case rateLimit
  case other
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 0: .none
    case 1: .notFound
    case 2: .server
    case 3: .connection
    case 4: .rateLimit
    case 5: .other
    default: .unknown(rawValue)
    }
  }

  public var rawValue: UInt32 {
    switch self {
    case .none: 0
    case .notFound: 1
    case .server: 2
    case .connection: 3
    case .rateLimit: 4
    case .other: 5
    case let .unknown(raw): raw
    }
  }
}

public struct ByteRange: Equatable, Sendable {
  public let start: UInt64
  public let end: UInt64

  public init(start: UInt64, end: UInt64) {
    self.start = start
    self.end = end
  }
}

public struct ResourceRequest: Equatable, Sendable {
  public let url: String
  public let kind: ResourceKind
  public let loadingMethod: ResourceLoadingMethod
  public let priority: ResourcePriority
  public let usage: ResourceUsage
  public let storagePolicy: ResourceStoragePolicy
  public let range: ByteRange?
  public let priorModifiedUnixMilliseconds: Int64?
  public let priorExpiresUnixMilliseconds: Int64?
  public let priorEtag: String?
  public let priorData: Data

  init(native: NativeResourceRequest) {
    url = native.url
    kind = ResourceKind.fromNative(native.kind)
    loadingMethod = ResourceLoadingMethod.fromNative(native.loadingMethod)
    priority = ResourcePriority.fromNative(native.priority)
    usage = ResourceUsage.fromNative(native.usage)
    storagePolicy = ResourceStoragePolicy.fromNative(native.storagePolicy)
    range = native.range.map { ByteRange(start: $0.start, end: $0.end) }
    priorModifiedUnixMilliseconds = native.priorModifiedUnixMilliseconds
    priorExpiresUnixMilliseconds = native.priorExpiresUnixMilliseconds
    priorEtag = native.priorEtag
    priorData = Data(native.priorData)
  }
}

public struct ResourceResponse: Equatable, Sendable {
  public var status: ResourceResponseStatus
  public var errorReason: ResourceErrorReason
  public var bytes: Data
  public var errorMessage: String?
  public var mustRevalidate: Bool
  public var modifiedUnixMilliseconds: Int64?
  public var expiresUnixMilliseconds: Int64?
  public var etag: String?
  public var retryAfterUnixMilliseconds: Int64?

  public init(
    status: ResourceResponseStatus,
    errorReason: ResourceErrorReason = .none,
    bytes: Data = Data(),
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

  var nativeInput: NativeResourceResponseInput {
    NativeResourceResponseInput(
      status: status.rawValue,
      errorReason: errorReason.rawValue,
      bytes: Array(bytes),
      errorMessage: errorMessage,
      mustRevalidate: mustRevalidate,
      modifiedUnixMilliseconds: modifiedUnixMilliseconds,
      expiresUnixMilliseconds: expiresUnixMilliseconds,
      etag: etag,
      retryAfterUnixMilliseconds: retryAfterUnixMilliseconds
    )
  }
}

public final class ResourceRequestHandle: @unchecked Sendable {
  private let state: NativeResourceRequestHandleState

  init(state: NativeResourceRequestHandleState) {
    self.state = state
  }

  public func complete(_ response: ResourceResponse) throws {
    if case let .unknown(raw) = response.errorReason {
      throw NativeStatusFailure.swiftInvalidArgument(
        "ResourceErrorReason.unknown(\(raw)) cannot be sent back to native"
      )
    }
    try mapNativeFailure {
      try state.complete(response.nativeInput)
    }
  }

  public func isCancelled() throws -> Bool {
    try mapNativeFailure {
      try state.isCancelled()
    }
  }

  public func close() {
    state.release()
  }
}

public enum ResourceProviderDecision: Sendable, Hashable {
  case passThrough
  case handle
}

public struct ResourceTransformRequest: Equatable, Sendable {
  public let kind: ResourceKind
  public let url: String

  init(native: NativeResourceTransformRequest) {
    kind = ResourceKind.fromNative(native.kind)
    url = native.url
  }
}
