internal import CMaplibreNativeC

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

  static func sourceInfo(
    fixed info: mln_style_source_info,
    attribution: String?,
    url: String?,
    tileURLs: [String]
  ) -> NativeStyleSourceInfo {
    let has: (UInt32) -> Bool = { (info.fields & $0) != 0 }
    let tileJSON = has(MLN_STYLE_SOURCE_INFO_TILEJSON.rawValue)
      ? NativeStyleSourceTileJSON(
        tileURLs: tileURLs,
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
}
