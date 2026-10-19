package org.maplibre.nativeffi.internal.memory

import dalvik.annotation.optimization.CriticalNative
import org.maplibre.nativeffi.internal.c.Jni

/** Native memory through the JNI shim's accessors in `src/androidMain/jni/mln_jni.c`. */
internal actual object NativeMemory {
  init {
    Jni.load()
  }

  actual val addressSize: Int = addressSizeNative()

  actual fun allocate(size: Long): Long =
    allocateNative(size).also {
      if (it == 0L) throw OutOfMemoryError("cannot allocate $size bytes of native memory")
    }

  @CriticalNative @JvmStatic actual external fun free(address: Long)

  @CriticalNative @JvmStatic actual external fun clear(address: Long, size: Long)

  actual fun allocateUninitialized(size: Long): Long =
    allocateUninitializedNative(size).also {
      if (it == 0L) throw OutOfMemoryError("cannot allocate $size bytes of native memory")
    }

  @CriticalNative @JvmStatic private external fun allocateUninitializedNative(size: Long): Long

  @CriticalNative @JvmStatic actual external fun getByte(address: Long): Byte

  @CriticalNative @JvmStatic actual external fun putByte(address: Long, value: Byte)

  @CriticalNative @JvmStatic actual external fun getShort(address: Long): Short

  @CriticalNative @JvmStatic actual external fun putShort(address: Long, value: Short)

  @CriticalNative @JvmStatic actual external fun getInt(address: Long): Int

  @CriticalNative @JvmStatic actual external fun putInt(address: Long, value: Int)

  @CriticalNative @JvmStatic actual external fun getLong(address: Long): Long

  @CriticalNative @JvmStatic actual external fun putLong(address: Long, value: Long)

  @CriticalNative @JvmStatic actual external fun getFloat(address: Long): Float

  @CriticalNative @JvmStatic actual external fun putFloat(address: Long, value: Float)

  @CriticalNative @JvmStatic actual external fun getDouble(address: Long): Double

  @CriticalNative @JvmStatic actual external fun putDouble(address: Long, value: Double)

  @JvmStatic actual external fun getBytes(address: Long, count: Int): ByteArray

  @JvmStatic actual external fun putBytes(address: Long, value: ByteArray)

  @JvmStatic actual external fun putUtf8(address: Long, value: String): Long

  @CriticalNative @JvmStatic actual external fun stringLength(address: Long): Long

  @CriticalNative @JvmStatic private external fun addressSizeNative(): Int

  @CriticalNative @JvmStatic private external fun allocateNative(size: Long): Long
}
