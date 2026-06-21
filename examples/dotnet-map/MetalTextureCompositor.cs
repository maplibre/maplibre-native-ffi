using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed class MetalTextureCompositor : ITextureCompositor
{
    private const ulong MtlLoadActionClear = 2;
    private const ulong MtlStoreActionStore = 1;
    private const ulong MtlPrimitiveTypeTriangle = 3;

    private readonly MetalContext context;
    private nint commandQueue;
    private nint pipeline;

    public MetalTextureCompositor(MetalContext context)
    {
        this.context = context;
        commandQueue = context.CreateCommandQueue();
        try
        {
            pipeline = context.CreateRenderPipeline();
        }
        catch
        {
            context.ReleaseObject(commandQueue);
            commandQueue = 0;
            throw;
        }
    }

    public void Resize(Viewport viewport)
    {
        _ = viewport;
    }

    public bool Draw(MetalOwnedTextureFrame frame)
    {
        if (frame.Width == 0 || frame.Height == 0 || frame.Texture.IsNull)
        {
            throw new InvalidOperationException(
                "Owned Metal frame has an empty extent or null texture."
            );
        }

        DrawTexture(frame.Texture.Address);
        return true;
    }

    public void DrawTexture(nint texture)
    {
        nint passDescriptor = 0;
        try
        {
            using var pool = MacObjectiveC.AutoreleasePool();
            var drawable = context.NextDrawable();
            if (drawable == 0)
            {
                throw new InvalidOperationException("CAMetalLayer returned no drawable.");
            }

            passDescriptor = MacObjectiveC.AllocInit("MTLRenderPassDescriptor");
            var attachment = MacObjectiveC.SendPointer(
                MacObjectiveC.SendPointer(passDescriptor, "colorAttachments"),
                "objectAtIndexedSubscript:",
                0
            );
            MacObjectiveC.SendVoid(attachment, "setTexture:", context.DrawableTexture(drawable));
            MacObjectiveC.SendVoid(attachment, "setLoadAction:", MtlLoadActionClear);
            MacObjectiveC.SendVoid(attachment, "setStoreAction:", MtlStoreActionStore);

            var commandBuffer = MacObjectiveC.SendPointer(commandQueue, "commandBuffer");
            if (commandBuffer == 0)
            {
                throw new InvalidOperationException("Metal command buffer creation failed.");
            }

            var encoder = MacObjectiveC.SendPointer(
                commandBuffer,
                "renderCommandEncoderWithDescriptor:",
                passDescriptor
            );
            if (encoder == 0)
            {
                throw new InvalidOperationException(
                    "Metal render command encoder creation failed."
                );
            }

            MacObjectiveC.SendVoid(encoder, "setRenderPipelineState:", pipeline);
            MacObjectiveC.SendVoid(encoder, "setFragmentTexture:atIndex:", texture, 0UL);
            MacObjectiveC.SendVoid(
                encoder,
                "drawPrimitives:vertexStart:vertexCount:",
                MtlPrimitiveTypeTriangle,
                0,
                3
            );
            MacObjectiveC.SendVoid(encoder, "endEncoding");
            MacObjectiveC.SendVoid(commandBuffer, "presentDrawable:", drawable);
            MacObjectiveC.SendVoid(commandBuffer, "commit");
            MacObjectiveC.SendVoid(commandBuffer, "waitUntilCompleted");
        }
        finally
        {
            MacObjectiveC.Release(passDescriptor);
        }
    }

    public void Dispose()
    {
        context.ReleaseObject(pipeline);
        context.ReleaseObject(commandQueue);
        pipeline = 0;
        commandQueue = 0;
    }
}
