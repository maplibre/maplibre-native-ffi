package org.maplibre.nativejni.render;

import java.util.Objects;

/**
 * Opaque borrowed native address value used for backend interop handles.
 *
 * <p>A {@code NativePointer} never owns the pointed-to resource and never grants general memory
 * access. It is only a carrier for backend-native handles accepted by render target descriptors or
 * returned by active texture frames.
 */
public final class NativePointer {
  public static final NativePointer NULL = new NativePointer(0, null);

  private final long address;
  private final FrameScope scope;

  private NativePointer(long address, FrameScope scope) {
    this.address = address;
    this.scope = scope;
  }

  /**
   * Borrows a backend-native address for interop with render target descriptors.
   *
   * <p>The caller keeps the referenced backend object alive and externally synchronized for the
   * full native borrow window documented by the descriptor or operation that receives this pointer.
   * A zero address returns {@link #NULL}.
   */
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
