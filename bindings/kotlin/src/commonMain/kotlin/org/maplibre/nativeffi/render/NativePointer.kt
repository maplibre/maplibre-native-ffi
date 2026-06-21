package org.maplibre.nativeffi.render

/** Opaque borrowed native address value used for backend interop handles. */
public class NativePointer
private constructor(private val addressValue: Long, private val scope: FrameScope?) {
  /** Borrowed address bit pattern. Access rejects use after a scoped frame closes. */
  public val address: Long
    get() {
      ensureActive()
      return addressValue
    }

  /** Returns true when this pointer represents a null backend handle. */
  public val isNull: Boolean
    get() {
      ensureActive()
      return addressValue == 0L
    }

  override fun equals(other: Any?): Boolean {
    ensureActive()
    if (other !is NativePointer) return false
    other.ensureActive()
    return addressValue == other.addressValue
  }

  override fun hashCode(): Int {
    ensureActive()
    return addressValue.hashCode()
  }

  override fun toString(): String {
    ensureActive()
    return "NativePointer[address=0x${addressValue.toString(16)}]"
  }

  private fun ensureActive() {
    scope?.ensureActive()
  }

  public companion object {
    /** Null native pointer value. */
    public val NULL: NativePointer = NativePointer(0L, null)

    /**
     * Creates an opaque borrowed backend interop pointer from an address bit pattern.
     *
     * The caller retains ownership of the pointed-to backend resource and keeps it valid and
     * synchronized for the full C API borrow window documented by the descriptor that receives this
     * value. This value grants no general memory access.
     */
    public fun ofAddress(address: Long): NativePointer =
      if (address == 0L) NULL else NativePointer(address, null)

    internal fun scoped(address: Long, scope: FrameScope): NativePointer =
      if (address == 0L) NULL else NativePointer(address, scope)
  }
}
