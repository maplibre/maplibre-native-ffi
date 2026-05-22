package org.maplibre.nativejni.render;

/** Opaque borrowed native address value used for backend interop handles. */
public final class NativePointer {
  public static final NativePointer NULL = new NativePointer(0);

  private final long address;

  private NativePointer(long address) {
    this.address = address;
  }

  public static NativePointer ofAddress(long address) {
    return address == 0 ? NULL : new NativePointer(address);
  }

  public long address() {
    return address;
  }

  public boolean isNull() {
    return address == 0;
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof NativePointer that && address == that.address;
  }

  @Override
  public int hashCode() {
    return Long.hashCode(address);
  }

  @Override
  public String toString() {
    return "NativePointer[address=0x" + Long.toHexString(address) + "]";
  }
}
