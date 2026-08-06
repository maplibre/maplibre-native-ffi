package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.generated.MlnCustomGeometrySourceOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnCustomGeometrySourceOptions
import org.maplibre.nativeffi.internal.wasm.generated.MlnGeojsonSourceOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnGeojsonSourceOptions
import org.maplibre.nativeffi.internal.wasm.generated.MlnStyleSourceInfo
import org.maplibre.nativeffi.internal.wasm.generated.MlnStyleSourceInfoField
import org.maplibre.nativeffi.internal.wasm.generated.MlnStyleTileSourceOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnStyleTileSourceOptions
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.style.RasterDemEncoding
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.TileJson
import org.maplibre.nativeffi.style.TileScheme
import org.maplibre.nativeffi.style.TileSourceOptions
import org.maplibre.nativeffi.style.VectorTileEncoding

/**
 * Places the style source descriptors into the Emscripten heap, and reads source metadata back.
 *
 * Each source descriptor pairs its values with a bit per field, so an absent Kotlin value is a bit
 * left clear rather than a sentinel written into the value.
 *
 * Three of them reach past their own bytes. A tile source carries its attribution as a string view,
 * so the text is placed beside the descriptor and the descriptor is measured before it is written.
 * A GeoJSON source borrows a JSON graph for the call, and a custom geometry source borrows the
 * callbacks the module's function table holds; both arrive as addresses their owners placed.
 *
 * Every offset and width here comes from the generated accessors, so this code names fields.
 */
internal object SourceMarshal {
  val GEOJSON_SOURCE_OPTIONS_SIZEOF: Int = MlnGeojsonSourceOptions.SIZEOF
  val CUSTOM_GEOMETRY_SOURCE_OPTIONS_SIZEOF: Int = MlnCustomGeometrySourceOptions.SIZEOF
  val SOURCE_INFO_SIZEOF: Int = MlnStyleSourceInfo.SIZEOF

  /**
   * Writes [options] at [base], with [clusterProperties] addressing the placed JSON graph.
   *
   * The C descriptor borrows the cluster properties for the call, so the graph is placed by whoever
   * owns the JSON marshalling and has to stay alive until the call returns. Passing the options
   * without the graph would set the field's bit over a null pointer, so it is refused here rather
   * than left for native to reject.
   */
  fun writeGeoJsonSourceOptions(
    base: HeapPointer,
    options: GeoJsonSourceOptions,
    clusterProperties: HeapPointer?,
  ) {
    // The leading size field is how the C API versions a descriptor: it carries the size this
    // binding was generated against so native can tell which fields it may read.
    MlnGeojsonSourceOptions.setSize(base, MlnGeojsonSourceOptions.SIZEOF)
    var fields = 0
    options.minZoom?.let {
      fields = fields or MlnGeojsonSourceOptionField.MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM
      MlnGeojsonSourceOptions.setMinZoom(base, it)
    }
    options.maxZoom?.let {
      fields = fields or MlnGeojsonSourceOptionField.MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM
      MlnGeojsonSourceOptions.setMaxZoom(base, it)
    }
    options.tolerance?.let {
      fields = fields or MlnGeojsonSourceOptionField.MLN_GEOJSON_SOURCE_OPTION_TOLERANCE
      MlnGeojsonSourceOptions.setTolerance(base, it)
    }
    options.clusterMaxZoom?.let {
      fields = fields or MlnGeojsonSourceOptionField.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
      MlnGeojsonSourceOptions.setClusterMaxZoom(base, it)
    }
    options.clusterProperties?.let {
      val graph =
        clusterProperties
          ?: throw Status.invalidArgument(
            "cluster properties were requested without a placed JSON graph"
          )
      fields = fields or MlnGeojsonSourceOptionField.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
      MlnGeojsonSourceOptions.setClusterProperties(base, graph)
    }
    options.tileSize?.let {
      fields = fields or MlnGeojsonSourceOptionField.MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE
      MlnGeojsonSourceOptions.setTileSize(base, it)
    }
    options.buffer?.let {
      fields = fields or MlnGeojsonSourceOptionField.MLN_GEOJSON_SOURCE_OPTION_BUFFER
      MlnGeojsonSourceOptions.setBuffer(base, it)
    }
    options.clusterRadius?.let {
      fields = fields or MlnGeojsonSourceOptionField.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS
      MlnGeojsonSourceOptions.setClusterRadius(base, it)
    }
    options.clusterMinPoints?.let {
      fields = fields or MlnGeojsonSourceOptionField.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
      MlnGeojsonSourceOptions.setClusterMinPoints(base, it)
    }
    options.lineMetrics?.let {
      fields = fields or MlnGeojsonSourceOptionField.MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS
      MlnGeojsonSourceOptions.setLineMetrics(base, it)
    }
    options.cluster?.let {
      fields = fields or MlnGeojsonSourceOptionField.MLN_GEOJSON_SOURCE_OPTION_CLUSTER
      MlnGeojsonSourceOptions.setCluster(base, it)
    }
    options.synchronousUpdate?.let {
      fields = fields or MlnGeojsonSourceOptionField.MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE
      MlnGeojsonSourceOptions.setSynchronousUpdate(base, it)
    }
    MlnGeojsonSourceOptions.setFields(base, fields)
  }

