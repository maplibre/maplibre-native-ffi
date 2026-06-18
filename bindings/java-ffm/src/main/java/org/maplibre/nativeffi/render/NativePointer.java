package org.maplibre.nativeffi.render;

import java.util.Objects;

/**
 * Opaque borrowed native address value used for backend interop handles.
 *
 * <p>The pointer transfers no ownership and grants no general memory access. Callers that create a
 * pointer from an address keep the backend object alive and externally synchronized for the full C
 * API borrow window of the render descriptor or active frame that receives it.
 */
public final class NativePointer {
  public static final NativePointer NULL = new NativePointer(0, null);

  private final long address;
  private final FrameScope scope;

  private NativePointer(long address, FrameScope scope) {
    this.address = address;
    this.scope = scope;
  }

  public static NativePointer ofAddress(long address) {
    return address == 0 ? NULL : new NativePointer(address, null);
  }

  static NativePointer scoped(long address, FrameScope scope) {
    return address == 0 ? NULL : new NativePointer(address, Objects.requireNonNull(scope, "scope"));
  }

  public long address() {
    ensureActive();
    return address;
  }

  public boolean isNull() {
    ensureActive();
    return address == 0;
  }

  private void ensureActive() {
    if (scope != null) {
      scope.ensureActive();
    }
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof NativePointer that && address() == that.address();
  }

  @Override
  public int hashCode() {
    return Long.hashCode(address());
  }

  @Override
  public String toString() {
    return "NativePointer[address=0x" + Long.toHexString(address()) + "]";
  }
}
