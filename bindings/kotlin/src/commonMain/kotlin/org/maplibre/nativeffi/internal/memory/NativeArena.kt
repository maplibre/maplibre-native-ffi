package org.maplibre.nativeffi.internal.memory

import org.maplibre.nativeffi.internal.status.Status

/**
 * Native memory for the inputs of one C call, freed together when the call returns.
 *
 * Small allocations share zeroed blocks; a larger one gets a block of its own.
 */
internal open class NativeArena : AutoCloseable {
  private val blocks = ArrayList<Long>(2)
  private var cursor = 0L
  private var limit = 0L

  /** Allocates [size] zeroed bytes aligned to [alignment], which must be a power of two. */
  fun allocate(size: Int, alignment: Int = 8): Long {
    require(size >= 0)
    val start = (cursor + alignment - 1) and (alignment - 1).toLong().inv()
    if (cursor != 0L && start + size <= limit) {
      cursor = start + size
      return start
    }
    if (size > BLOCK_SIZE / 4) return block(maxOf(size.toLong(), 1L))
    val block = block(BLOCK_SIZE)
    cursor = block + size
    limit = block + BLOCK_SIZE
    return block
  }

  private fun block(size: Long): Long = NativeMemory.allocate(size).also { blocks.add(it) }

  /** Copies [value] and returns its address, which is valid even when [value] is empty. */
  fun bytes(value: ByteArray): Long =
    allocate(maxOf(value.size, 1), 1).also {
      if (value.isNotEmpty()) NativeMemory.putBytes(it, value)
    }

  /** Copies [value] as NUL-terminated UTF-8, refusing an embedded NUL that C would truncate at. */
  fun cString(value: String): Long {
    Status.requireArgument('\u0000' !in value) { "text contains an embedded NUL" }
    return bytes(value.encodeToByteArray() + byteArrayOf(0))
  }

  /** Writes an `mln_buffer_view` of [value] at [address]; null leaves the view empty and null. */
  fun putView(address: Long, value: ByteArray?) {
    if (value == null) return
    writeAddress(address, bytes(value))
    writeSize(address + NativeMemory.addressSize, value.size.toULong())
  }

  fun putView(address: Long, value: String?) = putView(address, value?.encodeToByteArray())

  /** Allocates an `mln_buffer_view` of [value] and returns its address. */
  fun view(value: ByteArray?): Long =
    allocate(2 * NativeMemory.addressSize, NativeMemory.addressSize).also { putView(it, value) }

  fun view(value: String?): Long = view(value?.encodeToByteArray())

  /** Writes [items] as a C array of [size]-byte elements and returns its address. */
  inline fun <T> array(items: List<T>, size: Int, alignment: Int, write: (Long, T) -> Unit): Long {
    val result = allocate(maxOf(items.size, 1) * size, alignment)
    items.forEachIndexed { index, item -> write(result + index.toLong() * size, item) }
    return result
  }

  override fun close() {
    blocks.forEach(NativeMemory::free)
    blocks.clear()
    cursor = 0L
    limit = 0L
  }

  private companion object {
    const val BLOCK_SIZE = 1024L
  }
}