  /**
   * Bytes [options] needs, including the descriptor and any attribution text.
   *
   * The attribution crosses as a string view over memory native reads during the call, so the text
   * is placed beside the descriptor: one acquisition and one release however the options are
   * shaped. Measuring first is what lets both live in one block.
   */
  fun measureTileSourceOptions(options: TileSourceOptions): Int {
    val attribution = options.attribution?.let { JsonMarshal.measureText(it) } ?: 0L
    // Measured and added through the shared arena helpers, so the padding a block leaves behind and
    // a total that would wrap a 32-bit count are accounted for the one way every descriptor here
    // accounts for them.
    return JsonMarshal.plus(JsonMarshal.measureBlock(MlnStyleTileSourceOptions.SIZEOF), attribution)
      .toInt()
  }

  /** Writes [options] into [arena] and returns the descriptor's address. */
  fun writeTileSourceOptions(arena: HeapArena, options: TileSourceOptions): HeapPointer {
    val base = JsonMarshal.allocateBlock(arena, MlnStyleTileSourceOptions.SIZEOF)
    MlnStyleTileSourceOptions.setSize(base, MlnStyleTileSourceOptions.SIZEOF)
    var fields = 0
    options.minZoom?.let {
      fields = fields or MlnStyleTileSourceOptionField.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM
      MlnStyleTileSourceOptions.setMinZoom(base, it)
    }
    options.maxZoom?.let {
      fields = fields or MlnStyleTileSourceOptionField.MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM
      MlnStyleTileSourceOptions.setMaxZoom(base, it)
    }
    options.attribution?.let {
      fields = fields or MlnStyleTileSourceOptionField.MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION
      JsonMarshal.writeText(arena, base + MlnStyleTileSourceOptions.OFFSET_ATTRIBUTION, it)
    }
    options.scheme?.let {
      fields = fields or MlnStyleTileSourceOptionField.MLN_STYLE_TILE_SOURCE_OPTION_SCHEME
      MlnStyleTileSourceOptions.setScheme(base, it.nativeValue)
    }
    options.bounds?.let {
      fields = fields or MlnStyleTileSourceOptionField.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS
      MapOptionsMarshal.writeLatLngBounds(base + MlnStyleTileSourceOptions.OFFSET_BOUNDS, it)
    }
    options.tileSize?.let {
      fields = fields or MlnStyleTileSourceOptionField.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE
      MlnStyleTileSourceOptions.setTileSize(base, it)
    }
    options.vectorEncoding?.let {
      fields = fields or MlnStyleTileSourceOptionField.MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
      MlnStyleTileSourceOptions.setVectorEncoding(base, it.nativeValue)
    }
    options.rasterDemEncoding?.let {
      fields = fields or MlnStyleTileSourceOptionField.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
      MlnStyleTileSourceOptions.setRasterEncoding(base, it.nativeValue)
    }
    MlnStyleTileSourceOptions.setFields(base, fields)
    return base
  }

  /**
   * Writes [options] at [base], with the tile callbacks the caller registered.
   *
   * [fetchTile] and [cancelTile] are indices into the module's function table rather than heap
   * addresses, because a WebAssembly module's code lives outside the memory a pointer addresses.
   * The callback bridge owns them and the context [userData] addresses, and keeps both alive for as
   * long as the source exists. A [cancelTile] of zero leaves the optional cancel callback unset.
   */
  fun writeCustomGeometrySourceOptions(
    base: HeapPointer,
    options: CustomGeometrySourceOptions,
    fetchTile: Int,
    cancelTile: Int,
    userData: HeapPointer,
  ) {
    MlnCustomGeometrySourceOptions.setSize(base, MlnCustomGeometrySourceOptions.SIZEOF)
    // Function pointers carry no generated accessor, because an offset alone cannot say what a
    // table index means; they are the one place here that names a field by its offset constant.
    Heap.storeInt(base + MlnCustomGeometrySourceOptions.OFFSET_FETCH_TILE, fetchTile)
    Heap.storeInt(base + MlnCustomGeometrySourceOptions.OFFSET_CANCEL_TILE, cancelTile)
    MlnCustomGeometrySourceOptions.setUserData(base, userData)
    var fields = 0
    options.minZoom?.let {
      fields =
        fields or MlnCustomGeometrySourceOptionField.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
      MlnCustomGeometrySourceOptions.setMinZoom(base, it)
    }
    options.maxZoom?.let {
      fields =
        fields or MlnCustomGeometrySourceOptionField.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
      MlnCustomGeometrySourceOptions.setMaxZoom(base, it)
    }
    options.tolerance?.let {
      fields =
        fields or MlnCustomGeometrySourceOptionField.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE
      MlnCustomGeometrySourceOptions.setTolerance(base, it)
    }
    options.tileSize?.let {
      fields =
        fields or MlnCustomGeometrySourceOptionField.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE
      MlnCustomGeometrySourceOptions.setTileSize(base, it)
    }
    options.buffer?.let {
      fields = fields or MlnCustomGeometrySourceOptionField.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER
      MlnCustomGeometrySourceOptions.setBuffer(base, it)
    }
    options.clip?.let {
      fields = fields or MlnCustomGeometrySourceOptionField.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP
      MlnCustomGeometrySourceOptions.setClip(base, it)
    }
    options.wrap?.let {
      fields = fields or MlnCustomGeometrySourceOptionField.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP
      MlnCustomGeometrySourceOptions.setWrap(base, it)
    }
    MlnCustomGeometrySourceOptions.setFields(base, fields)
  }

