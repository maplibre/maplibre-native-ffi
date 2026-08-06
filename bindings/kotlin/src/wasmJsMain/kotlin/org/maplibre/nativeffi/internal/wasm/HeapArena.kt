package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.internal.status.Status

/**
 * A bump allocator over one scratch block.
 *
 * A geometry descriptor is a tree of spans that point at each other, so placing one means many
 * small allocations that all have to be freed together. Taking one block and carving it up makes
 * that a single acquisition and a single release, and removes any question of which piece frees
 * which.
 *
 * The block is measured before it is taken, so running out is a binding error rather than a
 * partially written descriptor: a caller measures, allocates, and then writes.
 */
internal class HeapArena(private val base: HeapPointer, private val capacity: Int) {
  private var used = 0

  /** Reserves [bytes] aligned to [align], and returns where they start. */
  fun allocate(bytes: Int, align: Int): HeapPointer {
    // A negative count means a caller's own size arithmetic wrapped. Refusing it
    // here matters because the bounds check below only guards the upper end: a
    // negative would move `used` backwards and hand back storage the next write
    // runs past.
    Status.requireArgument(bytes >= 0) { "arena allocation size must be non-negative" }
    // Aligned against the absolute address rather than the offset. An arena
    // whose base is odd of the alignment would otherwise hand back a misaligned
    // descriptor while believing it had aligned one.
    val absolute = base.address.toLong() + used
    val padding = ((align - (absolute % align)) % align).toInt()
    // Long arithmetic throughout, so a size that would wrap a 32-bit count is
    // caught here instead of passing the check and writing past the block.
    val start = used.toLong() + padding
    if (start + bytes.toLong() > capacity.toLong()) {
      throw Status.invalidState(
        "The browser binding measured $capacity bytes for a descriptor and needs more; " +
          "its measure and its write disagree."
      )
    }
    used = (start + bytes).toInt()
    return base + start.toInt()
  }

  internal companion object {
    /**
     * Rounds [bytes] up to the next multiple of [align].
     *
     * Long-valued, because a measure that wrapped a 32-bit count would produce a small positive
     * size, pass the arena's bounds check, and then write the real element count past the block.
     */
    fun aligned(bytes: Long, align: Int): Long = (bytes + align - 1) / align * align
  }
}
