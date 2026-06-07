internal import CMaplibreNativeC
import Foundation

public enum AmbientCacheOperation: UInt32, Sendable, Hashable {
  case resetDatabase = 1
  case packDatabase = 2
  case invalidate = 3
  case clear = 4
}

public enum OfflineRegionDownloadState: UInt32, Sendable, Hashable {
  case inactive = 0
  case active = 1
}

public enum OfflineRegionDefinition: Equatable, Sendable {
  case tilePyramid(styleURL: String, bounds: LatLngBounds, minZoom: Double, maxZoom: Double, pixelRatio: Float, includeIdeographs: Bool)
  case geometry(styleURL: String, geometry: Geometry, minZoom: Double, maxZoom: Double, pixelRatio: Float, includeIdeographs: Bool)

  var nativeDefinition: NativeOfflineRegionDefinition {
    switch self {
    case .tilePyramid(let styleURL, let bounds, let minZoom, let maxZoom, let pixelRatio, let includeIdeographs):
      .tilePyramid(
        styleURL: styleURL,
        bounds: bounds.nativeInput,
        minZoom: minZoom,
        maxZoom: maxZoom,
        pixelRatio: pixelRatio,
        includeIdeographs: includeIdeographs
      )
    case .geometry(let styleURL, let geometry, let minZoom, let maxZoom, let pixelRatio, let includeIdeographs):
      .geometry(
        styleURL: styleURL,
        geometry: geometry.nativeGeometry,
        minZoom: minZoom,
        maxZoom: maxZoom,
        pixelRatio: pixelRatio,
        includeIdeographs: includeIdeographs
      )
    }
  }

  init(native: NativeOfflineRegionDefinition) {
    switch native {
    case .tilePyramid(let styleURL, let bounds, let minZoom, let maxZoom, let pixelRatio, let includeIdeographs):
      self = .tilePyramid(
        styleURL: styleURL,
        bounds: LatLngBounds(native: bounds),
        minZoom: minZoom,
        maxZoom: maxZoom,
        pixelRatio: pixelRatio,
        includeIdeographs: includeIdeographs
      )
    case .geometry(let styleURL, let geometry, let minZoom, let maxZoom, let pixelRatio, let includeIdeographs):
      self = .geometry(
        styleURL: styleURL,
        geometry: Geometry(native: geometry),
        minZoom: minZoom,
        maxZoom: maxZoom,
        pixelRatio: pixelRatio,
        includeIdeographs: includeIdeographs
      )
    }
  }
}

public struct OfflineRegionInfo: Equatable, Sendable {
  public let id: Int64
  public let definition: OfflineRegionDefinition
  public let metadata: Data

  init(native: NativeOfflineRegionInfo) {
    id = native.id
    definition = OfflineRegionDefinition(native: native.definition)
    metadata = native.metadata
  }
}

extension RuntimeHandle {
  public func runAmbientCacheOperationStart(_ operation: AmbientCacheOperation) throws -> UInt64 {
    try mapNativeFailure {
      try NativeOffline.runAmbientCacheOperationStart(try requireLivePointer(), operation: operation.rawValue)
    }
  }

  public func discardOfflineOperation(_ operationId: UInt64) throws {
    try mapNativeFailure {
      try checkStatus(mln_runtime_offline_operation_discard(try requireLivePointer(), operationId))
    }
  }

  public func offlineRegionCreateStart(definition: OfflineRegionDefinition, metadata: Data = Data()) throws -> UInt64 {
    try mapNativeFailure {
      try definition.nativeDefinition.withNativeDefinition { definition in
        try NativeOffline.regionCreateStart(try requireLivePointer(), definition: definition, metadata: metadata)
      }
    }
  }

  public func offlineRegionGetStart(regionId: Int64) throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionGetStart(try requireLivePointer(), regionId: regionId) }
  }

  public func offlineRegionsListStart() throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionsListStart(try requireLivePointer()) }
  }

  public func offlineRegionsMergeDatabaseStart(sideDatabasePath: String) throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionsMergeDatabaseStart(try requireLivePointer(), sideDatabasePath: sideDatabasePath) }
  }

  public func offlineRegionUpdateMetadataStart(regionId: Int64, metadata: Data) throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionUpdateMetadataStart(try requireLivePointer(), regionId: regionId, metadata: metadata) }
  }

  public func offlineRegionGetStatusStart(regionId: Int64) throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionGetStatusStart(try requireLivePointer(), regionId: regionId) }
  }

  public func offlineRegionSetObservedStart(regionId: Int64, observed: Bool) throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionSetObservedStart(try requireLivePointer(), regionId: regionId, observed: observed) }
  }

  public func offlineRegionSetDownloadStateStart(regionId: Int64, state: OfflineRegionDownloadState) throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionSetDownloadStateStart(try requireLivePointer(), regionId: regionId, state: state.rawValue) }
  }

  public func offlineRegionInvalidateStart(regionId: Int64) throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionInvalidateStart(try requireLivePointer(), regionId: regionId) }
  }

  public func offlineRegionDeleteStart(regionId: Int64) throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionDeleteStart(try requireLivePointer(), regionId: regionId) }
  }

  public func offlineRegionCreateTakeResult(operationId: UInt64) throws -> OfflineRegionInfo {
    try mapNativeFailure { try OfflineRegionInfo(native: NativeOffline.regionCreateTakeResult(try requireLivePointer(), operationId: operationId)) }
  }

  public func offlineRegionGetTakeResult(operationId: UInt64) throws -> OfflineRegionInfo? {
    try mapNativeFailure { try NativeOffline.regionGetTakeResult(try requireLivePointer(), operationId: operationId).map(OfflineRegionInfo.init(native:)) }
  }

  public func offlineRegionsListTakeResult(operationId: UInt64) throws -> [OfflineRegionInfo] {
    try mapNativeFailure { try NativeOffline.regionsListTakeResult(try requireLivePointer(), operationId: operationId).map(OfflineRegionInfo.init(native:)) }
  }

  public func offlineRegionsMergeDatabaseTakeResult(operationId: UInt64) throws -> [OfflineRegionInfo] {
    try mapNativeFailure { try NativeOffline.regionsMergeDatabaseTakeResult(try requireLivePointer(), operationId: operationId).map(OfflineRegionInfo.init(native:)) }
  }

  public func offlineRegionUpdateMetadataTakeResult(operationId: UInt64) throws -> OfflineRegionInfo {
    try mapNativeFailure { try OfflineRegionInfo(native: NativeOffline.regionUpdateMetadataTakeResult(try requireLivePointer(), operationId: operationId)) }
  }

  public func offlineRegionGetStatusTakeResult(operationId: UInt64) throws -> OfflineRegionStatus {
    try mapNativeFailure { try OfflineRegionStatus(native: NativeOffline.regionGetStatusTakeResult(try requireLivePointer(), operationId: operationId)) }
  }
}
