internal import CMaplibreNativeC

enum NativeStyle {
  static func removeSource(_ map: OpaquePointer,
                           sourceId: mln_string_view) throws -> Bool
  {
    try NativeMemory.withTemporary(false) { removed in
      try checkStatus(mln_map_remove_style_source(map, sourceId, removed))
    }.value
  }

  static func sourceExists(_ map: OpaquePointer,
                           sourceId: mln_string_view) throws -> Bool
  {
    try NativeMemory.withTemporary(false) { exists in
      try checkStatus(mln_map_style_source_exists(map, sourceId, exists))
    }.value
  }

  static func sourceType(_ map: OpaquePointer,
                         sourceId: mln_string_view) throws -> UInt32?
  {
    var type = UInt32(0)
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_source_type(
        map,
        sourceId,
        &type,
        found
      ))
    }.value
    return found ? type : nil
  }

  static func sourceInfo(_ map: OpaquePointer,
                         sourceId: mln_string_view) throws
    -> NativeStyleSourceInfo?
  {
    var info = mln_style_source_info()
    info.size = UInt32(MemoryLayout<mln_style_source_info>.size)
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_source_info(
        map,
        sourceId,
        &info,
        found
      ))
    }.value
    return found ? NativeStyleSourceInfo(info) : nil
  }

  static func copySourceAttribution(
    _ map: OpaquePointer,
    sourceId: mln_string_view,
    capacity: Int
  ) throws -> (String?, Int) {
    var bytes = [UInt8](repeating: 0, count: capacity)
    var found = false
    let size = try bytes.withUnsafeMutableBufferPointer { buffer in
      try NativeMemory.withTemporary(0) { outSize in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_map_copy_style_source_attribution(
            map,
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
        data: buffer.baseAddress
          .map { UnsafeRawPointer($0).assumingMemoryBound(to: CChar.self) },
        size: size
      )
    }
    return (attribution, size)
  }

  static func sourceIds(_ map: OpaquePointer) throws -> [String] {
    let list = try NativeMemory.withTemporary(OpaquePointer?.none) { list in
      try checkStatus(mln_map_list_style_source_ids(map, list))
    }.value
    guard let list
    else {
      throw NativeStatusFailure.swiftNativeError("source ID list was null")
    }
    defer { mln_style_id_list_destroy(list) }
    return try copyStyleIdList(list)
  }

  static func removeImage(_ map: OpaquePointer,
                          imageId: mln_string_view) throws -> Bool
  {
    try NativeMemory.withTemporary(false) { removed in
      try checkStatus(mln_map_remove_style_image(map, imageId, removed))
    }.value
  }

  static func imageExists(_ map: OpaquePointer,
                          imageId: mln_string_view) throws -> Bool
  {
    try NativeMemory.withTemporary(false) { exists in
      try checkStatus(mln_map_style_image_exists(map, imageId, exists))
    }.value
  }

  static func imageInfo(_ map: OpaquePointer,
                        imageId: mln_string_view) throws
    -> NativeStyleImageInfo?
  {
    var info = mln_style_image_info_default()
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_image_info(map, imageId, &info, found))
    }.value
    return found ? NativeStyleImageInfo(info) : nil
  }

  static func copyImagePremultipliedRGBA8(
    _ map: OpaquePointer,
    imageId: mln_string_view,
    capacity: Int
  ) throws -> ([UInt8]?, Int) {
    var bytes = [UInt8](repeating: 0, count: capacity)
    var found = false
    let size = try bytes.withUnsafeMutableBufferPointer { buffer in
      try NativeMemory.withTemporary(0) { outSize in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_map_copy_style_image_premultiplied_rgba8(
            map,
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
    _ map: OpaquePointer,
    sourceId: mln_string_view,
    coordinates: [NativeLatLng],
    url: mln_string_view
  ) throws {
    try validateImageSourceCoordinates(coordinates)
    let rawCoordinates = coordinates.map(\.native)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try checkStatus(mln_map_add_image_source_url(
        map,
        sourceId,
        coordinates.baseAddress,
        coordinates.count,
        url
      ))
    }
  }

  static func addImageSourceImage(
    _ map: OpaquePointer,
    sourceId: mln_string_view,
    coordinates: [NativeLatLng],
    image: UnsafePointer<mln_premultiplied_rgba8_image>
  ) throws {
    try validateImageSourceCoordinates(coordinates)
    let rawCoordinates = coordinates.map(\.native)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try checkStatus(mln_map_add_image_source_image(
        map,
        sourceId,
        coordinates.baseAddress,
        coordinates.count,
        image
      ))
    }
  }

  static func setImageSourceCoordinates(
    _ map: OpaquePointer,
    sourceId: mln_string_view,
    coordinates: [NativeLatLng]
  ) throws {
    try validateImageSourceCoordinates(coordinates)
    let rawCoordinates = coordinates.map(\.native)
    try rawCoordinates.withUnsafeBufferPointer { coordinates in
      try checkStatus(mln_map_set_image_source_coordinates(
        map,
        sourceId,
        coordinates.baseAddress,
        coordinates.count
      ))
    }
  }

  static func imageSourceCoordinates(
    _ map: OpaquePointer,
    sourceId: mln_string_view
  ) throws -> [NativeLatLng]? {
    var coordinates = [mln_lat_lng](repeating: mln_lat_lng(), count: 4)
    var found = false
    let count = try coordinates.withUnsafeMutableBufferPointer { coordinates in
      try NativeMemory.withTemporary(0) { count in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_map_get_image_source_coordinates(
            map,
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

  static func removeLayer(_ map: OpaquePointer,
                          layerId: mln_string_view) throws -> Bool
  {
    try NativeMemory.withTemporary(false) { removed in
      try checkStatus(mln_map_remove_style_layer(map, layerId, removed))
    }.value
  }

  static func layerExists(_ map: OpaquePointer,
                          layerId: mln_string_view) throws -> Bool
  {
    try NativeMemory.withTemporary(false) { exists in
      try checkStatus(mln_map_style_layer_exists(map, layerId, exists))
    }.value
  }

  static func layerType(_ map: OpaquePointer,
                        layerId: mln_string_view) throws -> String?
  {
    var layerType = mln_string_view()
    let found = try NativeMemory.withTemporary(false) { found in
      try checkStatus(mln_map_get_style_layer_type(
        map,
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

  static func layerIds(_ map: OpaquePointer) throws -> [String] {
    let list = try NativeMemory.withTemporary(OpaquePointer?.none) { list in
      try checkStatus(mln_map_list_style_layer_ids(map, list))
    }.value
    guard let list
    else {
      throw NativeStatusFailure.swiftNativeError("layer ID list was null")
    }
    defer { mln_style_id_list_destroy(list) }
    return try copyStyleIdList(list)
  }

  static func layerJSON(_ map: OpaquePointer,
                        layerId: mln_string_view) throws -> NativeJSONValue?
  {
    let output = try NativeMemory
      .withTemporary(OpaquePointer?.none) { snapshot in
        try NativeMemory.withTemporary(false) { found in
          try checkStatus(mln_map_get_style_layer_json(
            map,
            layerId,
            snapshot,
            found
          ))
          if !found.pointee { snapshot.pointee = nil }
        }
      }.value
    guard let snapshot = output else { return nil }
    defer { mln_json_snapshot_destroy(snapshot) }
    return try NativeJSONSnapshot.copyValue(snapshot)
  }

  static func lightProperty(
    _ map: OpaquePointer,
    propertyName: mln_string_view
  ) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory
      .withTemporary(OpaquePointer?.none) { snapshot in
        try checkStatus(mln_map_get_style_light_property(
          map,
          propertyName,
          snapshot
        ))
      }.value
    guard let snapshot else { return nil }
    defer { mln_json_snapshot_destroy(snapshot) }
    return try NativeJSONSnapshot.copyValue(snapshot)
  }

  static func layerProperty(
    _ map: OpaquePointer,
    layerId: mln_string_view,
    propertyName: mln_string_view
  ) throws -> NativeJSONValue? {
    let snapshot = try NativeMemory
      .withTemporary(OpaquePointer?.none) { snapshot in
        try checkStatus(mln_map_get_layer_property(
          map,
          layerId,
          propertyName,
          snapshot
        ))
      }.value
    guard let snapshot else { return nil }
    defer { mln_json_snapshot_destroy(snapshot) }
    return try NativeJSONSnapshot.copyValue(snapshot)
  }

  static func layerFilter(_ map: OpaquePointer,
                          layerId: mln_string_view) throws -> NativeJSONValue?
  {
    let snapshot = try NativeMemory
      .withTemporary(OpaquePointer?.none) { snapshot in
        try checkStatus(mln_map_get_layer_filter(map, layerId, snapshot))
      }.value
    guard let snapshot else { return nil }
    defer { mln_json_snapshot_destroy(snapshot) }
    return try NativeJSONSnapshot.copyValue(snapshot)
  }

  private static func copyStyleIdList(_ list: OpaquePointer) throws
    -> [String]
  {
    let count = try NativeMemory.withTemporary(0) { count in
      try checkStatus(mln_style_id_list_count(list, count))
    }.value
    return try (0 ..< count).map { index in
      let output = try NativeMemory.withTemporary(mln_string_view()) { value in
        try checkStatus(mln_style_id_list_get(list, index, value))
      }
      return try NativeString.copyUTF8(
        data: output.value.data,
        size: output.value.size
      )
    }
  }
}
