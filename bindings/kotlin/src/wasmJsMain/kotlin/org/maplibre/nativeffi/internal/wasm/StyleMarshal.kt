package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.generated.MlnImageContent
import org.maplibre.nativeffi.internal.wasm.generated.MlnImageStretch
import org.maplibre.nativeffi.internal.wasm.generated.MlnPremultipliedRgba8Image
import org.maplibre.nativeffi.internal.wasm.generated.MlnStyleImageInfo
import org.maplibre.nativeffi.internal.wasm.generated.MlnStyleImageOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnStyleImageOptions
import org.maplibre.nativeffi.internal.wasm.generated.MlnStyleTransitionOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnStyleTransitionOptions
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.style.ImageContent
import org.maplibre.nativeffi.style.ImageStretch
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.StyleImageTextFit
import org.maplibre.nativeffi.style.StyleTransitionOptions

/**
 * Places the style's image and transition descriptors into the Emscripten heap, and reads them
 * back.
 *
 * The transition options pair their values with a bit per field, so an absent Kotlin value is a bit
 * left clear and a clear bit reads back as null. Image metadata reports optional fields through
 * flags of its own instead, and reading honours them the same way.
 *
 * Every offset and width here comes from the generated accessors, so this code names fields.
 */
internal object StyleMarshal {
  val IMAGE_INFO_SIZEOF: Int = MlnStyleImageInfo.SIZEOF
  val TRANSITION_OPTIONS_SIZEOF: Int = MlnStyleTransitionOptions.SIZEOF
  val IMAGE_STRETCH_SIZEOF: Int = MlnImageStretch.SIZEOF

  /**
   * Writes an image metadata header alone, for a buffer native fills.
   *
   * An output descriptor still states its size: native reads it to decide which fields it may
   * write, and a zeroed block would ask for a zero-sized descriptor.
   */
  fun writeImageInfoHeader(base: HeapPointer) {
    MlnStyleImageInfo.setSize(base, MlnStyleImageInfo.SIZEOF)
  }

  /** Reads the image metadata at [base], producing null for every field whose flag is clear. */
  fun readImageInfo(base: HeapPointer): StyleImageInfo =
    StyleImageInfo(
      width = MlnStyleImageInfo.width(base),
      height = MlnStyleImageInfo.height(base),
      stride = MlnStyleImageInfo.stride(base),
      byteLength = size(MlnStyleImageInfo.byteLength(base)),
      pixelRatio = MlnStyleImageInfo.pixelRatio(base),
      sdf = MlnStyleImageInfo.sdf(base),
      stretchXCount = size(MlnStyleImageInfo.stretchXCount(base)),
      stretchYCount = size(MlnStyleImageInfo.stretchYCount(base)),
      content =
        if (MlnStyleImageInfo.hasContent(base)) {
          readImageContent(base + MlnStyleImageInfo.OFFSET_CONTENT)
        } else {
          null
        },
      textFitWidth =
        if (MlnStyleImageInfo.hasTextFitWidth(base)) {
          StyleImageTextFit.fromNative(MlnStyleImageInfo.textFitWidth(base))
        } else {
          null
        },
      textFitHeight =
        if (MlnStyleImageInfo.hasTextFitHeight(base)) {
          StyleImageTextFit.fromNative(MlnStyleImageInfo.textFitHeight(base))
        } else {
          null
        },
    )

  /**
   * Bytes [options] needs, including the descriptor and either stretch array.
   *
   * The stretch arrays cross as pointers over memory native reads during the call, so they are
   * placed beside the descriptor: one acquisition and one release however the options are shaped.
   * Measuring first is what lets all three live in one block.
   */
  fun measureImageOptions(options: StyleImageOptions): Int {
    val stretchX = measureStretches(options.stretchX)
    val stretchY = measureStretches(options.stretchY)
    // Measured and added through the shared arena helpers, so the padding a block leaves behind and
    // a total that would wrap a 32-bit count are accounted for the one way every descriptor here
    // accounts for them.
    return JsonMarshal.plus(
        JsonMarshal.plus(JsonMarshal.measureBlock(MlnStyleImageOptions.SIZEOF), stretchX),
        stretchY,
      )
      .toInt()
  }

