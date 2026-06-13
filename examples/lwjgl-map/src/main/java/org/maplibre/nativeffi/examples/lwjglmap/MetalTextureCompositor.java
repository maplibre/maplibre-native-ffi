package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.system.MemoryUtil.NULL;

import org.maplibre.nativeffi.render.MetalOwnedTextureFrameHandle;

final class MetalTextureCompositor implements AutoCloseable {
  private static final int MTL_LOAD_ACTION_CLEAR = 2;
  private static final int MTL_STORE_ACTION_STORE = 1;
  private static final int MTL_PRIMITIVE_TYPE_TRIANGLE = 3;

  private final MetalContext context;
  private long commandQueue;
  private long pipeline;

  MetalTextureCompositor(MetalContext context) {
    this.context = context;
    this.commandQueue = context.createCommandQueue();
    try {
      this.pipeline = context.createRenderPipeline();
    } catch (RuntimeException error) {
      MacObjectiveC.release(commandQueue);
      commandQueue = NULL;
      throw error;
    }
  }

  void draw(MetalOwnedTextureFrameHandle frameHandle) {
    var frame = frameHandle.frame();
    if (frame.width() == 0 || frame.height() == 0 || frame.texture().isNull()) {
      throw new IllegalStateException("owned Metal frame has an empty extent or null texture");
    }
    drawTexture(frame.texture().address());
  }

  void drawTexture(long texture) {
    long passDescriptor = NULL;
    try (var ignored = MacObjectiveC.autoreleasePool()) {
      var drawable = context.nextDrawable();
      if (drawable == NULL) {
        throw new IllegalStateException("CAMetalLayer returned no drawable");
      }
      passDescriptor = MacObjectiveC.allocInit("MTLRenderPassDescriptor");
      var attachment =
          MacObjectiveC.sendPointer(
              MacObjectiveC.sendPointer(passDescriptor, "colorAttachments"),
              "objectAtIndexedSubscript:",
              0);
      MacObjectiveC.sendVoid(attachment, "setTexture:", context.drawableTexture(drawable));
      MacObjectiveC.sendVoid(attachment, "setLoadAction:", MTL_LOAD_ACTION_CLEAR);
      MacObjectiveC.sendVoid(attachment, "setStoreAction:", MTL_STORE_ACTION_STORE);

      var commandBuffer = MacObjectiveC.sendPointer(commandQueue, "commandBuffer");
      if (commandBuffer == NULL) {
        throw new IllegalStateException("Metal command buffer creation failed");
      }
      var encoder =
          MacObjectiveC.sendPointer(
              commandBuffer, "renderCommandEncoderWithDescriptor:", passDescriptor);
      if (encoder == NULL) {
        throw new IllegalStateException("Metal render command encoder creation failed");
      }
      MacObjectiveC.sendVoid(encoder, "setRenderPipelineState:", pipeline);
      MacObjectiveC.sendVoid(encoder, "setFragmentTexture:atIndex:", texture, 0);
      MacObjectiveC.sendVoid(
          encoder, "drawPrimitives:vertexStart:vertexCount:", MTL_PRIMITIVE_TYPE_TRIANGLE, 0, 3);
      MacObjectiveC.sendVoid(encoder, "endEncoding");
      MacObjectiveC.sendVoid(commandBuffer, "presentDrawable:", drawable);
      MacObjectiveC.sendVoid(commandBuffer, "commit");
      MacObjectiveC.sendVoid(commandBuffer, "waitUntilCompleted");
    } finally {
      MacObjectiveC.release(passDescriptor);
    }
  }

  @Override
  public void close() {
    MacObjectiveC.release(pipeline);
    MacObjectiveC.release(commandQueue);
    pipeline = NULL;
    commandQueue = NULL;
  }
}
