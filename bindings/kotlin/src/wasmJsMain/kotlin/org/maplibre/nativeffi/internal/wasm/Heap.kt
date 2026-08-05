package org.maplibre.nativeffi.internal.wasm

import kotlin.wasm.unsafe.withScopedMemoryAllocator
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.internal.status.Status

/**
 * The Emscripten module's linear memory, which is not this module's.
 *
 * A WebAssembly module cannot address another module's memory, so every descriptor, string, and
 * pixel buffer crosses through JavaScript. The cost that matters is the number of crossings, not
 * the number of bytes: a typed-array element assignment is one crossing each, so copying an image a
 * byte at a time costs one call per byte.
 *
 * The way out is that this module's own memory *is* reachable from JavaScript. Kotlin/Wasm exports
 * it, and the compiler places `wasmExports` in scope for the snippets below precisely so externals
 * can reach the instance. So a bulk transfer stages the bytes in this module's linear memory with
 * ordinary wasm stores, then copies the whole run across in one call.
 *
 * The module is linked without memory growth, so a heap view stays valid for the module's life.
 * Adding growth later would mean re-reading `HEAPU8` on every access instead of holding a view.
 *
 * These are top-level because `@JsFun` may only implement a top-level external function.
 */
@JsFun("(size) => globalThis.__maplibreNativeC._malloc(size)")
private external fun heapAllocate(size: Int): Int

@JsFun("(address) => globalThis.__maplibreNativeC._free(address)")
private external fun heapFree(address: Int)

// A `double` rather than an `int`, because a module linked at Emscripten's 2 GiB maximum has a heap
// length no Int holds, and a length that came back negative would make the bound it feeds accept
// every request instead of refusing the ones past the heap.
@JsFun("() => globalThis.__maplibreNativeC.HEAPU8.length")
private external fun heapByteLength(): Double

@JsFun(
  "(address, length) => { globalThis.__maplibreNativeC.HEAPU8.fill(0, address, address + length) }"
)
private external fun heapClear(address: Int, length: Int)

@JsFun("(text) => globalThis.__maplibreNativeC.lengthBytesUTF8(text)")
private external fun heapUtf8Length(text: String): Int

@JsFun(
  "(text, address, capacity) => { globalThis.__maplibreNativeC.stringToUTF8(text, address, capacity) }"
)
private external fun heapWriteUtf8(text: String, address: Int, capacity: Int)

@JsFun("(address) => globalThis.__maplibreNativeC.UTF8ToString(address)")
private external fun heapReadUtf8(address: Int): String

@JsFun("(address) => globalThis.__maplibreNativeC.HEAPU8[address]")
private external fun heapLoadByte(address: Int): Int

@JsFun("(address, value) => { globalThis.__maplibreNativeC.HEAPU8[address] = value }")
private external fun heapStoreByte(address: Int, value: Int)

@JsFun("(address) => globalThis.__maplibreNativeC.HEAPU16[address >>> 1]")
private external fun heapLoadUShort(address: Int): Int

@JsFun("(address) => (globalThis.__maplibreNativeC.HEAPU16[address >>> 1] << 16) >> 16")
private external fun heapLoadShort(address: Int): Int

@JsFun("(address, value) => { globalThis.__maplibreNativeC.HEAPU16[address >>> 1] = value }")
private external fun heapStoreShort(address: Int, value: Int)

@JsFun("(address) => globalThis.__maplibreNativeC.HEAPU32[address >>> 2] | 0")
private external fun heapLoadInt(address: Int): Int

@JsFun("(address, value) => { globalThis.__maplibreNativeC.HEAPU32[address >>> 2] = value }")
private external fun heapStoreInt(address: Int, value: Int)

@JsFun("(address) => globalThis.__maplibreNativeC.HEAPF32[address >>> 2]")
private external fun heapLoadFloat(address: Int): Float

@JsFun("(address, value) => { globalThis.__maplibreNativeC.HEAPF32[address >>> 2] = value }")
private external fun heapStoreFloat(address: Int, value: Float)

@JsFun("(address) => globalThis.__maplibreNativeC.HEAPF64[address >>> 3]")
private external fun heapLoadDouble(address: Int): Double

@JsFun("(address, value) => { globalThis.__maplibreNativeC.HEAPF64[address >>> 3] = value }")
private external fun heapStoreDouble(address: Int, value: Double)

// A handle is a 64-bit generational identifier whose kind occupies the top byte, so it exceeds the
// range a JavaScript number represents exactly. It crosses as a BigInt, which is what the module's
// own i64 interface expects, and which Kotlin maps to Long.
@JsFun("(address) => new BigInt64Array(globalThis.__maplibreNativeC.HEAPU8.buffer)[address >>> 3]")
private external fun heapLoadLong(address: Int): Long

@JsFun(
  "(address, value) => { new BigInt64Array(globalThis.__maplibreNativeC.HEAPU8.buffer)[address >>> 3] = value }"
)
private external fun heapStoreLong(address: Int, value: Long)

