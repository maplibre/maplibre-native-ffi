package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.generated.MlnEglContextDescriptor
import org.maplibre.nativeffi.internal.wasm.generated.MlnFeature
import org.maplibre.nativeffi.internal.wasm.generated.MlnFeatureCollection
import org.maplibre.nativeffi.internal.wasm.generated.MlnFeatureExtensionResultInfo
import org.maplibre.nativeffi.internal.wasm.generated.MlnFeatureExtensionResultType
import org.maplibre.nativeffi.internal.wasm.generated.MlnFeatureIdentifierType
import org.maplibre.nativeffi.internal.wasm.generated.MlnFeatureStateSelector
import org.maplibre.nativeffi.internal.wasm.generated.MlnFeatureStateSelectorField
import org.maplibre.nativeffi.internal.wasm.generated.MlnLatLng
import org.maplibre.nativeffi.internal.wasm.generated.MlnOpenglBorrowedTextureDescriptor
import org.maplibre.nativeffi.internal.wasm.generated.MlnOpenglContextDescriptor
import org.maplibre.nativeffi.internal.wasm.generated.MlnOpenglContextPlatform
import org.maplibre.nativeffi.internal.wasm.generated.MlnOpenglOwnedTextureFrame
import org.maplibre.nativeffi.internal.wasm.generated.MlnOpenglSurfaceDescriptor
import org.maplibre.nativeffi.internal.wasm.generated.MlnQueriedFeature
import org.maplibre.nativeffi.internal.wasm.generated.MlnQueriedFeatureField
import org.maplibre.nativeffi.internal.wasm.generated.MlnRenderTargetExtent
import org.maplibre.nativeffi.internal.wasm.generated.MlnRenderedFeatureQueryOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnRenderedFeatureQueryOptions
import org.maplibre.nativeffi.internal.wasm.generated.MlnRenderedQueryGeometry
import org.maplibre.nativeffi.internal.wasm.generated.MlnRenderedQueryGeometryType
import org.maplibre.nativeffi.internal.wasm.generated.MlnScreenBox
import org.maplibre.nativeffi.internal.wasm.generated.MlnScreenLineString
import org.maplibre.nativeffi.internal.wasm.generated.MlnScreenPoint
import org.maplibre.nativeffi.internal.wasm.generated.MlnSourceFeatureQueryOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnSourceFeatureQueryOptions
import org.maplibre.nativeffi.internal.wasm.generated.MlnStringView
import org.maplibre.nativeffi.internal.wasm.generated.MlnTextureImageInfo
import org.maplibre.nativeffi.internal.wasm.generated.MlnWebglContextDescriptor
import org.maplibre.nativeffi.internal.wasm.generated.MlnWglContextDescriptor
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.query.FeatureExtensionResult
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions
import org.maplibre.nativeffi.render.EglContextDescriptor
import org.maplibre.nativeffi.render.FrameScope
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLContextDescriptor
import org.maplibre.nativeffi.render.OpenGLOwnedTextureFrame
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.TextureImageInfo
import org.maplibre.nativeffi.render.WebglContextDescriptor
import org.maplibre.nativeffi.render.WglContextDescriptor

/**
 * Places a render session's descriptors into the Emscripten heap, and reads its results back.
 *
 * A render target descriptor is flat and goes straight into scratch. A query descriptor is a tree —
 * options carry a filter that carries an array of values — so it follows the rule the other
 * marshallers do: measure the tree, place it in one arena, hand native a single root pointer. The
 * arena arithmetic comes from [JsonMarshal], which is also what makes a filter, a feature, and a
 * geometry shareable with one block; a second copy of that checked arithmetic would be a second
 * place for an unchecked subtotal to appear.
 *
 * Reading is what this file adds that the others do not have. A query result is native-owned
 * storage that its destroy frees, so every string, JSON value, geometry, and feature below is
 * copied into Kotlin here rather than left as a view onto memory that is about to go away.
 *
 * Every offset and width comes from the generated accessors, so this code names fields.
 */
internal object RenderMarshal {
  /** Bytes a handle-valued or pointer-valued output slot occupies. */
  const val OUT_SLOT_BYTES: Int = 8

  // ---------------------------------------------------------------- render targets

