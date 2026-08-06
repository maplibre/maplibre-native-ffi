package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.generated.MlnJsonArray
import org.maplibre.nativeffi.internal.wasm.generated.MlnJsonMember
import org.maplibre.nativeffi.internal.wasm.generated.MlnJsonObject
import org.maplibre.nativeffi.internal.wasm.generated.MlnJsonValue
import org.maplibre.nativeffi.internal.wasm.generated.MlnJsonValueType
import org.maplibre.nativeffi.internal.wasm.generated.MlnStringView
import org.maplibre.nativeffi.json.JsonValue

/**
 * Places a [JsonValue] tree into the Emscripten heap, and reads one back.
 *
 * The C descriptor is a tagged union whose array and object arms point at further descriptors, so
 * the same approach [GeometryMarshal] takes applies here: measure the tree, place it in one arena,
 * and hand native a single root pointer. Measuring and writing walk the same shape in the same
 * order, and each pair sits together so that a change to one is visible against the other.
 *
 * The arena arithmetic below is shared with [GeoJsonMarshal], which places the same member arrays
 * and string views inside features. One copy of the checked arithmetic is deliberate: a second copy
 * is a second place for an unchecked subtotal to appear.
 */
internal object JsonMarshal {
  /**
   * Alignment every block in one of these trees is placed and measured at.
   *
   * One width for all of them rather than each struct's own, because a member is twelve bytes and a
   * key is any length: a member array followed by a value array would need padding that a measure
   * working from sizes alone cannot see. Placing every block at the widest alignment these
   * descriptors ask for makes a measured size exactly the space the write consumes, as long as the
   * arena's own base is at least this aligned — which the module's allocator guarantees.
   */
  private const val BLOCK_ALIGN = 8

  /** Adds two measured sizes, refusing a total the heap could not address. */
  fun plus(left: Long, right: Long): Long {
    val total = left + right
    if (total > Int.MAX_VALUE || total < 0) {
      throw Status.invalidArgument("descriptor tree is too large to place in the module's heap")
    }
    return total
  }

  /** Bytes one block of [bytes] occupies, including the padding that follows it. */
  fun measureBlock(bytes: Int): Long = plus(HeapArena.aligned(bytes.toLong(), BLOCK_ALIGN), 0)

  /** Reserves the block [measureBlock] accounted for. */
  fun allocateBlock(arena: HeapArena, bytes: Int): HeapPointer = arena.allocate(bytes, BLOCK_ALIGN)

  /** Bytes an array of [count] elements occupies, including the padding that follows it. */
  fun measureArray(elementBytes: Int, count: Int): Long =
    measureBlock(Heap.sizeOf(elementBytes, count))

  /** Reserves the array [measureArray] accounted for, and returns where it starts. */
  fun allocateArray(arena: HeapArena, elementBytes: Int, count: Int): HeapPointer =
    allocateBlock(arena, Heap.sizeOf(elementBytes, count))

  /** Bytes the storage behind a string view of [text] occupies. */
  fun measureText(text: String): Long = measureBlock(Heap.utf8Size(text))

  /** Writes the string view at [view], with the bytes it points at placed in [arena]. */
  fun writeText(arena: HeapArena, view: HeapPointer, text: String) {
    val bytes = Heap.utf8Size(text)
    val storage = allocateBlock(arena, bytes)
    Heap.storeUtf8(storage, text)
    MlnStringView.setData(view, storage)
    // The view's size excludes the terminator the writer above added: native reads the run the view
    // describes rather than scanning for a null, and the terminator is there only because the
    // module's own UTF-8 writer emits one.
    MlnStringView.setSize(view, bytes - 1)
  }

  /**
   * Reads the string view at [view].
   *
   * A view is a pointer and a length, so the length is what bounds the read. Native is free to
   * point one into the middle of a buffer it owns, where scanning for a terminator would run past
   * the text and into whatever follows it.
   */
  fun readText(view: HeapPointer): String {
    val data = MlnStringView.data(view)
    val size = readCount(MlnStringView.size(view))
    if (size == 0 || data.address == 0) return ""
    return Heap.loadBytes(data, size).decodeToString()
  }

  /** Bytes [value] needs, including its root descriptor. */
  fun measure(value: JsonValue): Int = measureValue(value, 0).toInt()

  /** Writes [value] into [arena] and returns the root descriptor's address. */
  fun write(arena: HeapArena, value: JsonValue): HeapPointer = writeValue(arena, value, 0)

  /** Bytes a descriptor for [value] and everything below it occupy, placed at [depth]. */
  fun measureValue(value: JsonValue, depth: Int): Long =
    plus(measureBlock(MlnJsonValue.SIZEOF), measurePayload(value, depth))