/** Copies a run out of this module's memory into the Emscripten heap, in one crossing. */
@JsFun(
  """
  (source, destination, length) => {
    globalThis.__maplibreNativeC.HEAPU8.set(
      new Uint8Array(wasmExports.memory.buffer, source, length), destination)
  }
"""
)
private external fun heapCopyIn(source: Int, destination: Int, length: Int)

/** Copies a run out of the Emscripten heap into this module's memory, in one crossing. */
@JsFun(
  """
  (source, destination, length) => {
    new Uint8Array(wasmExports.memory.buffer, destination, length).set(
      globalThis.__maplibreNativeC.HEAPU8.subarray(source, source + length))
  }
"""
)
private external fun heapCopyOut(source: Int, destination: Int, length: Int)

/** An address in the Emscripten heap. Pointers are 32-bit on this target; handles are not. */
internal value class HeapPointer(val address: Int) {
  operator fun plus(offset: Int): HeapPointer = HeapPointer(address + offset)
}

internal object Heap {
  /**
   * Refuses an address a typed-array view would not read where it was asked to.
   *
   * The accessors below index a view by element rather than by byte, because `HEAPF64[address >>>
   * 3]` is one shift where a `DataView` call is a method dispatch. The shift discards the low bits,
   * so a misaligned address does not read slowly — it reads a *different* address, and the value
   * that comes back belongs to whatever the neighbouring field is. That failure is silent and it is
   * not local: the descriptor still parses, and the wrong value only surfaces somewhere far from
   * the marshaller that misplaced it.
   *
   * Every descriptor these accessors reach is aligned by the C ABI already, so a violation here is
   * a marshalling bug rather than a caller's mistake. It is checked rather than assumed because the
   * one that was found — a `size_t` packed ahead of a struct in a shared scratch block — looked
   * correct in the code that wrote it.
   */
  private fun requireAligned(pointer: HeapPointer, width: Int) {
    if (pointer.address and (width - 1) != 0) {
      throw Status.invalidState(
        "A $width-byte field was placed at address ${pointer.address}, which is not $width-byte " +
          "aligned; the descriptor holding it is laid out wrongly."
      )
    }
  }

  /**
   * Allocates [size] zeroed bytes of Emscripten heap for the body, and frees them afterwards.
   *
   * Descriptors reach native as a pointer to bytes the caller owns, so every call that passes one
   * needs scratch. The C API reads whole descriptors, so the region starts zeroed rather than
   * carrying whatever the allocator last held there. Freeing in a finally block matters more here
   * than it would natively: a browser host cannot restart the process to recover leaked heap.
   */
  fun <T> withScratch(size: Int, body: (HeapPointer) -> T): T {
    Status.requireArgument(size > 0) { "scratch size must be positive" }
    // Asked before the allocator is, because this is the first thing most calls into the binding
    // touch. A final shutdown releases the module, and every accessor here reaches it through a
    // page global that is then null -- so without this, a call after a shutdown reports a
    // JavaScript type error naming `_malloc` rather than the binding failure that says what
    // happened. The check is a global read, which is nothing beside the allocation it guards.
    BrowserModule.require()
    val address = heapAllocate(size)
    // Reachable, which it was not always: the module is linked with `-sABORTING_MALLOC=0`, so an
    // exhausted heap returns null here rather than aborting the module out from under the page. The
    // size is not checked against the heap first, the way a caller's own buffer length is, because
    // that would put a boundary crossing on the path every call into this binding takes to say what
    // the allocator is about to say anyway.
    if (address == 0) throw allocationFailure(size)
    heapClear(address, size)
    try {
      return body(HeapPointer(address))
    } finally {
      heapFree(address)
    }
  }

  /**
   * The failure an acquisition of [size] bytes reports when the module's allocator refuses.
   *
   * Named rather than thrown inline because three places raise it: this file's scratch, a
   * [org.maplibre.nativeffi.render.NativeBuffer] a host asked for, and [InjectedFaults], which has
   * to produce the error a real failure would. A caller cannot tell the three apart, and three
   * spellings of one failure would drift the first time any of them was reworded.
   */
  fun allocationFailure(size: Int): MaplibreException =
    Status.invalidState("The MapLibre Native browser module could not allocate $size bytes")

  /**
   * The whole of the module's linear memory, in bytes.
   *
   * A ceiling rather than a reading. The heap is fixed at link time, so a request larger than this
   * cannot be served however empty the heap is, and no amount of freeing would change that; a
   * request smaller than it may still fail, because the same memory holds the module's code, its
   * threads' stacks, and everything already allocated. So this bounds what a caller asks for and
   * sizes nothing.
   */
  fun byteLength(): Long = heapByteLength().toLong()

