package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.BoundsConstraint
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.generated.MlnBoundOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnBoundOptions
import org.maplibre.nativeffi.internal.wasm.generated.MlnLatLngBounds
import org.maplibre.nativeffi.internal.wasm.generated.MlnMapOptions
import org.maplibre.nativeffi.internal.wasm.generated.MlnMapTileOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnMapTileOptions
import org.maplibre.nativeffi.internal.wasm.generated.MlnMapViewportOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnMapViewportOptions
import org.maplibre.nativeffi.map.ConstrainMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.map.NorthOrientation
import org.maplibre.nativeffi.map.TileLodMode
import org.maplibre.nativeffi.map.TileOptions
import org.maplibre.nativeffi.map.ViewportMode
import org.maplibre.nativeffi.map.ViewportOptions

/**
 * Places the map's own descriptors into the Emscripten heap, and reads them back.
 *
 * Three of the four pair their values with a bit per field, so an absent Kotlin value is a bit left
 * clear and a clear bit reads back as null. [MapOptions] is the exception and says why below.
 *
 * Every offset and width here comes from the generated accessors, so this code names fields.
 */
internal object MapOptionsMarshal {
  val MAP_OPTIONS_SIZEOF: Int = MlnMapOptions.SIZEOF
  val BOUND_OPTIONS_SIZEOF: Int = MlnBoundOptions.SIZEOF
  val VIEWPORT_OPTIONS_SIZEOF: Int = MlnMapViewportOptions.SIZEOF
  val TILE_OPTIONS_SIZEOF: Int = MlnMapTileOptions.SIZEOF

  /** The entry point that fills a map descriptor with the C API's own defaults. */
  private const val MAP_OPTIONS_DEFAULT = "mln_map_options_default"

  /** Slots that entry point reads: a struct return takes its destination as the first one. */
  private const val MAP_OPTIONS_DEFAULT_SLOTS = 1