  /** Places a descriptor for [value] and everything below it, and returns its address. */
  fun writeValue(arena: HeapArena, value: JsonValue, depth: Int): HeapPointer {
    val base = allocateBlock(arena, MlnJsonValue.SIZEOF)
    writeInto(arena, base, value, depth)
    return base
  }

  /**
   * Bytes [value] needs below its own descriptor.
   *
   * Every addition and every element count is bounded as it is taken rather than only at the end. A
   * subtotal that wrapped would produce a small positive size that passed both this check and the
   * arena's, and the write that followed would run past the block.
   */
  private fun measurePayload(value: JsonValue, depth: Int): Long {
    requireDepth(depth)
    return when (value) {
      // Scalars live in the descriptor's own union arm, so they need no storage of their own.
      JsonValue.Null -> 0L
      is JsonValue.Bool -> 0L
      is JsonValue.UInt -> 0L
      is JsonValue.Int -> 0L
      is JsonValue.DoubleValue -> 0L
      is JsonValue.StringValue -> measureText(value.value)
      is JsonValue.Array ->
        value.values.fold(measureArray(MlnJsonValue.SIZEOF, value.values.size)) { total, child ->
          plus(total, measurePayload(child, depth + 1))
        }
      is JsonValue.ObjectValue -> measureMembers(value.members, depth + 1)
      // A value read back from a native tag this binding did not recognise. Its shape is unknown,
      // so there is nothing to measure and nothing that could be written back.
      is JsonValue.Unknown -> throw unknownValue(value)
    }
  }

  private fun writeInto(arena: HeapArena, base: HeapPointer, value: JsonValue, depth: Int) {
    requireDepth(depth)
    // The leading size field is how the C API versions a descriptor: it carries the size this
    // binding was generated against so native can tell which fields it may read.
    MlnJsonValue.setSize(base, MlnJsonValue.SIZEOF)
    val data = base + MlnJsonValue.OFFSET_DATA
    when (value) {
      JsonValue.Null -> MlnJsonValue.setType(base, MlnJsonValueType.MLN_JSON_VALUE_TYPE_NULL)
      is JsonValue.Bool -> {
        MlnJsonValue.setType(base, MlnJsonValueType.MLN_JSON_VALUE_TYPE_BOOL)
        // The arm is a C `bool`, which is one byte on this target rather than the union's width.
        Heap.storeByte(data, if (value.value) 1.toByte() else 0.toByte())
      }
      is JsonValue.UInt -> {
        MlnJsonValue.setType(base, MlnJsonValueType.MLN_JSON_VALUE_TYPE_UINT)
        // Carried as the bit pattern it was read as. The C arm is unsigned and Kotlin's Long
        // is not, so reinterpreting here would change the value rather than preserve it.
        Heap.storeLong(data, value.value)
      }
      is JsonValue.Int -> {
        MlnJsonValue.setType(base, MlnJsonValueType.MLN_JSON_VALUE_TYPE_INT)
        Heap.storeLong(data, value.value)
      }
      is JsonValue.DoubleValue -> {
        MlnJsonValue.setType(base, MlnJsonValueType.MLN_JSON_VALUE_TYPE_DOUBLE)
        Heap.storeDouble(data, value.value)
      }
      is JsonValue.StringValue -> {
        MlnJsonValue.setType(base, MlnJsonValueType.MLN_JSON_VALUE_TYPE_STRING)
        writeText(arena, data, value.value)
      }
      is JsonValue.Array -> {
        MlnJsonValue.setType(base, MlnJsonValueType.MLN_JSON_VALUE_TYPE_ARRAY)
        val values = allocateArray(arena, MlnJsonValue.SIZEOF, value.values.size)
        value.values.forEachIndexed { index, child ->
          writeInto(arena, values + index * MlnJsonValue.SIZEOF, child, depth + 1)
        }
        MlnJsonArray.setValues(data, values)
        MlnJsonArray.setValueCount(data, value.values.size)
      }
      is JsonValue.ObjectValue -> {
        MlnJsonValue.setType(base, MlnJsonValueType.MLN_JSON_VALUE_TYPE_OBJECT)
        MlnJsonObject.setMembers(data, writeMembers(arena, value.members, depth + 1))
        MlnJsonObject.setMemberCount(data, value.members.size)
      }
      is JsonValue.Unknown -> throw unknownValue(value)
    }
  }

  /**
   * Bytes a member array and everything below it occupy, with the member values at [depth].
   *
   * A member holds its value by pointer rather than in place, so each one costs a descriptor of its
   * own on top of the array. Feature properties are the same array with the same shape, which is
   * why this is reachable from [GeoJsonMarshal] rather than folded into the object arm.
   */
  fun measureMembers(members: List<JsonValue.Member>, depth: Int): Long =
    members.fold(measureArray(MlnJsonMember.SIZEOF, members.size)) { total, member ->
      plus(plus(total, measureText(member.key)), measureValue(member.value, depth))
    }