  /** Writes a render target extent, which states its own size as every descriptor does. */
  fun writeExtent(base: HeapPointer, extent: RenderTargetExtent) {
    MlnRenderTargetExtent.setSize(base, MlnRenderTargetExtent.SIZEOF)
    MlnRenderTargetExtent.setWidth(base, extent.width)
    MlnRenderTargetExtent.setHeight(base, extent.height)
    MlnRenderTargetExtent.setScaleFactor(base, extent.scaleFactor)
  }

  /**
   * Writes the platform arm of an OpenGL context descriptor.
   *
   * The arms are the ones the common API declares. This build's OpenGL backend is compiled against
   * WebGL and accepts only its own provider, so a descriptor naming another one is written as given
   * and refused by native, which is where a build's capability is actually known.
   */
  fun writeOpenGLContext(base: HeapPointer, context: OpenGLContextDescriptor) {
    MlnOpenglContextDescriptor.setSize(base, MlnOpenglContextDescriptor.SIZEOF)
    val data = base + MlnOpenglContextDescriptor.OFFSET_DATA
    when (context) {
      is WglContextDescriptor -> {
        MlnOpenglContextDescriptor.setPlatform(
          base,
          MlnOpenglContextPlatform.MLN_OPENGL_CONTEXT_PLATFORM_WGL,
        )
        MlnWglContextDescriptor.setSize(data, MlnWglContextDescriptor.SIZEOF)
        MlnWglContextDescriptor.setDeviceContext(data, address(context.deviceContext))
        MlnWglContextDescriptor.setShareContext(data, address(context.shareContext))
        MlnWglContextDescriptor.setGetProcAddress(data, address(context.getProcAddress))
      }
      is EglContextDescriptor -> {
        MlnOpenglContextDescriptor.setPlatform(
          base,
          MlnOpenglContextPlatform.MLN_OPENGL_CONTEXT_PLATFORM_EGL,
        )
        MlnEglContextDescriptor.setSize(data, MlnEglContextDescriptor.SIZEOF)
        MlnEglContextDescriptor.setDisplay(data, address(context.display))
        MlnEglContextDescriptor.setConfig(data, address(context.config))
        MlnEglContextDescriptor.setShareContext(data, address(context.shareContext))
        MlnEglContextDescriptor.setGetProcAddress(data, address(context.getProcAddress))
      }
      // The arm this target actually renders through. A WebGL context is not an address but an
      // entry in the module's own context table, so it crosses as the index rather than through
      // the pointer narrowing the other two arms need.
      is WebglContextDescriptor -> {
        MlnOpenglContextDescriptor.setPlatform(
          base,
          MlnOpenglContextPlatform.MLN_OPENGL_CONTEXT_PLATFORM_WEBGL,
        )
        MlnWebglContextDescriptor.setSize(data, MlnWebglContextDescriptor.SIZEOF)
        // Native documents the handle as positive, and a zero here is the value a host gets back
        // from a context it failed to create, so it is refused before it reaches a render target.
        Status.requireArgument(context.context > 0) {
          "A WebGL context handle must be positive, but was ${context.context}"
        }
        MlnWebglContextDescriptor.setContext(data, context.context)
      }
    }
  }

  const val OPENGL_SURFACE_SIZEOF: Int = MlnOpenglSurfaceDescriptor.SIZEOF

  /**
   * Writes an OpenGL surface descriptor.
   *
   * The surface field is what a browser makes different. Every other OpenGL provider names a
   * drawable beside the context — an HDC, an EGLSurface — and a WebGL context has none: it is bound
   * to the canvas it was created on, and that canvas's default framebuffer is what the session
   * presents to. So native requires this field to be null here, and passing anything else is
   * refused there rather than silently ignored.
   */
  fun writeOpenGLSurface(base: HeapPointer, descriptor: OpenGLSurfaceDescriptor) {
    MlnOpenglSurfaceDescriptor.setSize(base, MlnOpenglSurfaceDescriptor.SIZEOF)
    writeExtent(base + MlnOpenglSurfaceDescriptor.OFFSET_EXTENT, descriptor.extent)
    writeOpenGLContext(base + MlnOpenglSurfaceDescriptor.OFFSET_CONTEXT, descriptor.context)
    MlnOpenglSurfaceDescriptor.setSurface(base, address(descriptor.surface))
  }

  const val OPENGL_BORROWED_TEXTURE_SIZEOF: Int = MlnOpenglBorrowedTextureDescriptor.SIZEOF

