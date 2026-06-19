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
  case tilePyramid(
    styleURL: String,
    bounds: LatLngBounds,
    minZoom: Double,
    maxZoom: Double,
    pixelRatio: Float,
    includeIdeographs: Bool
  )
  case geometry(
    styleURL: String,
    geometry: Geometry,
    minZoom: Double,
    maxZoom: Double,
    pixelRatio: Float,
    includeIdeographs: Bool
  )

  var nativeDefinition: NativeOfflineRegionDefinition {
    switch self {
    case let .tilePyramid(
      styleURL,
      bounds,
      minZoom,
      maxZoom,
      pixelRatio,
      includeIdeographs
    ):
      .tilePyramid(
        styleURL: styleURL,
        bounds: bounds.nativeInput,
        minZoom: minZoom,
        maxZoom: maxZoom,
        pixelRatio: pixelRatio,
        includeIdeographs: includeIdeographs
      )
    case let .geometry(
      styleURL,
      geometry,
      minZoom,
      maxZoom,
      pixelRatio,
      includeIdeographs
    ):
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
    case let .tilePyramid(
      styleURL,
      bounds,
      minZoom,
      maxZoom,
      pixelRatio,
      includeIdeographs
    ):
      self = .tilePyramid(
        styleURL: styleURL,
        bounds: LatLngBounds(native: bounds),
        minZoom: minZoom,
        maxZoom: maxZoom,
        pixelRatio: pixelRatio,
        includeIdeographs: includeIdeographs
      )
    case let .geometry(
      styleURL,
      geometry,
      minZoom,
      maxZoom,
      pixelRatio,
      includeIdeographs
    ):
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

public extension RuntimeHandle {
  func runAmbientCacheOperationStart(_ operation: AmbientCacheOperation) throws
    -> UInt64
  {
    try mapNativeFailure {
      try NativeOffline.runAmbientCacheOperationStart(
        requireLivePointer(),
        operation: operation.rawValue
      )
    }
  }

  func discardOfflineOperation(_ operationId: UInt64) throws {
    try mapNativeFailure {
      try checkStatus(mln_runtime_offline_operation_discard(
        requireLivePointer(),
        operationId
      ))
    }
  }

  func offlineRegionCreateStart(
    definition: OfflineRegionDefinition,
    metadata: Data = Data()
  ) throws -> UInt64 {
    try mapNativeFailure {
      try definition.nativeDefinition.withNativeDefinition { definition in
        try NativeOffline.regionCreateStart(
          requireLivePointer(),
          definition: definition,
          metadata: metadata
        )
      }
    }
  }

  func offlineRegionGetStart(regionId: Int64) throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionGetStart(
      requireLivePointer(),
      regionId: regionId
    ) }
  }

  func offlineRegionsListStart() throws -> UInt64 {
    try mapNativeFailure {
      try NativeOffline.regionsListStart(requireLivePointer())
    }
  }

  func offlineRegionsMergeDatabaseStart(sideDatabasePath: String) throws
    -> UInt64
  {
    try mapNativeFailure { try NativeOffline.regionsMergeDatabaseStart(
      requireLivePointer(),
      sideDatabasePath: sideDatabasePath
    ) }
  }

  func offlineRegionUpdateMetadataStart(regionId: Int64,
                                        metadata: Data) throws -> UInt64
  {
    try mapNativeFailure { try NativeOffline.regionUpdateMetadataStart(
      requireLivePointer(),
      regionId: regionId,
      metadata: metadata
    ) }
  }

  func offlineRegionGetStatusStart(regionId: Int64) throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionGetStatusStart(
      requireLivePointer(),
      regionId: regionId
    ) }
  }

  func offlineRegionSetObservedStart(regionId: Int64,
                                     observed: Bool) throws -> UInt64
  {
    try mapNativeFailure { try NativeOffline.regionSetObservedStart(
      requireLivePointer(),
      regionId: regionId,
      observed: observed
    ) }
  }

  func offlineRegionSetDownloadStateStart(
    regionId: Int64,
    state: OfflineRegionDownloadState
  ) throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionSetDownloadStateStart(
      requireLivePointer(),
      regionId: regionId,
      state: state.rawValue
    ) }
  }

  func offlineRegionInvalidateStart(regionId: Int64) throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionInvalidateStart(
      requireLivePointer(),
      regionId: regionId
    ) }
  }

  func offlineRegionDeleteStart(regionId: Int64) throws -> UInt64 {
    try mapNativeFailure { try NativeOffline.regionDeleteStart(
      requireLivePointer(),
      regionId: regionId
    ) }
  }

  func offlineRegionCreateTakeResult(operationId: UInt64) throws
    -> OfflineRegionInfo
  {
    try mapNativeFailure {
      try OfflineRegionInfo(native: NativeOffline.regionCreateTakeResult(
        requireLivePointer(),
        operationId: operationId
      ))
    }
  }

  func offlineRegionGetTakeResult(operationId: UInt64) throws
    -> OfflineRegionInfo?
  {
    try mapNativeFailure { try NativeOffline.regionGetTakeResult(
      requireLivePointer(),
      operationId: operationId
    ).map(OfflineRegionInfo.init(native:)) }
  }

  func offlineRegionsListTakeResult(operationId: UInt64) throws
    -> [OfflineRegionInfo]
  {
    try mapNativeFailure { try NativeOffline.regionsListTakeResult(
      requireLivePointer(),
      operationId: operationId
    ).map(OfflineRegionInfo.init(native:)) }
  }

  func offlineRegionsMergeDatabaseTakeResult(operationId: UInt64) throws
    -> [OfflineRegionInfo]
  {
    try mapNativeFailure { try NativeOffline.regionsMergeDatabaseTakeResult(
      requireLivePointer(),
      operationId: operationId
    ).map(OfflineRegionInfo.init(native:)) }
  }

  func offlineRegionUpdateMetadataTakeResult(operationId: UInt64) throws
    -> OfflineRegionInfo
  {
    try mapNativeFailure {
      try OfflineRegionInfo(native: NativeOffline
        .regionUpdateMetadataTakeResult(
          requireLivePointer(),
          operationId: operationId
        ))
    }
  }

  func offlineRegionGetStatusTakeResult(operationId: UInt64) throws
    -> OfflineRegionStatus
  {
    try mapNativeFailure {
      try OfflineRegionStatus(native: NativeOffline
        .regionGetStatusTakeResult(
          requireLivePointer(),
          operationId: operationId
        ))
    }
  }
}