  /** Writes a member array and returns where it starts. */
  fun writeMembers(arena: HeapArena, members: List<JsonValue.Member>, depth: Int): HeapPointer {
    val block = allocateArray(arena, MlnJsonMember.SIZEOF, members.size)
    members.forEachIndexed { index, member ->
      val entry = block + index * MlnJsonMember.SIZEOF
      writeText(arena, entry + MlnJsonMember.OFFSET_KEY, member.key)
      MlnJsonMember.setValue(entry, writeValue(arena, member.value, depth))
    }
    return block
  }

  /** Reads the value descriptor at [base], copying every string into Kotlin as it goes. */
  fun read(base: HeapPointer): JsonValue = readValue(base, 0)

  private fun readValue(base: HeapPointer, depth: Int): JsonValue {
    requireDepth(depth)
    val data = base + MlnJsonValue.OFFSET_DATA
    return when (MlnJsonValue.type(base)) {
      MlnJsonValueType.MLN_JSON_VALUE_TYPE_NULL -> JsonValue.Null
      MlnJsonValueType.MLN_JSON_VALUE_TYPE_BOOL -> JsonValue.Bool(Heap.loadByte(data) != 0.toByte())
      MlnJsonValueType.MLN_JSON_VALUE_TYPE_UINT -> JsonValue.UInt(Heap.loadLong(data))
      MlnJsonValueType.MLN_JSON_VALUE_TYPE_INT -> JsonValue.Int(Heap.loadLong(data))
      MlnJsonValueType.MLN_JSON_VALUE_TYPE_DOUBLE -> JsonValue.DoubleValue(Heap.loadDouble(data))
      MlnJsonValueType.MLN_JSON_VALUE_TYPE_STRING -> JsonValue.StringValue(readText(data))
      MlnJsonValueType.MLN_JSON_VALUE_TYPE_ARRAY -> {
        val values = MlnJsonArray.values(data)
        JsonValue.Array(
          List(readCount(MlnJsonArray.valueCount(data))) { index ->
            readValue(values + index * MlnJsonValue.SIZEOF, depth + 1)
          }
        )
      }
      MlnJsonValueType.MLN_JSON_VALUE_TYPE_OBJECT ->
        JsonValue.ObjectValue(
          readMembers(
            MlnJsonObject.members(data),
            readCount(MlnJsonObject.memberCount(data)),
            depth + 1,
          )
        )
      // A tag from a newer C API than this binding was generated against. The value is kept rather
      // than rejected so a caller can still see the rest of the tree it arrived in.
      else -> JsonValue.Unknown(MlnJsonValue.type(base), MlnJsonValue.size(base))
    }
  }

  /** Reads [count] members starting at [members], with their values at [depth]. */
  fun readMembers(members: HeapPointer, count: Int, depth: Int): List<JsonValue.Member> =
    List(count) { index ->
      val entry = members + index * MlnJsonMember.SIZEOF
      JsonValue.Member(
        readText(entry + MlnJsonMember.OFFSET_KEY),
        readValue(MlnJsonMember.value(entry), depth),
      )
    }

  /**
   * Refuses a tree deeper than the C API accepts.
   *
   * Checked before recursing rather than left to native: the walks above would otherwise descend an
   * over-deep tree first, and a deep enough one exhausts this module's stack before native ever
   * sees it. The read path is bounded for the same reason, since the descriptor it walks is only as
   * trustworthy as the module that produced it.
   */
  private fun requireDepth(depth: Int) {
    if (depth > JsonValue.MAX_DESCRIPTOR_DEPTH) {
      throw Status.invalidArgument(
        "JSON value nests deeper than the ${JsonValue.MAX_DESCRIPTOR_DEPTH} levels the C API accepts"
      )
    }
  }

  /**
   * Refuses a count or length native reported that no real descriptor could carry.
   *
   * `size_t` is 32 bits on this target, so a value past [Int.MAX_VALUE] arrives negative. The heap
   * could not hold a descriptor that large, so a negative one means the address being read is not
   * the descriptor it was taken for, and continuing would index arbitrary memory.
   */
  private fun readCount(count: Int): Int {
    if (count < 0) {
      throw Status.invalidState(
        "The MapLibre Native browser module reported a descriptor count of $count"
      )
    }
    return count
  }

  private fun unknownValue(value: JsonValue.Unknown) =
    Status.invalidArgument(
      "A JSON value of unknown native type ${value.rawType} cannot be sent to native; it was read " +
        "from a tag this binding does not recognise."
    )
}