  fun writeOpenGLBorrowedTexture(base: HeapPointer, descriptor: OpenGLBorrowedTextureDescriptor) {
    MlnOpenglBorrowedTextureDescriptor.setSize(base, MlnOpenglBorrowedTextureDescriptor.SIZEOF)
    writeExtent(base + MlnOpenglBorrowedTextureDescriptor.OFFSET_EXTENT, descriptor.extent)
    // A caller-owned texture is sized by its owner, so its physical size is stated rather than
    // derived from the extent above.
    MlnOpenglBorrowedTextureDescriptor.setPhysicalWidth(base, descriptor.physicalWidth)
    MlnOpenglBorrowedTextureDescriptor.setPhysicalHeight(base, descriptor.physicalHeight)
    writeOpenGLContext(base + MlnOpenglBorrowedTextureDescriptor.OFFSET_CONTEXT, descriptor.context)
    MlnOpenglBorrowedTextureDescriptor.setTexture(base, descriptor.texture)
    MlnOpenglBorrowedTextureDescriptor.setTarget(base, descriptor.target)
  }

  /**
   * Narrows a borrowed backend address to what this target can hold.
   *
   * A [NativePointer] is sixty-four bits because the C ABI is on most targets. Here it is not: a
   * browser module addresses thirty-two, so a wider address names memory native could never reach
   * and is refused rather than truncated into one that looks valid.
   */
  private fun address(pointer: NativePointer): HeapPointer {
    val value = pointer.address
    Status.requireArgument(value >= 0 && value <= MAX_ADDRESS) {
      "a native pointer must fit a 32-bit address on this target"
    }
    return HeapPointer(value.toInt())
  }

  // ---------------------------------------------------------------- texture readback

  const val TEXTURE_IMAGE_INFO_SIZEOF: Int = MlnTextureImageInfo.SIZEOF

  /**
   * Writes the readback metadata header alone, for a descriptor native fills.
   *
   * An output descriptor states its size too: native reads it to decide which fields it may write,
   * and a zeroed block asks for a zero-sized one, which it refuses.
   */
  fun writeTextureImageInfoHeader(base: HeapPointer) {
    MlnTextureImageInfo.setSize(base, MlnTextureImageInfo.SIZEOF)
  }

  fun readTextureImageInfo(base: HeapPointer): TextureImageInfo =
    TextureImageInfo(
      MlnTextureImageInfo.width(base),
      MlnTextureImageInfo.height(base),
      MlnTextureImageInfo.stride(base),
      // A byte length is `size_t`, which is unsigned and thirty-two bits here, so its top bit is
      // part of the length rather than a sign.
      MlnTextureImageInfo.byteLength(base).toLong() and MAX_ADDRESS,
    )

  // ---------------------------------------------------------------- owned texture frames

  const val OPENGL_OWNED_TEXTURE_FRAME_SIZEOF: Int = MlnOpenglOwnedTextureFrame.SIZEOF

  fun writeOpenGLFrameHeader(base: HeapPointer) {
    MlnOpenglOwnedTextureFrame.setSize(base, MlnOpenglOwnedTextureFrame.SIZEOF)
  }

  fun readOpenGLFrame(base: HeapPointer, scope: FrameScope): OpenGLOwnedTextureFrame =
    OpenGLOwnedTextureFrame(
      scope,
      MlnOpenglOwnedTextureFrame.generation(base),
      MlnOpenglOwnedTextureFrame.width(base),
      MlnOpenglOwnedTextureFrame.height(base),
      MlnOpenglOwnedTextureFrame.scaleFactor(base),
      MlnOpenglOwnedTextureFrame.frameId(base),
      MlnOpenglOwnedTextureFrame.texture(base),
      MlnOpenglOwnedTextureFrame.target(base),
      MlnOpenglOwnedTextureFrame.internalFormat(base),
      MlnOpenglOwnedTextureFrame.format(base),
      MlnOpenglOwnedTextureFrame.type(base),
    )

