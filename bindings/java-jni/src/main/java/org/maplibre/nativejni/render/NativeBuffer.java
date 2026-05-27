package org.maplibre.nativejni.render;

import java.nio.ByteBuffer;

/**
 * Explicit off-heap byte buffer for reusable native readback and upload storage.
 *
 * <p>The JNI implementation stores bytes in a direct {@link ByteBuffer}, so capacity is limited to
 * {@link Integer#MAX_VALUE} bytes. Closing the buffer invalidates this wrapper immediately; the JVM
 * releases the direct-buffer memory according to its own buffer and garbage-collection lifecycle.
 */
public final class NativeBuffer implements AutoCloseable {
  private final ByteBuffer buffer;
  private final long byteLength;

  private boolean closed;

  private NativeBuffer(long byteLength) {
    if (byteLength < 0) {
      throw new IllegalArgumentException("byteLength must be non-negative");
    }
    if (byteLength > Integer.MAX_VALUE) {
      throw new IllegalArgumentException("byteLength must be at most Integer.MAX_VALUE");
    }
    this.byteLength = byteLength;
    this.buffer = ByteBuffer.allocateDirect(Math.toIntExact(byteLength));
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
    var bytes = new byte[length];
    var duplicate = buffer.asReadOnlyBuffer();
    duplicate.position(0);
    duplicate.get(bytes);
    return bytes;
  }

  synchronized ByteBuffer borrowBuffer() {
    ensureOpen();
    var duplicate = buffer.duplicate();
    duplicate.position(0);
    return duplicate;
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
    closed = true;
  }
}