  /**
   * Writes a source metadata header alone, for a buffer native fills.
   *
   * An output descriptor still states its size: native reads it to decide which fields it may
   * write, and a zeroed block would ask for a zero-sized descriptor.
   */
  fun writeSourceInfoHeader(base: HeapPointer) {
    MlnStyleSourceInfo.setSize(base, MlnStyleSourceInfo.SIZEOF)
  }

  /** Reports whether the source at [base] carries attribution worth a second call to copy. */
  fun sourceInfoHasAttribution(base: HeapPointer): Boolean = MlnStyleSourceInfo.hasAttribution(base)

  /** Reports whether the source at [base] retains a URL, which a second call copies. */
  fun sourceInfoHasUrl(base: HeapPointer): Boolean =
    sourceInfoHas(base, MlnStyleSourceInfoField.MLN_STYLE_SOURCE_INFO_URL)

  /** Reports whether the source at [base] was defined with inline TileJSON. */
  fun sourceInfoHasTileJson(base: HeapPointer): Boolean =
    sourceInfoHas(base, MlnStyleSourceInfoField.MLN_STYLE_SOURCE_INFO_TILEJSON)

  /**
   * Reads the source metadata at [base], with the strings the caller copied.
   *
   * The C descriptor carries string lengths and counts rather than string contents, so the
   * attribution, the URL, and the inline tile URLs arrive from separate calls rather than from
   * these bytes.
   */
  fun readSourceInfo(
    base: HeapPointer,
    attribution: String?,
    url: String?,
    tileUrls: List<String>?,
  ): SourceInfo =
    SourceInfo(
      SourceType.fromNative(MlnStyleSourceInfo.type(base)),
      MlnStyleSourceInfo.isVolatile(base),
      attribution,
      if (sourceInfoHasUrl(base)) url else null,
      if (sourceInfoHasTileJson(base)) readTileJson(base, tileUrls.orEmpty()) else null,
      if (sourceInfoHas(base, MlnStyleSourceInfoField.MLN_STYLE_SOURCE_INFO_TILE_SIZE)) {
        MlnStyleSourceInfo.tileSize(base)
      } else {
        null
      },
      if (sourceInfoHas(base, MlnStyleSourceInfoField.MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING)) {
        VectorTileEncoding.fromNative(MlnStyleSourceInfo.vectorEncoding(base))
      } else {
        null
      },
      if (sourceInfoHas(base, MlnStyleSourceInfoField.MLN_STYLE_SOURCE_INFO_RASTER_ENCODING)) {
        RasterDemEncoding.fromNative(MlnStyleSourceInfo.rasterEncoding(base))
      } else {
        null
      },
    )

  private fun readTileJson(base: HeapPointer, tileUrls: List<String>): TileJson =
    TileJson(
      tileUrls,
      MlnStyleSourceInfo.minZoom(base),
      MlnStyleSourceInfo.maxZoom(base),
      TileScheme.fromNative(MlnStyleSourceInfo.scheme(base)),
      if (sourceInfoHas(base, MlnStyleSourceInfoField.MLN_STYLE_SOURCE_INFO_BOUNDS)) {
        MapOptionsMarshal.readLatLngBounds(base + MlnStyleSourceInfo.OFFSET_BOUNDS)
      } else {
        null
      },
    )

  private fun sourceInfoHas(base: HeapPointer, field: Int): Boolean =
    (MlnStyleSourceInfo.fields(base) and field) != 0

  /**
   * Reads a source type from the out-parameter at [pointer].
   *
   * The C API reports it as a bare enum rather than inside a descriptor, so the caller positions
   * the four bytes and this names what they hold.
   */
  fun readSourceType(pointer: HeapPointer): SourceType =
    SourceType.fromNative(Heap.loadInt(pointer))
}
