package org.maplibre.nativeffi.render;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.lang.ref.Cleaner;

/** Explicit off-heap byte buffer for reusable native readback and upload storage. */
public final class NativeBuffer implements AutoCloseable {
  private static final Cleaner CLEANER = Cleaner.create();

  private final NativeReference nativeReference;
  private final Cleaner.Cleanable cleanable;
  private final MemorySegment segment;
  private final long byteLength;

  private boolean closed;

  private NativeBuffer(long byteLength) {
    if (byteLength < 0) {
      throw new IllegalArgumentException("byteLength must be non-negative");
    }
    var arena = Arena.ofShared();
    this.nativeReference = new NativeReference(arena);
    this.cleanable = CLEANER.register(this, nativeReference);
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
    nativeReference.close();
    cleanable.clean();
  }

  private static final class NativeReference implements Runnable {
    private final Arena arena;
    private boolean closed;

    NativeReference(Arena arena) {
      this.arena = arena;
    }

    synchronized void close() {
      if (!closed) {
        closed = true;
        arena.close();
      }
    }

    @Override
    public void run() {
      close();
    }
  }
}
