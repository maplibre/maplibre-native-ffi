package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.system.MemoryUtil.NULL

internal class MetalTextureCompositor(graphicsContext: GraphicsContext) : AutoCloseable {
  private val context = graphicsContext as MetalContext
  private var commandQueue = context.createCommandQueue()
  private var pipeline = NULL

  init {
    try {
      pipeline = context.createRenderPipeline()
    } catch (error: RuntimeException) {
      MacObjectiveC.release(commandQueue)
      commandQueue = NULL
      throw error
    }
  }

  /**
   * Samples the texture into the layer's next drawable, and reports whether it presented.
   *
   * A minimized or occluded window has no drawable to hand out, and the drawable pool is
   * momentarily empty under load. Both are transient, so the frame is skipped rather than failed:
   * the caller's render request stays pending and the draw retries once a drawable is available.
   */
  fun drawTexture(texture: Long): Boolean {
    var passDescriptor = NULL
    try {
      return MacObjectiveC.autoreleasePool().use {
        val drawable = context.nextDrawable()
        if (drawable == NULL) {
          return@use false
        }
        passDescriptor = MacObjectiveC.allocInit("MTLRenderPassDescriptor")
        val attachment =
          MacObjectiveC.sendPointer(
            MacObjectiveC.sendPointer(passDescriptor, "colorAttachments"),
            "objectAtIndexedSubscript:",
            0,
          )
        MacObjectiveC.sendVoid(attachment, "setTexture:", context.drawableTexture(drawable))
        MacObjectiveC.sendVoid(attachment, "setLoadAction:", MTL_LOAD_ACTION_CLEAR)
        MacObjectiveC.sendVoid(attachment, "setStoreAction:", MTL_STORE_ACTION_STORE)

        val commandBuffer = MacObjectiveC.sendPointer(commandQueue, "commandBuffer")
        check(commandBuffer != NULL) { "Metal command buffer creation failed" }
        val encoder =
          MacObjectiveC.sendPointer(
            commandBuffer,
            "renderCommandEncoderWithDescriptor:",
            passDescriptor,
          )
        check(encoder != NULL) { "Metal render command encoder creation failed" }
        MacObjectiveC.sendVoid(encoder, "setRenderPipelineState:", pipeline)
        MacObjectiveC.sendVoid(encoder, "setFragmentTexture:atIndex:", texture, 0)
        MacObjectiveC.sendVoid(
          encoder,
          "drawPrimitives:vertexStart:vertexCount:",
          MTL_PRIMITIVE_TYPE_TRIANGLE,
          0,
          3,
        )
        MacObjectiveC.sendVoid(encoder, "endEncoding")
        MacObjectiveC.sendVoid(commandBuffer, "presentDrawable:", drawable)
        MacObjectiveC.sendVoid(commandBuffer, "commit")
        MacObjectiveC.sendVoid(commandBuffer, "waitUntilCompleted")
        true
      }
    } finally {
      MacObjectiveC.release(passDescriptor)
    }
  }

  override fun close() {
    MacObjectiveC.release(pipeline)
    MacObjectiveC.release(commandQueue)
    pipeline = NULL
    commandQueue = NULL
  }

  private companion object {
    private const val MTL_LOAD_ACTION_CLEAR = 2
    private const val MTL_STORE_ACTION_STORE = 1
    private const val MTL_PRIMITIVE_TYPE_TRIANGLE = 3
  }
}
