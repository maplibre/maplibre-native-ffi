internal import CMaplibreNativeC
import Foundation

enum NativeStyle {
  static func createGeoJSONSourceData(
    data: mln_buffer_view,
    options: NativeGeoJSONSourceOptions
  ) throws -> NativeGeoJSONSourceDataHandle {
    try options.withNativeOptions { options in
      try NativeHandleFactory.create(
        nullDiagnostic: "mln_geojson_source_data_create returned a null handle"
      ) { outHandle in
        try checkStatus(mln_geojson_source_data_create(
          data,
          options,
          outHandle
        ))
      }
    }
  }

  static func copyMapData(
    _ map: NativeMapHandle,
    copy: (
      mln_map,
      UnsafeMutablePointer<UInt8>?,
      Int,
      UnsafeMutablePointer<Int>
    ) -> mln_status
  ) throws -> Data {
    let required = try NativeMemory.withTemporary(0) { size in
      try checkStatus(copy(map.raw, nil, 0, size))
    }.value
    guard required > 0 else { return Data() }
    var bytes = [UInt8](repeating: 0, count: required)
    let size = try bytes.withUnsafeMutableBufferPointer { buffer in
      try NativeMemory.withTemporary(0) { size in
        try checkStatus(copy(map.raw, buffer.baseAddress, required, size))
      }.value
    }
    guard size <= bytes.count else {
      throw NativeStatusFailure.swiftNativeError(
        "native data size exceeded caller buffer"
      )
    }
    return Data(bytes[0 ..< size])
  }

  static func mapReadStart(
    _ map: NativeMapHandle,
    start: (mln_map, UnsafeMutablePointer<mln_operation>) -> mln_status
  ) throws -> NativeOperationHandle {
    try self.start { try checkStatus(start(map.raw, $0)) }
  }

  static func mapDataTakeResult(
    _ operation: NativeOperationHandle,
    take: (mln_operation, UnsafeMutablePointer<mln_buffer>) -> mln_status
  ) throws -> Data {
    try bufferTakeResult(operation, take: take) ?? Data()
  }

  static func mapTextTakeResult(
    _ operation: NativeOperationHandle,
    take: (mln_operation, UnsafeMutablePointer<mln_buffer>) -> mln_status
  ) throws -> String {
    try bufferStringTakeResult(operation, take: take)
  }

  private static func start(
    _ body: (UnsafeMutablePointer<mln_operation>) throws -> Void
  ) throws -> NativeOperationHandle {
    let raw = try NativeMemory.withTemporary(mln_operation(0)) { operation in
      try body(operation)
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func sourceInfoStart(
    _ map: NativeMapHandle,
    sourceId: mln_buffer_view
  ) throws -> NativeOperationHandle {
    try start {
      try checkStatus(mln_map_get_style_source_info_start(
        map.raw,
        sourceId,
        $0
      ))
    }
  }

  static func sourceInfoTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> mln_style_source_info? {
    var info = mln_style_source_info()
    info.size = UInt32(MemoryLayout<mln_style_source_info>.size)
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_source_info_take_result(
        operation.raw,
        &info,
        found
      ))
    }.value
    return found ? info : nil
  }

  static func sourceAttributionStart(
    _ map: NativeMapHandle,
    sourceId: mln_buffer_view
  ) throws -> NativeOperationHandle {
    try start {
      try checkStatus(mln_map_copy_style_source_attribution_start(
        map.raw,
        sourceId,
        $0
      ))
    }
  }

  static func sourceAttributionTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> String? {
    var found = false
    let raw = try NativeMemory.withTemporary(mln_buffer(0)) { buffer in
      try NativeMemory.withTemporary(false) { outFound in
        try checkStatus(mln_map_copy_style_source_attribution_take_result(
          operation.raw,
          buffer,
          outFound
        ))
        found = outFound.pointee
      }
    }.value
    guard found else { return nil }
    return try copyBufferString(NativeBufferHandle(raw: raw))
  }