  /** Writes [options] into [arena] and returns the descriptor's address. */
  fun writeImageOptions(arena: HeapArena, options: StyleImageOptions): HeapPointer {
    val base = JsonMarshal.allocateBlock(arena, MlnStyleImageOptions.SIZEOF)
    // The leading size field is how the C API versions a descriptor: it carries the size this
    // binding was generated against so native can tell which fields it may read.
    MlnStyleImageOptions.setSize(base, MlnStyleImageOptions.SIZEOF)
    var fields = 0
    options.pixelRatio?.let {
      fields = fields or MlnStyleImageOptionField.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
      MlnStyleImageOptions.setPixelRatio(base, it)
    }
    options.sdf?.let {
      fields = fields or MlnStyleImageOptionField.MLN_STYLE_IMAGE_OPTION_SDF
      MlnStyleImageOptions.setSdf(base, it)
    }
    options.stretchX?.let {
      // An empty list is still a present value: it says the image stretches nowhere horizontally,
      // which is not what leaving the bit clear means.
      fields = fields or MlnStyleImageOptionField.MLN_STYLE_IMAGE_OPTION_STRETCH_X
      MlnStyleImageOptions.setStretchX(base, writeStretches(arena, it))
      MlnStyleImageOptions.setStretchXCount(base, it.size)
    }
    options.stretchY?.let {
      fields = fields or MlnStyleImageOptionField.MLN_STYLE_IMAGE_OPTION_STRETCH_Y
      MlnStyleImageOptions.setStretchY(base, writeStretches(arena, it))
      MlnStyleImageOptions.setStretchYCount(base, it.size)
    }
    options.content?.let {
      fields = fields or MlnStyleImageOptionField.MLN_STYLE_IMAGE_OPTION_CONTENT
      writeImageContent(base + MlnStyleImageOptions.OFFSET_CONTENT, it)
    }
    options.textFitWidth?.let {
      fields = fields or MlnStyleImageOptionField.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH
      MlnStyleImageOptions.setTextFitWidth(base, it.nativeValue)
    }
    options.textFitHeight?.let {
      fields = fields or MlnStyleImageOptionField.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT
      MlnStyleImageOptions.setTextFitHeight(base, it.nativeValue)
    }
    MlnStyleImageOptions.setFields(base, fields)
    return base
  }

  private fun measureStretches(stretches: List<ImageStretch>?): Long =
    if (stretches == null) 0L else JsonMarshal.measureArray(MlnImageStretch.SIZEOF, stretches.size)

  private fun writeStretches(arena: HeapArena, stretches: List<ImageStretch>): HeapPointer {
    val block = JsonMarshal.allocateArray(arena, MlnImageStretch.SIZEOF, stretches.size)
    stretches.forEachIndexed { index, stretch ->
      val entry = block + index * MlnImageStretch.SIZEOF
      MlnImageStretch.setFrom(entry, stretch.from)
      MlnImageStretch.setTo(entry, stretch.to)
    }
    return block
  }

  /**
   * Reads one stretchable interval out of an array native filled.
   *
   * These arrive as a bare array rather than inside a descriptor, so the caller positions the
   * element with [IMAGE_STRETCH_SIZEOF] and this names what the eight bytes hold.
   */
  fun readImageStretch(base: HeapPointer): ImageStretch =
    ImageStretch(MlnImageStretch.from(base), MlnImageStretch.to(base))

  /** Writes a transition header alone, for a buffer native fills. */
  fun writeTransitionOptionsHeader(base: HeapPointer) {
    MlnStyleTransitionOptions.setSize(base, MlnStyleTransitionOptions.SIZEOF)
  }