  /**
   * Rebuilds an acquired frame's descriptor so that it can be released.
   *
   * Nothing keeps the descriptor native filled at acquire: it lived in scratch that call freed, and
   * a browser host cannot be handed a heap address to hold across its own frame loop. Rebuilding is
   * sound because the C API matches a release by value — it compares the frame's generation and
   * frame id against the acquired ones — rather than by the pointer those values arrive through.
   */
  fun writeOpenGLFrame(base: HeapPointer, frame: OpenGLOwnedTextureFrame) {
    writeOpenGLFrameHeader(base)
    MlnOpenglOwnedTextureFrame.setGeneration(base, frame.generation())
    MlnOpenglOwnedTextureFrame.setWidth(base, frame.width())
    MlnOpenglOwnedTextureFrame.setHeight(base, frame.height())
    MlnOpenglOwnedTextureFrame.setScaleFactor(base, frame.scaleFactor())
    MlnOpenglOwnedTextureFrame.setFrameId(base, frame.frameId())
    MlnOpenglOwnedTextureFrame.setTexture(base, frame.texture())
    MlnOpenglOwnedTextureFrame.setTarget(base, frame.target())
    MlnOpenglOwnedTextureFrame.setInternalFormat(base, frame.internalFormat())
    MlnOpenglOwnedTextureFrame.setFormat(base, frame.format())
    MlnOpenglOwnedTextureFrame.setType(base, frame.type())
  }

  // ---------------------------------------------------------------- string view arguments

  /**
   * Bytes a string view passed as its own argument needs.
   *
   * A C parameter of type `mln_string_view` is taken by value, which this target lowers to a
   * pointer to the view, so the view itself needs a block as well as its text.
   */
  fun measureStringViewRoot(text: String): Long =
    JsonMarshal.plus(JsonMarshal.measureBlock(MlnStringView.SIZEOF), JsonMarshal.measureText(text))

  fun writeStringViewRoot(arena: HeapArena, text: String): HeapPointer {
    val base = JsonMarshal.allocateBlock(arena, MlnStringView.SIZEOF)
    JsonMarshal.writeText(arena, base, text)
    return base
  }

  // ---------------------------------------------------------------- feature state

  fun measureFeatureStateSelector(selector: FeatureStateSelector): Long {
    var total =
      JsonMarshal.plus(
        JsonMarshal.measureBlock(MlnFeatureStateSelector.SIZEOF),
        JsonMarshal.measureText(selector.sourceId),
      )
    selector.sourceLayerId?.let { total = JsonMarshal.plus(total, JsonMarshal.measureText(it)) }
    selector.featureId?.let { total = JsonMarshal.plus(total, JsonMarshal.measureText(it)) }
    selector.stateKey?.let { total = JsonMarshal.plus(total, JsonMarshal.measureText(it)) }
    return total
  }

  fun writeFeatureStateSelector(arena: HeapArena, selector: FeatureStateSelector): HeapPointer {
    val base = JsonMarshal.allocateBlock(arena, MlnFeatureStateSelector.SIZEOF)
    MlnFeatureStateSelector.setSize(base, MlnFeatureStateSelector.SIZEOF)
    // The source ID is required and carries no field bit. The rest are present only where a bit
    // says so, so an absent Kotlin value leaves a bit clear rather than writing an empty view that
    // native would read as a present, empty ID.
    JsonMarshal.writeText(arena, base + MlnFeatureStateSelector.OFFSET_SOURCE_ID, selector.sourceId)
    var fields = 0
    selector.sourceLayerId?.let {
      fields = fields or MlnFeatureStateSelectorField.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
      JsonMarshal.writeText(arena, base + MlnFeatureStateSelector.OFFSET_SOURCE_LAYER_ID, it)
    }
    selector.featureId?.let {
      fields = fields or MlnFeatureStateSelectorField.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
      JsonMarshal.writeText(arena, base + MlnFeatureStateSelector.OFFSET_FEATURE_ID, it)
    }
    selector.stateKey?.let {
      fields = fields or MlnFeatureStateSelectorField.MLN_FEATURE_STATE_SELECTOR_STATE_KEY
      JsonMarshal.writeText(arena, base + MlnFeatureStateSelector.OFFSET_STATE_KEY, it)
    }
    MlnFeatureStateSelector.setFields(base, fields)
    return base
  }

  // ---------------------------------------------------------------- query inputs

  fun measureRenderedQueryGeometry(geometry: RenderedQueryGeometry): Long {
    val root = JsonMarshal.measureBlock(MlnRenderedQueryGeometry.SIZEOF)
    return when (geometry) {
      // A point and a box live in the descriptor's own union arm, so neither needs storage of its
      // own; only a line string points somewhere else.
      is RenderedQueryGeometry.Point -> root
      is RenderedQueryGeometry.Box -> root
      is RenderedQueryGeometry.LineString ->
        JsonMarshal.plus(
          root,
          JsonMarshal.measureArray(MlnScreenPoint.SIZEOF, geometry.points.size),
        )
    }
  }