  /**
   * Sizes an array of [count] elements, refusing one this target could not address.
   *
   * A pointer is 32 bits here, so an element count large enough to wrap the product would produce a
   * small positive size: the scratch would be allocated, the real count would still be handed to
   * native, and native would read past the block.
   */
  fun sizeOf(elementBytes: Int, count: Int): Int {
    Status.requireArgument(count >= 0) { "element count must be non-negative" }
    val bytes = elementBytes.toLong() * count
    Status.requireArgument(bytes <= Int.MAX_VALUE) {
      "$count elements of $elementBytes bytes cannot be addressed on this target"
    }
    return bytes.toInt()
  }

  /**
   * Rejects a string C would truncate when it is passed as null-terminated text.
   *
   * Only for arguments that cross as a bare `const char*`. A `mln_string_view` carries its own
   * length, so an embedded NUL is ordinary content there and must not be refused; a null-terminated
   * argument would instead be silently cut at the first one, and native would act on a prefix the
   * caller never asked for.
   */
  fun requireCString(value: String, subject: String) {
    Status.requireArgument('\u0000' !in value) { "$subject cannot contain embedded NUL characters" }
  }

  /** Bytes a null-terminated copy of [text] occupies, including the terminator. */
  fun utf8Size(text: String): Int = heapUtf8Length(text) + 1

  /**
   * Writes [text] at [pointer] as null-terminated UTF-8.
   *
   * The caller sizes the region with [utf8Size]; the module's own writer handles the encoding, so
   * nothing here re-implements it.
   */
  fun storeUtf8(pointer: HeapPointer, text: String) {
    heapWriteUtf8(text, pointer.address, utf8Size(text))
  }

  /** Reads a null-terminated UTF-8 string, copying it into Kotlin before it can be invalidated. */
  fun loadUtf8(pointer: HeapPointer): String =
    if (pointer.address == 0) "" else heapReadUtf8(pointer.address)

  fun loadByte(pointer: HeapPointer): Byte = heapLoadByte(pointer.address).toByte()

  fun storeByte(pointer: HeapPointer, value: Byte) {
    heapStoreByte(pointer.address, value.toInt() and 0xFF)
  }

  fun loadUShort(pointer: HeapPointer): Int {
    requireAligned(pointer, 2)
    return heapLoadUShort(pointer.address)
  }

  fun loadShort(pointer: HeapPointer): Int {
    requireAligned(pointer, 2)
    return heapLoadShort(pointer.address)
  }

  fun storeShort(pointer: HeapPointer, value: Int) {
    requireAligned(pointer, 2)
    heapStoreShort(pointer.address, value)
  }

  fun loadInt(pointer: HeapPointer): Int {
    requireAligned(pointer, 4)
    return heapLoadInt(pointer.address)
  }

  fun storeInt(pointer: HeapPointer, value: Int) {
    requireAligned(pointer, 4)
    heapStoreInt(pointer.address, value)
  }

  fun loadLong(pointer: HeapPointer): Long {
    requireAligned(pointer, 8)
    return heapLoadLong(pointer.address)
  }

  fun storeLong(pointer: HeapPointer, value: Long) {
    requireAligned(pointer, 8)
    heapStoreLong(pointer.address, value)
  }

  fun loadFloat(pointer: HeapPointer): Float {
    requireAligned(pointer, 4)
    return heapLoadFloat(pointer.address)
  }

  fun storeFloat(pointer: HeapPointer, value: Float) {
    requireAligned(pointer, 4)
    heapStoreFloat(pointer.address, value)
  }

  fun loadDouble(pointer: HeapPointer): Double {
    requireAligned(pointer, 8)
    return heapLoadDouble(pointer.address)
  }

  fun storeDouble(pointer: HeapPointer, value: Double) {
    requireAligned(pointer, 8)
    heapStoreDouble(pointer.address, value)
  }

  /**
   * Writes [bytes] into the Emscripten heap at [pointer] using one boundary crossing.
   *
   * The staging loop runs entirely inside this module, so it costs wasm stores rather than
   * JavaScript calls; only [heapCopyIn] crosses.
   */
  fun storeBytes(pointer: HeapPointer, bytes: ByteArray) {
    if (bytes.isEmpty()) return
    withScopedMemoryAllocator { allocator ->
      val staging = allocator.allocate(bytes.size)
      for (index in bytes.indices) (staging + index).storeByte(bytes[index])
      heapCopyIn(staging.address.toInt(), pointer.address, bytes.size)
    }
  }

  /** Reads [length] bytes from the Emscripten heap at [pointer] using one boundary crossing. */
  fun loadBytes(pointer: HeapPointer, length: Int): ByteArray {
    Status.requireArgument(length >= 0) { "length must be non-negative" }
    if (length == 0) return ByteArray(0)
    val bytes = ByteArray(length)
    withScopedMemoryAllocator { allocator ->
      val staging = allocator.allocate(length)
      heapCopyOut(pointer.address, staging.address.toInt(), length)
      for (index in 0 until length) bytes[index] = (staging + index).loadByte()
    }
    return bytes
  }
}
