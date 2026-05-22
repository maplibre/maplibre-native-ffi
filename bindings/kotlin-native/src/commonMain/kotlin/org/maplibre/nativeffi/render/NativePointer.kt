package org.maplibre.nativeffi.render

/** Opaque borrowed native address value used for backend interop handles. */
public value class NativePointer(public val address: ULong) {
  /** Returns true when this pointer represents a null backend handle. */
  public val isNull: Boolean
    get() = address == 0UL

  override fun toString(): String = "NativePointer[address=0x${address.toString(16)}]"

  public companion object {
    /** Null native pointer value. */
    public val NULL: NativePointer = NativePointer(0UL)

    /** Creates an opaque borrowed pointer value from an address bit pattern. */
    public fun ofAddress(address: ULong): NativePointer =
      if (address == 0UL) NULL else NativePointer(address)
  }
}