  fun writeRenderedQueryGeometry(arena: HeapArena, geometry: RenderedQueryGeometry): HeapPointer {
    val base = JsonMarshal.allocateBlock(arena, MlnRenderedQueryGeometry.SIZEOF)
    MlnRenderedQueryGeometry.setSize(base, MlnRenderedQueryGeometry.SIZEOF)
    val data = base + MlnRenderedQueryGeometry.OFFSET_DATA
    when (geometry) {
      is RenderedQueryGeometry.Point -> {
        MlnRenderedQueryGeometry.setType(
          base,
          MlnRenderedQueryGeometryType.MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT,
        )
        writeScreenPoint(data, geometry.point)
      }
      is RenderedQueryGeometry.Box -> {
        MlnRenderedQueryGeometry.setType(
          base,
          MlnRenderedQueryGeometryType.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX,
        )
        writeScreenPoint(data + MlnScreenBox.OFFSET_MIN, geometry.box.min)
        writeScreenPoint(data + MlnScreenBox.OFFSET_MAX, geometry.box.max)
      }
      is RenderedQueryGeometry.LineString -> {
        MlnRenderedQueryGeometry.setType(
          base,
          MlnRenderedQueryGeometryType.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING,
        )
        val points = JsonMarshal.allocateArray(arena, MlnScreenPoint.SIZEOF, geometry.points.size)
        geometry.points.forEachIndexed { index, point ->
          writeScreenPoint(points + index * MlnScreenPoint.SIZEOF, point)
        }
        MlnScreenLineString.setPoints(data, points)
        MlnScreenLineString.setPointCount(data, geometry.points.size)
      }
    }
    return base
  }

  private fun writeScreenPoint(base: HeapPointer, point: ScreenPoint) {
    MlnScreenPoint.setX(base, point.x)
    MlnScreenPoint.setY(base, point.y)
  }

  fun measureRenderedFeatureQueryOptions(options: RenderedFeatureQueryOptions?): Long {
    if (options == null) return 0L
    var total = JsonMarshal.measureBlock(MlnRenderedFeatureQueryOptions.SIZEOF)
    options.layerIds?.let { total = JsonMarshal.plus(total, measureStringViewArray(it)) }
    options.filter?.let { total = JsonMarshal.plus(total, JsonMarshal.measureValue(it, 0)) }
    return total
  }

  /** Returns the null pointer for absent options, which the C API reads as its own defaults. */
  fun writeRenderedFeatureQueryOptions(
    arena: HeapArena,
    options: RenderedFeatureQueryOptions?,
  ): HeapPointer {
    if (options == null) return HeapPointer(0)
    val base = JsonMarshal.allocateBlock(arena, MlnRenderedFeatureQueryOptions.SIZEOF)
    MlnRenderedFeatureQueryOptions.setSize(base, MlnRenderedFeatureQueryOptions.SIZEOF)
    var fields = 0
    options.layerIds?.let { layerIds ->
      fields =
        fields or MlnRenderedFeatureQueryOptionField.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
      MlnRenderedFeatureQueryOptions.setLayerIds(base, writeStringViewArray(arena, layerIds))
      MlnRenderedFeatureQueryOptions.setLayerIdCount(base, layerIds.size)
    }
    // A filter carries no field bit of its own: the C API reads a null pointer as no filter.
    options.filter?.let {
      MlnRenderedFeatureQueryOptions.setFilter(base, JsonMarshal.write(arena, it))
    }
    MlnRenderedFeatureQueryOptions.setFields(base, fields)
    return base
  }

  fun measureSourceFeatureQueryOptions(options: SourceFeatureQueryOptions?): Long {
    if (options == null) return 0L
    var total = JsonMarshal.measureBlock(MlnSourceFeatureQueryOptions.SIZEOF)
    options.sourceLayerIds?.let { total = JsonMarshal.plus(total, measureStringViewArray(it)) }
    options.filter?.let { total = JsonMarshal.plus(total, JsonMarshal.measureValue(it, 0)) }
    return total
  }

