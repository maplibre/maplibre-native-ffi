internal import CMaplibreNativeC
import Foundation

enum NativeOffline {
  static func runAmbientCacheOperationStart(
    _ runtime: OpaquePointer,
    operation: UInt32
  ) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_run_ambient_cache_operation_start(
        runtime,
        operation,
        operationId
      ))
    }.value
  }

  static func regionCreateStart(
    _ runtime: OpaquePointer,
    definition: UnsafePointer<mln_offline_region_definition>,
    metadata: Data
  ) throws -> UInt64 {
    try metadata.withUnsafeBytes { bytes in
      try NativeMemory.withTemporary(UInt64(0)) { operationId in
        try checkStatus(mln_runtime_offline_region_create_start(
          runtime,
          definition,
          bytes.bindMemory(to: UInt8.self).baseAddress,
          bytes.count,
          operationId
        ))
      }.value
    }
  }

  static func regionGetStart(_ runtime: OpaquePointer,
                             regionId: Int64) throws -> UInt64
  {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_get_start(
        runtime,
        regionId,
        operationId
      ))
    }.value
  }

  static func regionsListStart(_ runtime: OpaquePointer) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_regions_list_start(
        runtime,
        operationId
      ))
    }.value
  }

  static func regionsMergeDatabaseStart(
    _ runtime: OpaquePointer,
    sideDatabasePath: String
  ) throws -> UInt64 {
    try NativeString.withCString(sideDatabasePath) { path in
      try NativeMemory.withTemporary(UInt64(0)) { operationId in
        try checkStatus(mln_runtime_offline_regions_merge_database_start(
          runtime,
          path,
          operationId
        ))
      }.value
    }
  }

  static func regionUpdateMetadataStart(
    _ runtime: OpaquePointer,
    regionId: Int64,
    metadata: Data
  ) throws -> UInt64 {
    try metadata.withUnsafeBytes { bytes in
      try NativeMemory.withTemporary(UInt64(0)) { operationId in
        try checkStatus(mln_runtime_offline_region_update_metadata_start(
          runtime,
          regionId,
          bytes.bindMemory(to: UInt8.self).baseAddress,
          bytes.count,
          operationId
        ))
      }.value
    }
  }

  static func regionGetStatusStart(_ runtime: OpaquePointer,
                                   regionId: Int64) throws -> UInt64
  {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_get_status_start(
        runtime,
        regionId,
        operationId
      ))
    }.value
  }

  static func regionSetObservedStart(
    _ runtime: OpaquePointer,
    regionId: Int64,
    observed: Bool
  ) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_set_observed_start(
        runtime,
        regionId,
        observed,
        operationId
      ))
    }.value
  }

  static func regionSetDownloadStateStart(
    _ runtime: OpaquePointer,
    regionId: Int64,
    state: UInt32
  ) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_set_download_state_start(
        runtime,
        regionId,
        state,
        operationId
      ))
    }.value
  }

  static func regionInvalidateStart(_ runtime: OpaquePointer,
                                    regionId: Int64) throws -> UInt64
  {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_invalidate_start(
        runtime,
        regionId,
        operationId
      ))
    }.value
  }

  static func regionDeleteStart(_ runtime: OpaquePointer,
                                regionId: Int64) throws -> UInt64
  {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_delete_start(
        runtime,
        regionId,
        operationId
      ))
    }.value
  }

  static func regionCreateTakeResult(
    _ runtime: OpaquePointer,
    operationId: UInt64
  ) throws -> NativeOfflineRegionInfo {
    let snapshot = try NativeMemory
      .withTemporary(OpaquePointer?.none) { snapshot in
        try checkStatus(mln_runtime_offline_region_create_take_result(
          runtime,
          operationId,
          snapshot
        ))
      }.value
    guard let snapshot else { throw NativeStatusFailure(
      rawStatus: 0,
      diagnostic: "offline region create result was null"
    ) }
    defer { mln_offline_region_snapshot_destroy(snapshot) }
    return try offlineRegionSnapshotCopy(snapshot)
  }

  static func regionGetTakeResult(
    _ runtime: OpaquePointer,
    operationId: UInt64
  ) throws -> NativeOfflineRegionInfo? {
    var found = false
    let snapshot = try NativeMemory
      .withTemporary(OpaquePointer?.none) { snapshot in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_runtime_offline_region_get_take_result(
            runtime,
            operationId,
            snapshot,
            outFound
          ))
          found = outFound.pointee
        }
      }.value
    guard found, let snapshot else { return nil }
    defer { mln_offline_region_snapshot_destroy(snapshot) }
    return try offlineRegionSnapshotCopy(snapshot)
  }

  static func regionsListTakeResult(
    _ runtime: OpaquePointer,
    operationId: UInt64
  ) throws -> [NativeOfflineRegionInfo] {
    let list = try NativeMemory.withTemporary(OpaquePointer?.none) { list in
      try checkStatus(mln_runtime_offline_regions_list_take_result(
        runtime,
        operationId,
        list
      ))
    }.value
    guard let list else { throw NativeStatusFailure(
      rawStatus: 0,
      diagnostic: "offline region list result was null"
    ) }
    defer { mln_offline_region_list_destroy(list) }
    return try offlineRegionListCopy(list)
  }

  static func regionsMergeDatabaseTakeResult(
    _ runtime: OpaquePointer,
    operationId: UInt64
  ) throws -> [NativeOfflineRegionInfo] {
    let list = try NativeMemory.withTemporary(OpaquePointer?.none) { list in
      try checkStatus(mln_runtime_offline_regions_merge_database_take_result(
        runtime,
        operationId,
        list
      ))
    }.value
    guard let list else { throw NativeStatusFailure(
      rawStatus: 0,
      diagnostic: "offline merge result list was null"
    ) }
    defer { mln_offline_region_list_destroy(list) }
    return try offlineRegionListCopy(list)
  }

  static func regionUpdateMetadataTakeResult(
    _ runtime: OpaquePointer,
    operationId: UInt64
  ) throws -> NativeOfflineRegionInfo {
    let snapshot = try NativeMemory
      .withTemporary(OpaquePointer?.none) { snapshot in
        try checkStatus(mln_runtime_offline_region_update_metadata_take_result(
          runtime,
          operationId,
          snapshot
        ))
      }.value
    guard let snapshot else { throw NativeStatusFailure(
      rawStatus: 0,
      diagnostic: "offline update metadata result was null"
    ) }
    defer { mln_offline_region_snapshot_destroy(snapshot) }
    return try offlineRegionSnapshotCopy(snapshot)
  }

  static func regionGetStatusTakeResult(
    _ runtime: OpaquePointer,
    operationId: UInt64
  ) throws -> NativeOfflineRegionStatus {
    var status = mln_offline_region_status()
    status.size = UInt32(MemoryLayout<mln_offline_region_status>.size)
    try checkStatus(mln_runtime_offline_region_get_status_take_result(
      runtime,
      operationId,
      &status
    ))
    return NativeOfflineRegionStatus(status)
  }

  private static func offlineRegionSnapshotCopy(
    _ snapshot: OpaquePointer
  ) throws
    -> NativeOfflineRegionInfo
  {
    var info = mln_offline_region_info()
    info.size = UInt32(MemoryLayout<mln_offline_region_info>.size)
    try checkStatus(mln_offline_region_snapshot_get(snapshot, &info))
    return try NativeOfflineRegionInfo(copying: info)
  }

  private static func offlineRegionListCopy(_ list: OpaquePointer) throws
    -> [NativeOfflineRegionInfo]
  {
    let count = try NativeMemory.withTemporary(0) { count in
      try checkStatus(mln_offline_region_list_count(list, count))
    }.value
    return try (0 ..< count).map { index in
      var info = mln_offline_region_info()
      info.size = UInt32(MemoryLayout<mln_offline_region_info>.size)
      try checkStatus(mln_offline_region_list_get(list, index, &info))
      return try NativeOfflineRegionInfo(copying: info)
    }
  }
}
