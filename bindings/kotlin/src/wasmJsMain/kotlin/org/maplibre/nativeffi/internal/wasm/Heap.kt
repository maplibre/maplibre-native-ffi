package org.maplibre.nativeffi.internal.wasm

import kotlin.wasm.unsafe.withScopedMemoryAllocator
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
   * Allocates [size] zeroed bytes of Emscripten heap for the body, and frees them afterwards.
   *
   * Descriptors reach native as a pointer to bytes the caller owns, so every call that passes one
   * needs scratch. The C API reads whole descriptors, so the region starts zeroed rather than
   * carrying whatever the allocator last held there. Freeing in a finally block matters more here
   * than it would natively: a browser host cannot restart the process to recover leaked heap.
   */
  fun <T> withScratch(size: Int, body: (HeapPointer) -> T): T {
    Status.requireArgument(size > 0) { "scratch size must be positive" }
    val address = heapAllocate(size)
    if (address == 0) {
      throw Status.invalidState("The MapLibre Native browser module could not allocate $size bytes")
    }
    heapClear(address, size)
    try {
      return body(HeapPointer(address))
    } finally {
      heapFree(address)
    }
  }

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

  fun loadUShort(pointer: HeapPointer): Int = heapLoadUShort(pointer.address)

  fun loadShort(pointer: HeapPointer): Int = heapLoadShort(pointer.address)

  fun storeShort(pointer: HeapPointer, value: Int) {
    heapStoreShort(pointer.address, value)
  }

  fun loadInt(pointer: HeapPointer): Int = heapLoadInt(pointer.address)

  fun storeInt(pointer: HeapPointer, value: Int) {
    heapStoreInt(pointer.address, value)
  }

  fun loadLong(pointer: HeapPointer): Long = heapLoadLong(pointer.address)

  fun storeLong(pointer: HeapPointer, value: Long) {
    heapStoreLong(pointer.address, value)
  }

  fun loadFloat(pointer: HeapPointer): Float = heapLoadFloat(pointer.address)

  fun storeFloat(pointer: HeapPointer, value: Float) {
    heapStoreFloat(pointer.address, value)
  }

  fun loadDouble(pointer: HeapPointer): Double = heapLoadDouble(pointer.address)

  fun storeDouble(pointer: HeapPointer, value: Double) {
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