  /**
   * Writes [options] at [base], leaving every absent value at the C API's own default.
   *
   * This descriptor carries no field mask, so an absent value still has to be a value: a zeroed
   * block asks for a map of zero width at zero scale, which the C API rejects. The defaults are
   * read from the module rather than copied here as constants, so a map created through this
   * binding matches one created through any other.
   */
  fun writeMapOptions(base: HeapPointer, options: MapOptions) {
    NativeCall.call(
      MAP_OPTIONS_DEFAULT,
      MAP_OPTIONS_DEFAULT_SLOTS,
      fill = { it.setPointer(0, base) },
      read = {},
    )
    // The default carries a size too, but it is the module's rather than this binding's. Stating it
    // here keeps every descriptor reporting the size these offsets were generated against.
    MlnMapOptions.setSize(base, MlnMapOptions.SIZEOF)
    options.width?.let { MlnMapOptions.setWidth(base, it) }
    options.height?.let { MlnMapOptions.setHeight(base, it) }
    options.scaleFactor?.let { MlnMapOptions.setScaleFactor(base, it) }
    options.mapMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown map mode cannot be used as input: ${it.nativeValue}"
      }
      MlnMapOptions.setMapMode(base, it.nativeValue)
    }
    options.fastPforEnabled?.let { MlnMapOptions.setFastPforEnabled(base, it) }
  }

  /**
   * Writes a bound descriptor's header alone, for a buffer native fills.
   *
   * An output descriptor still states its size: native reads it to decide which fields it may
   * write, and a zeroed block would ask for a zero-sized descriptor.
   */
  fun writeBoundOptionsHeader(base: HeapPointer) {
    MlnBoundOptions.setSize(base, MlnBoundOptions.SIZEOF)
  }

  /** Writes [options] at [base], setting a field's bit only where the value is present. */
  fun writeBoundOptions(base: HeapPointer, options: BoundOptions) {
    MlnBoundOptions.setSize(base, MlnBoundOptions.SIZEOF)
    var fields = 0
    // The two constraint bits are mutually exclusive, and the unbounded one leaves the bounds
    // unread, so the sealed constraint maps to one bit or the other rather than to both.
    when (val constraint = options.bounds) {
      is BoundsConstraint.Bounded -> {
        fields = fields or MlnBoundOptionField.MLN_BOUND_OPTION_BOUNDS
        writeLatLngBounds(base + MlnBoundOptions.OFFSET_BOUNDS, constraint.bounds)
      }
      BoundsConstraint.Unbounded -> {
        fields = fields or MlnBoundOptionField.MLN_BOUND_OPTION_UNBOUNDED
      }
      null -> {}
    }
    options.minZoom?.let {
      fields = fields or MlnBoundOptionField.MLN_BOUND_OPTION_MIN_ZOOM
      MlnBoundOptions.setMinZoom(base, it)
    }
    options.maxZoom?.let {
      fields = fields or MlnBoundOptionField.MLN_BOUND_OPTION_MAX_ZOOM
      MlnBoundOptions.setMaxZoom(base, it)
    }
    options.minPitch?.let {
      fields = fields or MlnBoundOptionField.MLN_BOUND_OPTION_MIN_PITCH
      MlnBoundOptions.setMinPitch(base, it)
    }
    options.maxPitch?.let {
      fields = fields or MlnBoundOptionField.MLN_BOUND_OPTION_MAX_PITCH
      MlnBoundOptions.setMaxPitch(base, it)
    }
    MlnBoundOptions.setFields(base, fields)
  }

  /** Reads the bounds at [base], producing null for every field whose bit is clear. */
  fun readBoundOptions(base: HeapPointer): BoundOptions {
    val fields = MlnBoundOptions.fields(base)
    fun has(bit: Int) = (fields and bit) != 0
    return BoundOptions().also {
      if (has(MlnBoundOptionField.MLN_BOUND_OPTION_BOUNDS)) {
        it.bounds = BoundsConstraint.Bounded(readLatLngBounds(base + MlnBoundOptions.OFFSET_BOUNDS))
      } else if (has(MlnBoundOptionField.MLN_BOUND_OPTION_UNBOUNDED)) {
        it.bounds = BoundsConstraint.Unbounded
      }
      if (has(MlnBoundOptionField.MLN_BOUND_OPTION_MIN_ZOOM)) {
        it.minZoom = MlnBoundOptions.minZoom(base)
      }
      if (has(MlnBoundOptionField.MLN_BOUND_OPTION_MAX_ZOOM)) {
        it.maxZoom = MlnBoundOptions.maxZoom(base)
      }
      if (has(MlnBoundOptionField.MLN_BOUND_OPTION_MIN_PITCH)) {
        it.minPitch = MlnBoundOptions.minPitch(base)
      }
      if (has(MlnBoundOptionField.MLN_BOUND_OPTION_MAX_PITCH)) {
        it.maxPitch = MlnBoundOptions.maxPitch(base)
      }
    }
  }

  /** Writes a viewport descriptor's header alone, for a buffer native fills. */
  fun writeViewportOptionsHeader(base: HeapPointer) {
    MlnMapViewportOptions.setSize(base, MlnMapViewportOptions.SIZEOF)
  }

  /** Writes [options] at [base], setting a field's bit only where the value is present. */
  fun writeViewportOptions(base: HeapPointer, options: ViewportOptions) {
    MlnMapViewportOptions.setSize(base, MlnMapViewportOptions.SIZEOF)
    var fields = 0
    options.northOrientation?.let {
      // The open domain preserves a value native reported, so it can hold one this binding never
      // named. Sending that back would ask native for a viewport it has no meaning for.
      Status.requireArgument(it.isKnown) {
        "Unknown north orientation cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MlnMapViewportOptionField.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
      MlnMapViewportOptions.setNorthOrientation(base, it.nativeValue)
    }
    options.constrainMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown constrain mode cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MlnMapViewportOptionField.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
      MlnMapViewportOptions.setConstrainMode(base, it.nativeValue)
    }
    options.viewportMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown viewport mode cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MlnMapViewportOptionField.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
      MlnMapViewportOptions.setViewportMode(base, it.nativeValue)
    }
    options.frustumOffset?.let {
      fields = fields or MlnMapViewportOptionField.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET
      CameraMarshal.writeEdgeInsets(base + MlnMapViewportOptions.OFFSET_FRUSTUM_OFFSET, it)
    }
    MlnMapViewportOptions.setFields(base, fields)
  }

  /** Reads the viewport options at [base], producing null for every field whose bit is clear. */
  fun readViewportOptions(base: HeapPointer): ViewportOptions {
    val fields = MlnMapViewportOptions.fields(base)
    fun has(bit: Int) = (fields and bit) != 0
    return ViewportOptions().also {
      if (has(MlnMapViewportOptionField.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION)) {
        it.northOrientation =
          NorthOrientation.fromNative(MlnMapViewportOptions.northOrientation(base))
      }
      if (has(MlnMapViewportOptionField.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE)) {
        it.constrainMode = ConstrainMode.fromNative(MlnMapViewportOptions.constrainMode(base))
      }
      if (has(MlnMapViewportOptionField.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE)) {
        it.viewportMode = ViewportMode.fromNative(MlnMapViewportOptions.viewportMode(base))
      }
      if (has(MlnMapViewportOptionField.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET)) {
        it.frustumOffset =
          CameraMarshal.readEdgeInsets(base + MlnMapViewportOptions.OFFSET_FRUSTUM_OFFSET)
      }
    }
  }

  /** Writes a tile descriptor's header alone, for a buffer native fills. */
  fun writeTileOptionsHeader(base: HeapPointer) {
    MlnMapTileOptions.setSize(base, MlnMapTileOptions.SIZEOF)
  }

  /** Writes [options] at [base], setting a field's bit only where the value is present. */
  fun writeTileOptions(base: HeapPointer, options: TileOptions) {
    MlnMapTileOptions.setSize(base, MlnMapTileOptions.SIZEOF)
    var fields = 0
    options.prefetchZoomDelta?.let {
      fields = fields or MlnMapTileOptionField.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA
      MlnMapTileOptions.setPrefetchZoomDelta(base, it)
    }
    options.lodMinRadius?.let {
      fields = fields or MlnMapTileOptionField.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS
      MlnMapTileOptions.setLodMinRadius(base, it)
    }
    options.lodScale?.let {
      fields = fields or MlnMapTileOptionField.MLN_MAP_TILE_OPTION_LOD_SCALE
      MlnMapTileOptions.setLodScale(base, it)
    }
    options.lodPitchThreshold?.let {
      fields = fields or MlnMapTileOptionField.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD
      MlnMapTileOptions.setLodPitchThreshold(base, it)
    }
    options.lodZoomShift?.let {
      fields = fields or MlnMapTileOptionField.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT
      MlnMapTileOptions.setLodZoomShift(base, it)
    }
    options.lodMode?.let {
      Status.requireArgument(it.isKnown) {
        "Unknown tile LOD mode cannot be used as input: ${it.nativeValue}"
      }
      fields = fields or MlnMapTileOptionField.MLN_MAP_TILE_OPTION_LOD_MODE
      MlnMapTileOptions.setLodMode(base, it.nativeValue)
    }
    MlnMapTileOptions.setFields(base, fields)
  }

  /** Reads the tile options at [base], producing null for every field whose bit is clear. */
  fun readTileOptions(base: HeapPointer): TileOptions {
    val fields = MlnMapTileOptions.fields(base)
    fun has(bit: Int) = (fields and bit) != 0
    return TileOptions().also {
      if (has(MlnMapTileOptionField.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA)) {
        it.prefetchZoomDelta = MlnMapTileOptions.prefetchZoomDelta(base)
      }
      if (has(MlnMapTileOptionField.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS)) {
        it.lodMinRadius = MlnMapTileOptions.lodMinRadius(base)
      }
      if (has(MlnMapTileOptionField.MLN_MAP_TILE_OPTION_LOD_SCALE)) {
        it.lodScale = MlnMapTileOptions.lodScale(base)
      }
      if (has(MlnMapTileOptionField.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD)) {
        it.lodPitchThreshold = MlnMapTileOptions.lodPitchThreshold(base)
      }
      if (has(MlnMapTileOptionField.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT)) {
        it.lodZoomShift = MlnMapTileOptions.lodZoomShift(base)
      }
      if (has(MlnMapTileOptionField.MLN_MAP_TILE_OPTION_LOD_MODE)) {
        it.lodMode = TileLodMode.fromNative(MlnMapTileOptions.lodMode(base))
      }
    }
  }

  /** Writes a bounds pair, which carries no field mask of its own. */
  fun writeLatLngBounds(base: HeapPointer, bounds: LatLngBounds) {
    CameraMarshal.writeLatLng(base + MlnLatLngBounds.OFFSET_SOUTHWEST, bounds.southwest)
    CameraMarshal.writeLatLng(base + MlnLatLngBounds.OFFSET_NORTHEAST, bounds.northeast)
  }

  fun readLatLngBounds(base: HeapPointer): LatLngBounds =
    LatLngBounds(
      CameraMarshal.readLatLng(base + MlnLatLngBounds.OFFSET_SOUTHWEST),
      CameraMarshal.readLatLng(base + MlnLatLngBounds.OFFSET_NORTHEAST),
    )
}
