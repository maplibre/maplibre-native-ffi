package org.maplibre.nativeffi.render

/** Borrowed 64-bit Vulkan non-dispatchable handle bit pattern. */
public class VulkanHandle
private constructor(private val bitsValue: Long, private val scope: FrameScope?) {
  /** Native handle bit pattern. Access rejects use after a scoped frame closes. */
  public val bits: Long
    get() {
      ensureActive()
      return bitsValue
    }

  /** Returns true when this value represents `VK_NULL_HANDLE`. */
  public val isNull: Boolean
    get() = bits == 0L

  override fun equals(other: Any?): Boolean {
    ensureActive()
    if (other !is VulkanHandle) return false
    other.ensureActive()
    return bitsValue == other.bitsValue
  }

  override fun hashCode(): Int {
    ensureActive()
    return bitsValue.hashCode()
  }

  override fun toString(): String {
    ensureActive()
    return "VulkanHandle[bits=0x${bitsValue.toULong().toString(16)}]"
  }

  private fun ensureActive() {
    scope?.ensureActive()
  }

  public companion object {
    /** Null Vulkan non-dispatchable handle. */
    public val NULL_HANDLE: VulkanHandle = VulkanHandle(0L, null)

    /**
     * Creates a borrowed Vulkan non-dispatchable handle from its native bit pattern.
     *
     * The caller keeps the Vulkan object valid and synchronized for the full C API borrow window
     * documented by the descriptor that receives this value.
     */
    public fun ofBits(bits: Long): VulkanHandle =
      if (bits == 0L) NULL_HANDLE else VulkanHandle(bits, null)

    internal fun scoped(bits: Long, scope: FrameScope): VulkanHandle = VulkanHandle(bits, scope)
  }
}
