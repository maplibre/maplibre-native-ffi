package org.maplibre.nativeffi.internal.memory

import org.maplibre.nativeffi.internal.status.Status

/** Pointers allocated here expire when the C call returns. */
internal open class NativeArena : AutoCloseable {
  private var firstBlock = 0L
  private var blocks: LongArray? = null
  private var blockCount = 0
  private var cursor = 0L
  private var limit = 0L

  fun allocate(size: Int, alignment: Int = 8): Long = allocate(size.toLong(), alignment)

  /** Allocates [size] zeroed bytes aligned to [alignment], which must be a power of two. */
  fun allocate(size: Long, alignment: Int = 8): Long {
    require(size >= 0)
    val start = (cursor + alignment - 1) and (alignment - 1).toLong().inv()
    if (cursor != 0L && start + size <= limit) {
      cursor = start + size
      return start
    }
    if (size > BLOCK_SIZE / 4) return block(maxOf(size, 1L))
    val block = block(BLOCK_SIZE)
    cursor = block + size
    limit = block + BLOCK_SIZE
    return block
  }

  fun sized(size: Int, alignment: Int): Long =
    allocate(size, alignment).also { writeU32(it, size.toUInt()) }

  private fun block(size: Long, zeroed: Boolean = true): Long {
    if (firstBlock != 0L) {
      val existing = blocks
      if (existing == null) blocks = LongArray(2)
      else if (blockCount == existing.size) blocks = existing.copyOf(existing.size * 2)
    }
    val address =
      if (zeroed) NativeMemory.allocate(size) else NativeMemory.allocateUninitialized(size)
    if (firstBlock == 0L) firstBlock = address else requireNotNull(blocks)[blockCount++] = address
    return address
  }

  /** Copies [value] and returns its address, which is valid even when [value] is empty. */
  fun bytes(value: ByteArray): Long =
    (if (value.size > BLOCK_SIZE / 4) block(value.size.toLong(), false)
      else allocate(maxOf(value.size, 1), 1))
      .also {
        if (value.isNotEmpty()) NativeMemory.putBytes(it, value)
      }

  /** Copies [value] as NUL-terminated UTF-8, refusing an embedded NUL that C would truncate at. */
  fun cString(value: String): Long {
    Status.requireArgument('\u0000' !in value) { "text contains an embedded NUL" }
    val encoded = value.encodeToByteArray()
    val size = encoded.size.toLong() + 1
    return (if (size > BLOCK_SIZE / 4) block(size, false) else allocate(size, 1)).also {
      NativeMemory.putBytes(it, encoded)
      NativeMemory.putByte(it + encoded.size, 0)
    }
  }

  fun putView(address: Long, value: ByteArray?) {
    if (value == null) return
    writeAddress(address, bytes(value))
    writeSize(address + NativeMemory.addressSize, value.size.toULong())
  }

  fun putView(address: Long, value: String?) = putView(address, value?.encodeToByteArray())

  fun view(value: ByteArray?): Long =
    allocate(2 * NativeMemory.addressSize, NativeMemory.addressSize).also { putView(it, value) }

  fun view(value: String?): Long = view(value?.encodeToByteArray())

  inline fun <T> array(items: List<T>, size: Int, alignment: Int, write: (Long, T) -> Unit): Long {
    // Two Int factors cannot overflow a Long product.
    val result = allocate(maxOf(items.size, 1).toLong() * size, alignment)
    items.forEachIndexed { index, item -> write(result + index.toLong() * size, item) }
    return result
  }

  override fun close() {
    if (firstBlock != 0L) NativeMemory.free(firstBlock)
    blocks?.let { for (index in 0 until blockCount) NativeMemory.free(it[index]) }
    firstBlock = 0L
    blockCount = 0
    cursor = 0L
    limit = 0L
  }

  private companion object {
    const val BLOCK_SIZE = 1024L
  }
}
