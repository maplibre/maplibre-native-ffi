package org.maplibre.nativeffi.render

/** The JNI shim that graphics_jni.c exports, compiled with graphics.c by the NDK. */
internal object TestGraphicsJni {
  init {
    System.loadLibrary("binding_test_graphics")
  }

  external fun create(backend: Int): Long

  external fun context(graphics: Long): LongArray?

  external fun makeCurrent(graphics: Long): Boolean

  external fun lastError(): String
}

/** tests/graphics over JNI. */
internal actual object TestGraphicsLibrary {
  actual fun create(backend: UInt): Long {
    val graphics = TestGraphicsJni.create(backend.toInt())
    check(graphics != 0L) { "tests/graphics has no context: ${TestGraphicsJni.lastError()}" }
    return graphics
  }

  actual fun context(graphics: Long): TestGraphicsContext {
    val values =
      checkNotNull(TestGraphicsJni.context(graphics)) {
        "tests/graphics context lookup failed: ${TestGraphicsJni.lastError()}"
      }
    return TestGraphicsContext(
      vulkanQueueFamilyIndex = values[0].toUInt(),
      metalDevice = values[1],
      vulkanInstance = values[2],
      vulkanPhysicalDevice = values[3],
      vulkanDevice = values[4],
      vulkanQueue = values[5],
      vulkanGetInstanceProcAddr = values[6],
      vulkanGetDeviceProcAddr = values[7],
      eglDisplay = values[8],
      eglConfig = values[9],
      eglContext = values[10],
    )
  }

  actual fun makeCurrent(graphics: Long) {
    check(TestGraphicsJni.makeCurrent(graphics)) {
      "tests/graphics could not make its context current: ${TestGraphicsJni.lastError()}"
    }
  }
}