  fun writeSourceFeatureQueryOptions(
    arena: HeapArena,
    options: SourceFeatureQueryOptions?,
  ): HeapPointer {
    if (options == null) return HeapPointer(0)
    val base = JsonMarshal.allocateBlock(arena, MlnSourceFeatureQueryOptions.SIZEOF)
    MlnSourceFeatureQueryOptions.setSize(base, MlnSourceFeatureQueryOptions.SIZEOF)
    var fields = 0
    options.sourceLayerIds?.let { sourceLayerIds ->
      fields =
        fields or MlnSourceFeatureQueryOptionField.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
      MlnSourceFeatureQueryOptions.setSourceLayerIds(
        base,
        writeStringViewArray(arena, sourceLayerIds),
      )
      MlnSourceFeatureQueryOptions.setSourceLayerIdCount(base, sourceLayerIds.size)
    }
    options.filter?.let {
      MlnSourceFeatureQueryOptions.setFilter(base, JsonMarshal.write(arena, it))
    }
    MlnSourceFeatureQueryOptions.setFields(base, fields)
    return base
  }

  private fun measureStringViewArray(values: List<String>): Long =
    values.fold(JsonMarshal.measureArray(MlnStringView.SIZEOF, values.size)) { total, value ->
      JsonMarshal.plus(total, JsonMarshal.measureText(value))
    }

  private fun writeStringViewArray(arena: HeapArena, values: List<String>): HeapPointer {
    val base = JsonMarshal.allocateArray(arena, MlnStringView.SIZEOF, values.size)
    values.forEachIndexed { index, value ->
      JsonMarshal.writeText(arena, base + index * MlnStringView.SIZEOF, value)
    }
    return base
  }

  // ---------------------------------------------------------------- query results

  const val QUERIED_FEATURE_SIZEOF: Int = MlnQueriedFeature.SIZEOF

  fun writeQueriedFeatureHeader(base: HeapPointer) {
    MlnQueriedFeature.setSize(base, MlnQueriedFeature.SIZEOF)
  }

  fun readQueriedFeature(base: HeapPointer): QueriedFeature {
    val fields = MlnQueriedFeature.fields(base)
    val sourceId =
      if ((fields and MlnQueriedFeatureField.MLN_QUERIED_FEATURE_SOURCE_ID) != 0) {
        JsonMarshal.readText(base + MlnQueriedFeature.OFFSET_SOURCE_ID)
      } else {
        null
      }
    val sourceLayerId =
      if ((fields and MlnQueriedFeatureField.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID) != 0) {
        JsonMarshal.readText(base + MlnQueriedFeature.OFFSET_SOURCE_LAYER_ID)
      } else {
        null
      }
    val state =
      if ((fields and MlnQueriedFeatureField.MLN_QUERIED_FEATURE_STATE) != 0) {
        readJsonPointer(MlnQueriedFeature.state(base))
      } else {
        null
      }
    return QueriedFeature(
      readFeature(base + MlnQueriedFeature.OFFSET_FEATURE),
      sourceId,
      sourceLayerId,
      state,
    )
  }

  const val FEATURE_EXTENSION_RESULT_INFO_SIZEOF: Int = MlnFeatureExtensionResultInfo.SIZEOF

  fun writeFeatureExtensionResultInfoHeader(base: HeapPointer) {
    MlnFeatureExtensionResultInfo.setSize(base, MlnFeatureExtensionResultInfo.SIZEOF)
  }

  fun readFeatureExtensionResultInfo(base: HeapPointer): FeatureExtensionResult {
    val data = base + MlnFeatureExtensionResultInfo.OFFSET_DATA
    return when (val type = MlnFeatureExtensionResultInfo.type(base)) {
      MlnFeatureExtensionResultType.MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE ->
        // The arm is a bare pointer rather than a struct, so there is no generated field accessor
        // to name; what is read is the union's own address.
        FeatureExtensionResult.Value(
          readJsonPointer(HeapPointer(Heap.loadInt(data))) ?: JsonValue.Null
        )
      MlnFeatureExtensionResultType.MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION ->
        FeatureExtensionResult.FeatureCollection(readFeatureCollection(data))
      // A tag from a newer C API than this binding was generated against. The arm it selects is
      // unknown, so nothing here may read the union; the raw tag is kept so a caller can see what
      // arrived.
      else -> FeatureExtensionResult.Unknown(type)
    }
  }