  static func sourceURLStart(
    _ map: NativeMapHandle,
    sourceId: mln_buffer_view
  ) throws -> NativeOperationHandle {
    try start {
      try checkStatus(mln_map_copy_style_source_url_start(
        map.raw,
        sourceId,
        $0
      ))
    }
  }

  static func sourceURLTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> String? {
    var found = false
    let raw = try NativeMemory.withTemporary(mln_buffer(0)) { buffer in
      try NativeMemory.withTemporary(false) { outFound in
        try checkStatus(mln_map_copy_style_source_url_take_result(
          operation.raw,
          buffer,
          outFound
        ))
        found = outFound.pointee
      }
    }.value
    guard found else { return nil }
    return try copyBufferString(NativeBufferHandle(raw: raw))
  }

  static func sourceTileURLsStart(
    _ map: NativeMapHandle,
    sourceId: mln_buffer_view
  ) throws -> NativeOperationHandle {
    try start {
      try checkStatus(mln_map_get_style_source_tile_urls_start(
        map.raw,
        sourceId,
        $0
      ))
    }
  }

  static func sourceTileURLsTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> [String]? {
    var found = false
    let raw = try NativeMemory.withTemporary(mln_style_string_list(0)) { list in
      try NativeMemory.withTemporary(false) { outFound in
        try checkStatus(mln_map_get_style_source_tile_urls_take_result(
          operation.raw,
          list,
          outFound
        ))
        found = outFound.pointee
      }
    }.value
    guard found else { return nil }
    let list = NativeStyleStringListHandle(raw: raw)
    defer { mln_style_string_list_destroy(list.raw) }
    let count = try NativeMemory.withTemporary(0) { count in
      try checkStatus(mln_style_string_list_count(list.raw, count))
    }.value
    return try (0 ..< count).map { index in
      let output = try NativeMemory.withTemporary(mln_buffer_view()) { value in
        try checkStatus(mln_style_string_list_get(list.raw, index, value))
      }.value
      return try NativeString.copyUTF8(data: output.data, size: output.size)
    }
  }

  static func sourceIdsStart(
    _ map: NativeMapHandle
  ) throws -> NativeOperationHandle {
    try start {
      try checkStatus(mln_map_list_style_source_ids_start(map.raw, $0))
    }
  }

  static func sourceIdsTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> [String] {
    let raw = try NativeMemory.withTemporary(mln_style_id_list(0)) { list in
      try checkStatus(mln_map_list_style_source_ids_take_result(
        operation.raw,
        list
      ))
    }.value
    let list = NativeStyleIdListHandle(raw: raw)
    defer { mln_style_id_list_destroy(list.raw) }
    return try copyStyleIdList(list)
  }

  private static func copyBufferString(
    _ buffer: NativeBufferHandle
  ) throws -> String {
    let data = try NativeMemory.copyBuffer(buffer)
    return try data.withUnsafeBytes {
      try NativeString.copyUTF8(data: $0.baseAddress, size: $0.count)
    }
  }

  static func imageOperationStart(
    _ map: NativeMapHandle,
    imageId: mln_buffer_view,
    start: (mln_map, mln_buffer_view, UnsafeMutablePointer<mln_operation>)
      -> mln_status
  ) throws -> NativeOperationHandle {
    try self.start { try checkStatus(start(map.raw, imageId, $0)) }
  }

