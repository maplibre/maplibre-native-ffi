package org.maplibre.nativeffi.internal.memory

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

  @JvmStatic actual external fun free(address: Long)

  @JvmStatic actual external fun getByte(address: Long): Byte

  @JvmStatic actual external fun putByte(address: Long, value: Byte)

  @JvmStatic actual external fun getShort(address: Long): Short

  @JvmStatic actual external fun putShort(address: Long, value: Short)

  @JvmStatic actual external fun getInt(address: Long): Int

  @JvmStatic actual external fun putInt(address: Long, value: Int)

  @JvmStatic actual external fun getLong(address: Long): Long

  @JvmStatic actual external fun putLong(address: Long, value: Long)

  @JvmStatic actual external fun getFloat(address: Long): Float

  @JvmStatic actual external fun putFloat(address: Long, value: Float)

  @JvmStatic actual external fun getDouble(address: Long): Double

  @JvmStatic actual external fun putDouble(address: Long, value: Double)

  @JvmStatic actual external fun getBytes(address: Long, count: Int): ByteArray

  @JvmStatic actual external fun putBytes(address: Long, value: ByteArray)

  @JvmStatic actual external fun stringLength(address: Long): Long

  @JvmStatic private external fun addressSizeNative(): Int

  @JvmStatic private external fun allocateNative(size: Long): Long
}
