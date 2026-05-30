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

  override fun equals(other: Any?): Boolean =
    other is NativePointer && addressValue == other.addressValue

  override fun hashCode(): Int = addressValue.hashCode()

  override fun toString(): String = "NativePointer[address=0x${addressValue.toString(16)}]"

  private fun ensureActive() {
    scope?.ensureActive()
  }

  public companion object {
    /** Null native pointer value. */
    public val NULL: NativePointer = NativePointer(0L, null)

    /** Creates an opaque borrowed pointer value from an address bit pattern. */
    public fun ofAddress(address: Long): NativePointer =
      if (address == 0L) NULL else NativePointer(address, null)

    internal fun scoped(address: Long, scope: FrameScope): NativePointer =
      if (address == 0L) NULL else NativePointer(address, scope)
  }
}
