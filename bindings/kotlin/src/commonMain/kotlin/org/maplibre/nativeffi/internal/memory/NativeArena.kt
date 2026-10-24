package org.maplibre.nativeffi.internal.memory

/** Pointers allocated here expire when the C call returns. */
internal open class NativeArena : AutoCloseable {
  private var firstBlock = 0L
  private var blocks: LongArray? = null
  private var blockCount = 0
  private var cursor = 0L
  private var limit = 0L
  private var scratch: ArenaScratch? = null
  private var scratchChecked = false
  private var largeUsed = false

  private fun scratch(): ArenaScratch? {
    if (!scratchChecked) {
      scratchChecked = true
      val candidate = threadArenaScratch()
      if (!candidate.leased) {
        candidate.leased = true
        scratch = candidate
      }
    }
    return scratch
  }

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
    val block =
      if (cursor == 0L)
        scratch()?.small?.address?.also { NativeMemory.clear(it, BLOCK_SIZE) } ?: block(BLOCK_SIZE)
      else block(BLOCK_SIZE)
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
    (if (value.size > BLOCK_SIZE / 4) byteBlock(value.size) else allocate(maxOf(value.size, 1), 1))
      .also {
        if (value.isNotEmpty()) NativeMemory.putBytes(it, value)
      }

  private fun byteBlock(size: Int): Long {
    if (!largeUsed && size <= 1024 * 1024) {
      scratch()?.let {
        largeUsed = true
        return it.large(size)
      }
    }
    return block(size.toLong(), false)
  }

  /** Copies [value] as NUL-terminated UTF-8, refusing an embedded NUL that C would truncate at. */
  fun cString(value: String): Long {
    require('\u0000' !in value) { "text contains an embedded NUL" }
    val size = value.length.toLong() * 3 + 1
    return (if (size > BLOCK_SIZE / 4) block(size, false) else allocate(size, 1)).also {
      val length = NativeMemory.putUtf8(it, value)
      NativeMemory.putByte(it + length, 0)
    }
  }

  fun putView(address: Long, value: ByteArray?) {
    if (value == null) return
    writeAddress(address, bytes(value))
    writeSize(address + NativeMemory.addressSize, value.size.toULong())
  }

  fun putView(address: Long, value: String?) {
    if (value == null) return
    val capacity = maxOf(value.length.toLong() * 3, 1L)
    val data = if (capacity > BLOCK_SIZE / 4) block(capacity, false) else allocate(capacity, 1)
    writeAddress(address, data)
    writeSize(address + NativeMemory.addressSize, NativeMemory.putUtf8(data, value).toULong())
  }

  fun view(value: ByteArray?): Long =
    allocate(2 * NativeMemory.addressSize, NativeMemory.addressSize).also { putView(it, value) }

  fun view(value: String?): Long =
    allocate(2 * NativeMemory.addressSize, NativeMemory.addressSize).also { putView(it, value) }

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
    largeUsed = false
    scratch?.leased = false
    scratch = null
    scratchChecked = false
  }

  private companion object {
    const val BLOCK_SIZE = 1024L
  }
}
