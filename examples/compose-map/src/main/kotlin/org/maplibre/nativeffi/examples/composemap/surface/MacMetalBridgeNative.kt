package org.maplibre.nativeffi.examples.composemap.surface

internal object MacMetalBridgeNative {
  private const val LIBRARY_PATH_PROPERTY = "org.maplibre.nativeffi.composemap.bridge.path"

  init {
    val path = System.getProperty(LIBRARY_PATH_PROPERTY)
    if (path.isNullOrBlank()) {
      System.loadLibrary("composemap_bridge")
    } else {
      System.load(path)
    }
  }

  @JvmStatic
  external fun createMetalTexture(
    metalDevice: Long,
    oldTexture: Long,
    width: Int,
    height: Int,
  ): Long

  @JvmStatic external fun disposeMetalTexture(texture: Long)

  @JvmStatic external fun texturePixelFormat(texture: Long): Long
}
