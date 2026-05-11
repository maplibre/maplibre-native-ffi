package org.maplibre.nativeffi.render;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;

/** Explicit off-heap byte buffer for reusable native readback and upload storage. */
public final class NativeBuffer implements AutoCloseable {
  private final Arena arena;
  private final MemorySegment segment;
  private final long byteLength;

  private boolean closed;

  private NativeBuffer(long byteLength) {
    if (byteLength < 0) {
      throw new IllegalArgumentException("byteLength must be non-negative");
    }
    this.arena = Arena.ofShared();
    this.byteLength = byteLength;
    this.segment = byteLength == 0 ? MemorySegment.NULL : arena.allocate(byteLength);
  }

  public static NativeBuffer allocate(long byteLength) {
    return new NativeBuffer(byteLength);
  }

  public synchronized long byteLength() {
    ensureOpen();
    return byteLength;
  }

  public synchronized byte[] toByteArray() {
    ensureOpen();
    var length = Math.toIntExact(byteLength);
    if (length == 0) {
      return new byte[0];
    }
    return segment.toArray(ValueLayout.JAVA_BYTE);
  }

  synchronized MemorySegment segment() {
    ensureOpen();
    return segment;
  }

  synchronized void ensureCapacity(long requiredBytes) {
    ensureOpen();
    if (byteLength < requiredBytes) {
      throw new IllegalArgumentException("buffer is smaller than required byte length");
    }
  }

  private void ensureOpen() {
    if (closed) {
      throw new IllegalStateException("NativeBuffer is already closed");
    }
  }

  @Override
  public synchronized void close() {
    if (closed) {
      return;
    }
    closed = true;
    arena.close();
  }
}