  /** Writes [options] at [base], setting a field's bit only where the value is present. */
  fun writeTransitionOptions(base: HeapPointer, options: StyleTransitionOptions) {
    // The leading size field is how the C API versions a descriptor: it carries the size this
    // binding was generated against so native can tell which fields it may read.
    MlnStyleTransitionOptions.setSize(base, MlnStyleTransitionOptions.SIZEOF)
    var fields = 0
    options.durationMs?.let {
      fields = fields or MlnStyleTransitionOptionField.MLN_STYLE_TRANSITION_OPTION_DURATION
      MlnStyleTransitionOptions.setDurationMs(base, it)
    }
    options.delayMs?.let {
      fields = fields or MlnStyleTransitionOptionField.MLN_STYLE_TRANSITION_OPTION_DELAY
      MlnStyleTransitionOptions.setDelayMs(base, it)
    }
    options.enablePlacementTransitions?.let {
      // MapLibre Native always holds a value for the cross-fade, so this bit carries the one
      // distinction it cannot: a caller that omitted the field against one that cleared it.
      fields =
        fields or
          MlnStyleTransitionOptionField.MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
      MlnStyleTransitionOptions.setEnablePlacementTransitions(base, it)
    }
    MlnStyleTransitionOptions.setFields(base, fields)
  }

  /** Reads the transition options at [base], producing null for every field whose bit is clear. */
  fun readTransitionOptions(base: HeapPointer): StyleTransitionOptions {
    val fields = MlnStyleTransitionOptions.fields(base)
    fun has(bit: Int) = (fields and bit) != 0
    return StyleTransitionOptions().also {
      if (has(MlnStyleTransitionOptionField.MLN_STYLE_TRANSITION_OPTION_DURATION)) {
        it.durationMs = MlnStyleTransitionOptions.durationMs(base)
      }
      if (has(MlnStyleTransitionOptionField.MLN_STYLE_TRANSITION_OPTION_DELAY)) {
        it.delayMs = MlnStyleTransitionOptions.delayMs(base)
      }
      if (
        has(MlnStyleTransitionOptionField.MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS)
      ) {
        it.enablePlacementTransitions = MlnStyleTransitionOptions.enablePlacementTransitions(base)
      }
    }
  }

  /**
   * Places [image] and its pixels in one scratch block, and calls [body] with the descriptor.
   *
   * The descriptor points at the pixels rather than carrying them, so both live in one block: one
   * acquisition and one release whatever the image measures. The pixels are read out of the Kotlin
   * image once, because that accessor hands back a fresh copy of the whole image on every read.
   */
  fun <T> withImage(image: PremultipliedRgba8Image, body: (HeapPointer) -> T): T {
    val pixels = image.pixels
    // Long arithmetic, because a sum that wrapped a 32-bit count would allocate a small block while
    // the real pixel length still reached the copy below.
    val total = MlnPremultipliedRgba8Image.SIZEOF.toLong() + pixels.size
    Status.requireArgument(total <= Int.MAX_VALUE.toLong()) {
      "image is too large to place in the module's heap"
    }
    return Heap.withScratch(total.toInt()) { base ->
      val storage = base + MlnPremultipliedRgba8Image.SIZEOF
      Heap.storeBytes(storage, pixels)
      MlnPremultipliedRgba8Image.setSize(base, MlnPremultipliedRgba8Image.SIZEOF)
      MlnPremultipliedRgba8Image.setWidth(base, image.width)
      MlnPremultipliedRgba8Image.setHeight(base, image.height)
      MlnPremultipliedRgba8Image.setStride(base, image.stride)
      MlnPremultipliedRgba8Image.setPixels(base, storage)
      MlnPremultipliedRgba8Image.setByteLength(base, pixels.size)
      body(base)
    }
  }

  /** Writes a content box, which carries no field mask of its own. */
  private fun writeImageContent(base: HeapPointer, content: ImageContent) {
    MlnImageContent.setLeft(base, content.left)
    MlnImageContent.setTop(base, content.top)
    MlnImageContent.setRight(base, content.right)
    MlnImageContent.setBottom(base, content.bottom)
  }

  private fun readImageContent(base: HeapPointer): ImageContent =
    ImageContent(
      MlnImageContent.left(base),
      MlnImageContent.top(base),
      MlnImageContent.right(base),
      MlnImageContent.bottom(base),
    )

  /**
   * Widens a `size_t`, which is 32 bits on this target and unsigned.
   *
   * The generated accessor reads it as a signed Int, so a length past two gigabytes would arrive
   * negative and be reported as a negative count.
   */
  private fun size(value: Int): Long = value.toUInt().toLong()
}