  static func setTransitionOptions(
    _ map: NativeMapHandle,
    options: NativeStyleTransitionOptions
  ) throws -> UInt64 {
    try options.withNativeOptions { native in
      try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try checkStatus(mln_map_set_style_transition_options(
          map.raw,
          native,
          commandId
        ))
      }.value
    }
  }

  static func transitionOptionsStart(
    _ map: NativeMapHandle
  ) throws -> NativeOperationHandle {
    try start {
      try checkStatus(mln_map_get_style_transition_options_start(map.raw, $0))
    }
  }

  static func transitionOptionsTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> NativeStyleTransitionOptions {
    var options = mln_style_transition_options_default()
    try checkStatus(mln_map_get_style_transition_options_take_result(
      operation.raw,
      &options
    ))
    return NativeStyleTransitionOptions(options)
  }

  static func sourceInfo(
    fixed info: mln_style_source_info,
    attribution: String?,
    url: String?,
    tileURLs: [String]?
  ) -> NativeStyleSourceInfo {
    let has: (UInt32) -> Bool = { (info.fields & $0) != 0 }
    let tileJSON = has(MLN_STYLE_SOURCE_INFO_TILEJSON.rawValue)
      ? NativeStyleSourceTileJSON(
        tileURLs: tileURLs ?? [],
        minZoom: info.min_zoom,
        maxZoom: info.max_zoom,
        scheme: info.scheme,
        bounds: has(MLN_STYLE_SOURCE_INFO_BOUNDS.rawValue)
          ? NativeLatLngBounds(info.bounds) : nil
      ) : nil
    return NativeStyleSourceInfo(
      type: info.type,
      isVolatile: info.is_volatile,
      attribution: attribution,
      url: url,
      tileJSON: tileJSON,
      tileSize: has(MLN_STYLE_SOURCE_INFO_TILE_SIZE.rawValue)
        ? info.tile_size : nil,
      vectorEncoding: has(MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING.rawValue)
        ? info.vector_encoding : nil,
      rasterEncoding: has(MLN_STYLE_SOURCE_INFO_RASTER_ENCODING.rawValue)
        ? info.raster_encoding : nil
    )
  }

  static func imageInfoTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> NativeStyleImageInfo? {
    var info = mln_style_image_info_default()
    let found = try NativeMemory.withTemporary(false) {
      try checkStatus(mln_map_get_style_image_info_take_result(
        operation.raw,
        &info,
        $0
      ))
    }.value
    return found ? NativeStyleImageInfo(info) : nil
  }

  static func imagePixelsTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> [UInt8]? {
    guard let data = try optionalBufferTakeResult(
      operation,
      take: mln_map_copy_style_image_premultiplied_rgba8_take_result
    ) else { return nil }
    return Array(data)
  }

  static func imageSourceCommand(
    _ coordinates: [NativeLatLng],
    _ body: (UnsafePointer<mln_lat_lng>?, Int, UnsafeMutablePointer<UInt64>)
      throws -> Void
  ) throws -> UInt64 {
    try validateImageSourceCoordinates(coordinates)
    let rawCoordinates = coordinates.map(\.native)
    return try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try NativeMemory.withTemporary(UInt64(0)) { commandId in
        try body(coordinates.baseAddress, coordinates.count, commandId)
      }.value
    }
  }

  static func imageSourceCoordinatesTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> [NativeLatLng]? {
    var coordinates = [mln_lat_lng](repeating: mln_lat_lng(), count: 4)
    var found = false
    let count = try coordinates.withUnsafeMutableBufferPointer { coordinates in
      try NativeMemory.withTemporary(0) { count in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_map_get_image_source_coordinates_take_result(
            operation.raw,
            coordinates.baseAddress,
            coordinates.count,
            count,
            outFound
          ))
          found = outFound.pointee
        }
      }.value
    }
    guard found else { return nil }
    guard count == coordinates.count else {
      throw NativeStatusFailure.swiftNativeError(
        "native image source coordinate count did not match Swift image source invariant"
      )
    }
    return coordinates.map(NativeLatLng.init)
  }

  private static func validateImageSourceCoordinates(
    _ coordinates: [NativeLatLng]
  ) throws {
    guard coordinates.count == 4 else {
      throw NativeStatusFailure
        .swiftInvalidArgument(
          "image source coordinates must contain exactly 4 coordinates"
        )
    }
  }

  static func layerInfoStart(
    _ map: NativeMapHandle,
    layerId: mln_buffer_view
  ) throws -> NativeOperationHandle {
    try start {
      try checkStatus(mln_map_get_style_layer_info_start(
        map.raw,
        layerId,
        $0
      ))
    }
  }

  static func layerInfoTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> mln_style_layer_info? {
    var info = mln_style_layer_info()
    info.size = UInt32(MemoryLayout<mln_style_layer_info>.size)
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_layer_info_take_result(
        operation.raw,
        &info,
        found
      ))
    }.value
    return found ? info : nil
  }

  static func layerIdsStart(_ map: NativeMapHandle) throws
    -> NativeOperationHandle
  {
    try start {
      try checkStatus(mln_map_list_style_layer_ids_start(map.raw, $0))
    }
  }

  static func layerIdsTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> [String] {
    let raw = try NativeMemory.withTemporary(mln_style_id_list(0)) { list in
      try checkStatus(mln_map_list_style_layer_ids_take_result(
        operation.raw,
        list
      ))
    }.value
    let list = NativeStyleIdListHandle(raw: raw)
    defer { mln_style_id_list_destroy(list.raw) }
    return try copyStyleIdList(list)
  }

  static func layerJSONStart(
    _ map: NativeMapHandle,
    layerId: mln_buffer_view
  ) throws -> NativeOperationHandle {
    try start {
      try checkStatus(mln_map_get_style_layer_json_start(map.raw, layerId, $0))
    }
  }

  static func lightPropertyStart(
    _ map: NativeMapHandle,
    propertyName: mln_buffer_view
  ) throws -> NativeOperationHandle {
    try start {
      try checkStatus(mln_map_get_style_light_property_start(
        map.raw,
        propertyName,
        $0
      ))
    }
  }

  static func layerPropertyStart(
    _ map: NativeMapHandle,
    layerId: mln_buffer_view,
    propertyName: mln_buffer_view
  ) throws -> NativeOperationHandle {
    try start {
      try checkStatus(mln_map_get_layer_property_start(
        map.raw,
        layerId,
        propertyName,
        $0
      ))
    }
  }

  static func layerFilterStart(
    _ map: NativeMapHandle,
    layerId: mln_buffer_view
  ) throws -> NativeOperationHandle {
    try start {
      try checkStatus(mln_map_get_layer_filter_start(map.raw, layerId, $0))
    }
  }

  static func bufferTakeResult(
    _ operation: NativeOperationHandle,
    take: (mln_operation, UnsafeMutablePointer<mln_buffer>) -> mln_status
  ) throws -> Data? {
    let raw = try NativeMemory.withTemporary(mln_buffer(0)) { buffer in
      try checkStatus(take(operation.raw, buffer))
    }.value
    let buffer = NativeBufferHandle(raw: raw)
    return buffer.isNull ? nil : try NativeMemory.copyBuffer(buffer)
  }

  static func optionalBufferTakeResult(
    _ operation: NativeOperationHandle,
    take: (
      mln_operation,
      UnsafeMutablePointer<mln_buffer>,
      UnsafeMutablePointer<Bool>
    ) -> mln_status
  ) throws -> Data? {
    var found = false
    let raw = try NativeMemory.withTemporary(mln_buffer(0)) { buffer in
      try NativeMemory.withTemporary(false) { outFound in
        try checkStatus(take(operation.raw, buffer, outFound))
        found = outFound.pointee
      }
    }.value
    guard found else { return nil }
    return try NativeMemory.copyBuffer(NativeBufferHandle(raw: raw))
  }

  static func layerTextStart(
    _ map: NativeMapHandle,
    layerId: mln_buffer_view,
    start: (mln_map, mln_buffer_view, UnsafeMutablePointer<mln_operation>)
      -> mln_status
  ) throws -> NativeOperationHandle {
    try self.start { try checkStatus(start(map.raw, layerId, $0)) }
  }

  static func bufferStringTakeResult(
    _ operation: NativeOperationHandle,
    take: (mln_operation, UnsafeMutablePointer<mln_buffer>) -> mln_status
  ) throws -> String {
    let raw = try NativeMemory.withTemporary(mln_buffer(0)) { buffer in
      try checkStatus(take(operation.raw, buffer))
    }.value
    return try copyBufferString(NativeBufferHandle(raw: raw))
  }

  /// Probes the required interval counts, then copies. Null arrays with zero
  /// capacity are a size probe the C API answers with OK.
  static func imageStretchesTakeResult(
    _ operation: NativeOperationHandle
  ) throws -> ([ImageStretch], [ImageStretch])? {
    var xCount = 0
    var yCount = 0
    var found = false
    try checkStatus(mln_map_copy_style_image_stretches_take_result(
      operation.raw, nil, 0, &xCount, nil, 0, &yCount, &found
    ))
    guard found else { return nil }
    var rawX = [mln_image_stretch](
      repeating: mln_image_stretch(from: 0, to: 0), count: xCount
    )
    var rawY = [mln_image_stretch](
      repeating: mln_image_stretch(from: 0, to: 0), count: yCount
    )
    try rawX.withUnsafeMutableBufferPointer { x in
      try rawY.withUnsafeMutableBufferPointer { y in
        try checkStatus(mln_map_copy_style_image_stretches_take_result(
          operation.raw,
          x.baseAddress,
          x.count,
          &xCount,
          y.baseAddress,
          y.count,
          &yCount,
          &found
        ))
      }
    }
    return (
      rawX.map { ImageStretch(from: $0.from, to: $0.to) },
      rawY.map { ImageStretch(from: $0.from, to: $0.to) }
    )
  }

  /// Probes the required byte length, then copies. A null buffer with zero
  /// capacity is a size probe the C API answers with OK.
  static func copyMapText(
    _ map: NativeMapHandle,
    copy: (
      mln_map,
      UnsafeMutablePointer<CChar>?,
      Int,
      UnsafeMutablePointer<Int>
    ) -> mln_status
  ) throws -> String {
    let required = try NativeMemory.withTemporary(0) { outSize in
      try checkStatus(copy(map.raw, nil, 0, outSize))
    }.value
    guard required > 0 else { return "" }

    var bytes = [CChar](repeating: 0, count: required)
    let size = try bytes.withUnsafeMutableBufferPointer { buffer in
      try NativeMemory.withTemporary(0) { outSize in
        try checkStatus(copy(map.raw, buffer.baseAddress, required, outSize))
      }.value
    }
    guard size <= required else {
      throw NativeStatusFailure(
        rawStatus: MLN_STATUS_NATIVE_ERROR.rawValue,
        diagnostic: "native text size exceeded caller buffer"
      )
    }
    return try bytes.withUnsafeBufferPointer { buffer in
      try NativeString.copyUTF8(data: buffer.baseAddress, size: size)
    }
  }

  static func copyLayerText(
    _ map: NativeMapHandle,
    layerId: mln_buffer_view,
    copy: (
      mln_map,
      mln_buffer_view,
      UnsafeMutablePointer<CChar>?,
      Int,
      UnsafeMutablePointer<Int>
    ) -> mln_status
  ) throws -> String {
    let required = try NativeMemory.withTemporary(0) { outSize in
      try checkStatus(copy(map.raw, layerId, nil, 0, outSize))
    }.value
    guard required > 0 else { return "" }

    var bytes = [CChar](repeating: 0, count: required)
    let size = try bytes.withUnsafeMutableBufferPointer { buffer in
      try NativeMemory.withTemporary(0) { outSize in
        try checkStatus(copy(
          map.raw,
          layerId,
          buffer.baseAddress,
          required,
          outSize
        ))
      }.value
    }
    guard size <= required else {
      throw NativeStatusFailure(
        rawStatus: MLN_STATUS_NATIVE_ERROR.rawValue,
        diagnostic: "native layer text size exceeded caller buffer"
      )
    }
    return try bytes.withUnsafeBufferPointer { buffer in
      try NativeString.copyUTF8(data: buffer.baseAddress, size: size)
    }
  }

  private static func copyStyleIdList(_ list: NativeStyleIdListHandle) throws
    -> [String]
  {
    let count = try NativeMemory.withTemporary(0) { count in
      try checkStatus(mln_style_id_list_count(list.raw, count))
    }.value
    return try (0 ..< count).map { index in
      let output = try NativeMemory.withTemporary(mln_buffer_view()) { value in
        try checkStatus(mln_style_id_list_get(list.raw, index, value))
      }
      return try NativeString.copyUTF8(
        data: output.value.data,
        size: output.value.size
      )
    }
  }
}
