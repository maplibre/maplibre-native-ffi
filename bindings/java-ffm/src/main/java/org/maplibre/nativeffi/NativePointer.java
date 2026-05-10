package org.maplibre.nativeffi;

/** Opaque borrowed native address value used for backend interop handles. */
public record NativePointer(long address) {
  public static final NativePointer NULL = new NativePointer(0);

  public static NativePointer ofAddress(long address) {
    return address == 0 ? NULL : new NativePointer(address);
  }

  public boolean isNull() {
    return address == 0;
  }
}
