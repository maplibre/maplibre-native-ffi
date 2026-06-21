package org.maplibre.nativeffi.render

/** OpenGL context provider support flag reported by the native library build. */
public enum class OpenGLContextProvider(public val nativeMask: Int) {
  WGL(1),
  EGL(1 shl 1);

  internal companion object {
    internal fun fromMask(mask: UInt): Set<OpenGLContextProvider> = fromMask(mask.toInt())

    internal fun fromMask(mask: Int): Set<OpenGLContextProvider> =
      entries.filterTo(mutableSetOf()) { (mask and it.nativeMask) != 0 }
  }
}