  /** Reads a JSON value behind a pointer native may leave null to mean absent. */
  fun readJsonPointer(base: HeapPointer): JsonValue? =
    if (base.address == 0) null else JsonMarshal.read(base)

  private fun readFeatureCollection(base: HeapPointer): List<Feature> {
    val features = MlnFeatureCollection.features(base)
    return List(readCount(MlnFeatureCollection.featureCount(base))) { index ->
      readFeature(features + index * MlnFeature.SIZEOF)
    }
  }

  // ---------------------------------------------------------------- features, reading only

  /**
   * Reads a feature descriptor native owns.
   *
   * Writing lives in [GeoJsonMarshal], which has to measure and place a tree. Reading only walks
   * pointers native has already placed, so it needs none of that machinery — but it does have to
   * copy, because the storage it walks belongs to a result handle that is about to be destroyed.
   */
  fun readFeature(base: HeapPointer): Feature =
    Feature(
      readGeometry(MlnFeature.geometry(base), 0),
      JsonMarshal.readMembers(
        MlnFeature.properties(base),
        readCount(MlnFeature.propertyCount(base)),
        // Properties are a root member array rather than an object's, so their values start at
        // depth zero, matching how they are written.
        0,
      ),
      readFeatureIdentifier(base),
    )

  private fun readFeatureIdentifier(base: HeapPointer): FeatureIdentifier {
    val data = base + MlnFeature.OFFSET_IDENTIFIER
    return when (val type = MlnFeature.identifierType(base)) {
      MlnFeatureIdentifierType.MLN_FEATURE_IDENTIFIER_TYPE_NULL -> FeatureIdentifier.Null
      // Carried as the bit pattern it was read as. The C arm is unsigned and Kotlin's Long is not,
      // so reinterpreting here would change the identifier rather than preserve it.
      MlnFeatureIdentifierType.MLN_FEATURE_IDENTIFIER_TYPE_UINT ->
        FeatureIdentifier.UInt(Heap.loadLong(data))
      MlnFeatureIdentifierType.MLN_FEATURE_IDENTIFIER_TYPE_INT ->
        FeatureIdentifier.Int(Heap.loadLong(data))
      MlnFeatureIdentifierType.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE ->
        FeatureIdentifier.DoubleValue(Heap.loadDouble(data))
      MlnFeatureIdentifierType.MLN_FEATURE_IDENTIFIER_TYPE_STRING ->
        FeatureIdentifier.StringValue(JsonMarshal.readText(data))
      // A tag from a newer C API than this binding was generated against, kept rather than
      // rejected so a caller can still see the rest of the feature it arrived in.
      else -> FeatureIdentifier.Unknown(type)
    }
  }

  // ---------------------------------------------------------------- geometry, reading only

  /** Reads a geometry tree native owns. Both halves live in [GeometryMarshal]. */
  fun readGeometry(base: HeapPointer, depth: Int): Geometry = GeometryMarshal.read(base, depth)

  private fun readLatLng(base: HeapPointer): LatLng =
    LatLng(MlnLatLng.latitude(base), MlnLatLng.longitude(base))

  /**
   * Refuses a tree deeper than the C API accepts.
   *
   * Checked before recursing rather than left to the walk: a deep enough tree exhausts this
   * module's own stack, and the descriptor being read is only as trustworthy as the module that
   * produced it.
   */
  private fun requireGeometryDepth(depth: Int) {
    if (depth > Geometry.MAX_COLLECTION_DEPTH) {
      throw Status.invalidArgument(
        "geometry nests deeper than the ${Geometry.MAX_COLLECTION_DEPTH} levels the C API accepts"
      )
    }
  }

  /**
   * Refuses a count native reported that no real descriptor could carry.
   *
   * `size_t` is thirty-two bits on this target, so a value past [Int.MAX_VALUE] arrives negative.
   * The heap could not hold a descriptor that large, so a negative count means the address being
   * read is not the descriptor it was taken for, and continuing would index arbitrary memory.
   */
  private fun readCount(count: Int): Int {
    if (count < 0) {
      throw Status.invalidState(
        "The MapLibre Native browser module reported a descriptor count of $count"
      )
    }
    return count
  }

  /** The largest address this target can hold, as an unsigned thirty-two-bit value. */
  private const val MAX_ADDRESS = 0xFFFFFFFFL
}
