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

  static func removeSource(_ map: NativeMapHandle,
                           sourceId: mln_buffer_view) throws -> Bool
  {
    try NativeMemory.withTemporary(false) { removed in
      try checkStatus(mln_map_remove_style_source(map.raw, sourceId, removed))
    }.value
  }

  static func sourceExists(_ map: NativeMapHandle,
                           sourceId: mln_buffer_view) throws -> Bool
  {
    try NativeMemory.withTemporary(false) { exists in
      try checkStatus(mln_map_style_source_exists(map.raw, sourceId, exists))
    }.value
  }

  static func sourceType(_ map: NativeMapHandle,
                         sourceId: mln_buffer_view) throws -> UInt32?
  {
    var type = UInt32(0)
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_source_type(
        map.raw,
        sourceId,
        &type,
        found
      ))
    }.value
    return found ? type : nil
  }

  static func sourceInfo(_ map: NativeMapHandle,
                         sourceId: mln_buffer_view) throws
    -> NativeStyleSourceInfo?
  {
    var info = mln_style_source_info()
    info.size = UInt32(MemoryLayout<mln_style_source_info>.size)
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_source_info(
        map.raw,
        sourceId,
        &info,
        found
      ))
    }.value
    guard found else { return nil }

    let attribution = info.has_attribution
      ? try copySourceAttribution(
        map,
        sourceId: sourceId,
        capacity: info.attribution_size
      ).0
      : nil
    let url = hasSourceInfoField(info, MLN_STYLE_SOURCE_INFO_URL.rawValue)
      ? try copySourceURL(map, sourceId: sourceId, capacity: info.url_size).0
      : nil
    let tileJSON: NativeStyleSourceTileJSON?
    if hasSourceInfoField(info, MLN_STYLE_SOURCE_INFO_TILEJSON.rawValue) {
      let tileURLs = try sourceTileURLs(map, sourceId: sourceId) ?? []
      tileJSON = NativeStyleSourceTileJSON(
        tileURLs: tileURLs,
        minZoom: info.min_zoom,
        maxZoom: info.max_zoom,
        scheme: info.scheme,
        bounds: hasSourceInfoField(info, MLN_STYLE_SOURCE_INFO_BOUNDS.rawValue)
          ? NativeLatLngBounds(info.bounds)
          : nil
      )
    } else {
      tileJSON = nil
    }

    return NativeStyleSourceInfo(
      type: info.type,
      isVolatile: info.is_volatile,
      attribution: attribution,
      url: url,
      tileJSON: tileJSON,
      tileSize: hasSourceInfoField(
        info,
        MLN_STYLE_SOURCE_INFO_TILE_SIZE.rawValue
      ) ? info.tile_size : nil,
      vectorEncoding: hasSourceInfoField(
        info,
        MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING.rawValue
      ) ? info.vector_encoding : nil,
      rasterEncoding: hasSourceInfoField(
        info,
        MLN_STYLE_SOURCE_INFO_RASTER_ENCODING.rawValue
      ) ? info.raster_encoding : nil
    )
  }

  static func sourceAttribution(
    _ map: NativeMapHandle,
    sourceId: mln_buffer_view
  ) throws -> String? {
    var info = mln_style_source_info()
    info.size = UInt32(MemoryLayout<mln_style_source_info>.size)
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_source_info(
        map.raw,
        sourceId,
        &info,
        found
      ))
    }.value
    guard found, info.has_attribution else { return nil }
    return try copySourceAttribution(
      map,
      sourceId: sourceId,
      capacity: info.attribution_size
    ).0
  }

  private static func hasSourceInfoField(
    _ info: mln_style_source_info,
    _ field: UInt32
  ) -> Bool {
    (info.fields & field) != 0
  }

  static func copySourceAttribution(
    _ map: NativeMapHandle,
    sourceId: mln_buffer_view,
    capacity: Int
  ) throws -> (String?, Int) {
    var bytes = [UInt8](repeating: 0, count: capacity)
    var found = false
    let size = try bytes.withUnsafeMutableBufferPointer { buffer in
      try NativeMemory.withTemporary(0) { outSize in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_map_copy_style_source_attribution(
            map.raw,
            sourceId,
            buffer.baseAddress,
            capacity,
            outSize,
            outFound
          ))
          found = outFound.pointee
        }
      }.value
    }
    guard found else { return (nil, size) }
    guard size <= capacity else {
      throw NativeStatusFailure(
        rawStatus: MLN_STATUS_NATIVE_ERROR.rawValue,
        diagnostic: "native style source attribution size exceeded caller buffer"
      )
    }
    let attribution = try bytes.withUnsafeBufferPointer { buffer in
      try NativeString.copyUTF8(
        data: buffer.baseAddress.map(UnsafeRawPointer.init),
        size: size
      )
    }
    return (attribution, size)
  }

  static func copySourceURL(
    _ map: NativeMapHandle,
    sourceId: mln_buffer_view,
    capacity: Int
  ) throws -> (String?, Int) {
    var bytes = [UInt8](repeating: 0, count: capacity)
    var found = false
    let size = try bytes.withUnsafeMutableBufferPointer { buffer in
      try NativeMemory.withTemporary(0) { outSize in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_map_copy_style_source_url(
            map.raw,
            sourceId,
            buffer.baseAddress,
            capacity,
            outSize,
            outFound
          ))
          found = outFound.pointee
        }
      }.value
    }
    guard found else { return (nil, size) }
    guard size <= capacity else {
      throw NativeStatusFailure(
        rawStatus: MLN_STATUS_NATIVE_ERROR.rawValue,
        diagnostic: "native style source URL size exceeded caller buffer"
      )
    }
    let url = try bytes.withUnsafeBufferPointer { buffer in
      try NativeString.copyUTF8(
        data: buffer.baseAddress.map(UnsafeRawPointer.init),
        size: size
      )
    }
    return (url, size)
  }

  static func sourceTileURLs(
    _ map: NativeMapHandle,
    sourceId: mln_buffer_view
  ) throws -> [String]? {
    var found = false
    let listValue = try NativeMemory.withTemporary(UInt64(0)) { outHandle in
      try NativeMemory.withTemporary(false) { outFound in
        try checkStatus(mln_map_get_style_source_tile_urls(
          map.raw,
          sourceId,
          outHandle,
          outFound
        ))
        found = outFound.pointee
      }
    }.value
    guard found else { return nil }
    let list = NativeStyleStringListHandle(raw: listValue)
    guard !list.isNull else {
      throw NativeStatusFailure.swiftNativeError("tile URL list was null")
    }
    defer { mln_style_string_list_destroy(list.raw) }
    let count = try NativeMemory.withTemporary(0) { count in
      try checkStatus(mln_style_string_list_count(list.raw, count))
    }.value
    return try (0 ..< count).map { index in
      let output = try NativeMemory.withTemporary(mln_buffer_view()) { value in
        try checkStatus(mln_style_string_list_get(list.raw, index, value))
      }
      return try NativeString.copyUTF8(
        data: output.value.data,
        size: output.value.size
      )
    }
  }

  static func sourceIds(_ map: NativeMapHandle) throws -> [String] {
    let listValue = try NativeMemory.withTemporary(UInt64(0)) { outHandle in
      try checkStatus(mln_map_list_style_source_ids(map.raw, outHandle))
    }.value
    let list = NativeStyleIdListHandle(raw: listValue)
    guard !list.isNull
    else {
      throw NativeStatusFailure.swiftNativeError("source ID list was null")
    }
    defer { mln_style_id_list_destroy(list.raw) }
    return try copyStyleIdList(list)
  }

  static func removeImage(_ map: NativeMapHandle,
                          imageId: mln_buffer_view) throws -> Bool
  {
    try NativeMemory.withTemporary(false) { removed in
      try checkStatus(mln_map_remove_style_image(map.raw, imageId, removed))
    }.value
  }

  static func setTransitionOptions(
    _ map: NativeMapHandle,
    options: NativeStyleTransitionOptions
  ) throws {
    try options.withNativeOptions { native in
      try checkStatus(mln_map_set_style_transition_options(map.raw, native))
    }
  }

  static func transitionOptions(_ map: NativeMapHandle) throws
    -> NativeStyleTransitionOptions
  {
    var options = mln_style_transition_options_default()
    try checkStatus(mln_map_get_style_transition_options(map.raw, &options))
    return NativeStyleTransitionOptions(options)
  }

  static func imageExists(_ map: NativeMapHandle,
                          imageId: mln_buffer_view) throws -> Bool
  {
    try NativeMemory.withTemporary(false) { exists in
      try checkStatus(mln_map_style_image_exists(map.raw, imageId, exists))
    }.value
  }

  static func imageInfo(_ map: NativeMapHandle,
                        imageId: mln_buffer_view) throws
    -> NativeStyleImageInfo?
  {
    var info = mln_style_image_info_default()
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_image_info(
        map.raw,
        imageId,
        &info,
        found
      ))
    }.value
    return found ? NativeStyleImageInfo(info) : nil
  }

  static func copyImagePremultipliedRGBA8(
    _ map: NativeMapHandle,
    imageId: mln_buffer_view,
    capacity: Int
  ) throws -> ([UInt8]?, Int) {
    var bytes = [UInt8](repeating: 0, count: capacity)
    var found = false
    let size = try bytes.withUnsafeMutableBufferPointer { buffer in
      try NativeMemory.withTemporary(0) { outSize in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_map_copy_style_image_premultiplied_rgba8(
            map.raw,
            imageId,
            buffer.baseAddress,
            capacity,
            outSize,
            outFound
          ))
          found = outFound.pointee
        }
      }.value
    }
    guard found else { return (nil, size) }
    guard size <= capacity else {
      throw NativeStatusFailure(
        rawStatus: MLN_STATUS_NATIVE_ERROR.rawValue,
        diagnostic: "native style image byte size exceeded caller buffer"
      )
    }
    return (Array(bytes.prefix(size)), size)
  }

  static func addImageSourceURL(
    _ map: NativeMapHandle,
    sourceId: mln_buffer_view,
    coordinates: [NativeLatLng],
    url: mln_buffer_view
  ) throws {
    try validateImageSourceCoordinates(coordinates)
    let rawCoordinates = coordinates.map(\.native)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try checkStatus(mln_map_add_image_source_url(
        map.raw,
        sourceId,
        coordinates.baseAddress,
        coordinates.count,
        url
      ))
    }
  }

  static func addImageSourceImage(
    _ map: NativeMapHandle,
    sourceId: mln_buffer_view,
    coordinates: [NativeLatLng],
    image: UnsafePointer<mln_premultiplied_rgba8_image>
  ) throws {
    try validateImageSourceCoordinates(coordinates)
    let rawCoordinates = coordinates.map(\.native)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try checkStatus(mln_map_add_image_source_image(
        map.raw,
        sourceId,
        coordinates.baseAddress,
        coordinates.count,
        image
      ))
    }
  }

  static func setImageSourceCoordinates(
    _ map: NativeMapHandle,
    sourceId: mln_buffer_view,
    coordinates: [NativeLatLng]
  ) throws {
    try validateImageSourceCoordinates(coordinates)
    let rawCoordinates = coordinates.map(\.native)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try checkStatus(mln_map_set_image_source_coordinates(
        map.raw,
        sourceId,
        coordinates.baseAddress,
        coordinates.count
      ))
    }
  }

  static func imageSourceCoordinates(
    _ map: NativeMapHandle,
    sourceId: mln_buffer_view
  ) throws -> [NativeLatLng]? {
    var coordinates = [mln_lat_lng](repeating: mln_lat_lng(), count: 4)
    var found = false
    let count = try coordinates.withUnsafeMutableBufferPointer { coordinates in
      try NativeMemory.withTemporary(0) { count in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_map_get_image_source_coordinates(
            map.raw,
            sourceId,
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
      throw NativeStatusFailure(
        rawStatus: MLN_STATUS_NATIVE_ERROR.rawValue,
        diagnostic: "native image source coordinate count did not match Swift image source invariant"
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

  static func removeLayer(_ map: NativeMapHandle,
                          layerId: mln_buffer_view) throws -> Bool
  {
    try NativeMemory.withTemporary(false) { removed in
      try checkStatus(mln_map_remove_style_layer(map.raw, layerId, removed))
    }.value
  }

  static func layerExists(_ map: NativeMapHandle,
                          layerId: mln_buffer_view) throws -> Bool
  {
    try NativeMemory.withTemporary(false) { exists in
      try checkStatus(mln_map_style_layer_exists(map.raw, layerId, exists))
    }.value
  }

  static func layerType(_ map: NativeMapHandle,
                        layerId: mln_buffer_view) throws -> String?
  {
    var layerType = mln_buffer_view()
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_layer_type(
        map.raw,
        layerId,
        &layerType,
        found
      ))
    }.value
    return found ? try NativeString.copyUTF8(
      data: layerType.data,
      size: layerType.size
    ) : nil
  }

  static func layerIds(_ map: NativeMapHandle) throws -> [String] {
    let listValue = try NativeMemory.withTemporary(UInt64(0)) { outHandle in
      try checkStatus(mln_map_list_style_layer_ids(map.raw, outHandle))
    }.value
    let list = NativeStyleIdListHandle(raw: listValue)
    guard !list.isNull
    else {
      throw NativeStatusFailure.swiftNativeError("layer ID list was null")
    }
    defer { mln_style_id_list_destroy(list.raw) }
    return try copyStyleIdList(list)
  }

  static func layerJSON(_ map: NativeMapHandle,
                        layerId: mln_buffer_view) throws -> Data?
  {
    let output = try NativeMemory
      .withTemporary(UInt64(0)) { outHandle in
        try NativeMemory.withTemporary(false) { found in
          try checkStatus(mln_map_get_style_layer_json(
            map.raw,
            layerId,
            outHandle,
            found
          ))
          if !found.pointee { outHandle.pointee = 0 }
        }
      }.value
    let buffer = NativeBufferHandle(raw: output)
    return buffer.isNull ? nil : try NativeMemory.copyBuffer(buffer)
  }

  static func lightProperty(
    _ map: NativeMapHandle,
    propertyName: mln_buffer_view
  ) throws -> Data? {
    let snapshotValue = try NativeMemory
      .withTemporary(UInt64(0)) { outHandle in
        try checkStatus(mln_map_get_style_light_property(
          map.raw,
          propertyName,
          outHandle
        ))
      }.value
    let buffer = NativeBufferHandle(raw: snapshotValue)
    return buffer.isNull ? nil : try NativeMemory.copyBuffer(buffer)
  }

  static func layerProperty(
    _ map: NativeMapHandle,
    layerId: mln_buffer_view,
    propertyName: mln_buffer_view
  ) throws -> Data? {
    let snapshotValue = try NativeMemory
      .withTemporary(UInt64(0)) { outHandle in
        try checkStatus(mln_map_get_layer_property(
          map.raw,
          layerId,
          propertyName,
          outHandle
        ))
      }.value
    let buffer = NativeBufferHandle(raw: snapshotValue)
    return buffer.isNull ? nil : try NativeMemory.copyBuffer(buffer)
  }

  static func layerFilter(_ map: NativeMapHandle,
                          layerId: mln_buffer_view) throws -> Data?
  {
    let snapshotValue = try NativeMemory
      .withTemporary(UInt64(0)) { outHandle in
        try checkStatus(mln_map_get_layer_filter(map.raw, layerId, outHandle))
      }.value
    let buffer = NativeBufferHandle(raw: snapshotValue)
    return buffer.isNull ? nil : try NativeMemory.copyBuffer(buffer)
  }

  /// Probes the required interval counts, then copies. Null arrays with zero
  /// capacity are a size probe the C API answers with OK.
  static func copyImageStretches(
    _ map: NativeMapHandle,
    imageId: mln_buffer_view
  ) throws -> ([ImageStretch], [ImageStretch])? {
    var xCount = 0
    var yCount = 0
    var found = false
    try checkStatus(mln_map_copy_style_image_stretches(
      map.raw, imageId, nil, 0, &xCount, nil, 0, &yCount, &found
    ))
    guard found else { return nil }

    var rawX = [mln_image_stretch](
      repeating: mln_image_stretch(from: 0, to: 0), count: xCount
    )
    var rawY = [mln_image_stretch](
      repeating: mln_image_stretch(from: 0, to: 0), count: yCount
    )
    try rawX.withUnsafeMutableBufferPointer { bufferX in
      try rawY.withUnsafeMutableBufferPointer { bufferY in
        try checkStatus(mln_map_copy_style_image_stretches(
          map.raw,
          imageId,
          bufferX.baseAddress,
          bufferX.count,
          &xCount,
          bufferY.baseAddress,
          bufferY.count,
          &yCount,
          &found
        ))
      }
    }
    let toPublic = { (raw: [mln_image_stretch]) -> [ImageStretch] in
      raw.map { ImageStretch(from: $0.from, to: $0.to) }
    }
    return (toPublic(rawX), toPublic(rawY))
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
