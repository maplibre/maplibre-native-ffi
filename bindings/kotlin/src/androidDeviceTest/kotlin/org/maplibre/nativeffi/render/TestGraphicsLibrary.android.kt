package org.maplibre.nativeffi.render

/**
 * The JNI shim that graphics_jni.c exports, compiled with graphics.c by the NDK. It also calls the
 * binding's completion function pointers, as native would.
 */
internal object TestGraphicsJni {
  init {
    System.loadLibrary("binding_test_graphics")
  }

  external fun create(backend: Int): Long

  external fun context(graphics: Long): LongArray?

  external fun makeCurrent(graphics: Long): Boolean

  external fun textureCreate(graphics: Long, width: Int, height: Int): Long

  external fun textureInfo(texture: Long): LongArray?

  external fun textureDestroy(texture: Long)

  external fun lastError(): String

  external fun callCompletion(callback: Long, userData: Long, result: Long)

  external fun callCompletionRelease(release: Long, userData: Long)
}

/** tests/graphics over JNI. */
internal actual object TestGraphicsLibrary {
  actual suspend fun create(backend: UInt): Long {
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

  actual fun createTexture(graphics: Long, width: Int, height: Int): TestTexture {
    val texture = TestGraphicsJni.textureCreate(graphics, width, height)
    check(texture != 0L) {
      "tests/graphics could not create a texture: ${TestGraphicsJni.lastError()}"
    }
    val values = TestGraphicsJni.textureInfo(texture)
    if (values == null) {
      val reason = TestGraphicsJni.lastError()
      TestGraphicsJni.textureDestroy(texture)
      error("tests/graphics could not describe a texture: $reason")
    }
    return TestTexture(
      vulkanImage = values[0].toULong(),
      vulkanImageView = values[1].toULong(),
      metalTexture = values[2],
      width = values[3].toUInt(),
      height = values[4].toUInt(),
      format = values[5].toUInt(),
      vulkanFinalLayout = values[6].toUInt(),
      openGlTexture = values[7].toUInt(),
      openGlTarget = values[8].toUInt(),
    ) {
      TestGraphicsJni.textureDestroy(texture)
    }
  }
}
