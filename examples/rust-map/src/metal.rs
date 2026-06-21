use maplibre_native::{
    Error as MaplibreError, ErrorKind, MetalOwnedTextureFrameHandle, NativePointer,
};
use objc2::ClassType;
use objc2::rc::Retained;
use objc2::runtime::ProtocolObject;
use objc2_app_kit::NSView;
use objc2_foundation::{CGSize, ns_string};
use objc2_metal::{
    MTLClearColor, MTLCommandBuffer, MTLCommandEncoder, MTLCommandQueue,
    MTLCreateSystemDefaultDevice, MTLDevice, MTLDrawable, MTLLibrary, MTLLoadAction,
    MTLPixelFormat, MTLPrimitiveType, MTLRenderCommandEncoder, MTLRenderPassDescriptor,
    MTLRenderPipelineDescriptor, MTLRenderPipelineState, MTLStoreAction, MTLTexture,
    MTLTextureDescriptor, MTLTextureUsage,
};
use objc2_quartz_core::{CAMetalDrawable, CAMetalLayer};
use raw_window_handle::{HasWindowHandle, RawWindowHandle};
use std::error::Error;
use winit::window::Window;

use crate::viewport::Viewport;

pub struct MetalContext {
    _view: Retained<NSView>,
    device: Retained<ProtocolObject<dyn MTLDevice>>,
    layer: Retained<CAMetalLayer>,
}

pub struct MetalTextureCompositor {
    layer: Retained<CAMetalLayer>,
    queue: Retained<ProtocolObject<dyn MTLCommandQueue>>,
    pipeline: Retained<ProtocolObject<dyn MTLRenderPipelineState>>,
}

pub struct MetalBorrowedTexture {
    texture: Retained<ProtocolObject<dyn MTLTexture>>,
}

impl MetalContext {
    pub fn new(window: &Window) -> Result<Self, Box<dyn Error>> {
        let viewport = Viewport::from_window(window);
        let raw_window = window.window_handle()?.as_raw();
        let RawWindowHandle::AppKit(handle) = raw_window else {
            return Err("Metal requires an AppKit window handle".into());
        };

        // SAFETY: winit returns the live NSView that backs this window.
        let view = unsafe { Retained::retain(handle.ns_view.as_ptr().cast::<NSView>()) }
            .ok_or("AppKit window handle did not contain an NSView")?;
        // SAFETY: MTLCreateSystemDefaultDevice returns a retained-compatible Objective-C object.
        let device = unsafe { Retained::retain(MTLCreateSystemDefaultDevice()) }
            .ok_or("MTLCreateSystemDefaultDevice returned nil")?;
        // SAFETY: CAMetalLayer::layer constructs a retained layer object.
        let layer = unsafe { CAMetalLayer::layer() };

        // SAFETY: device and layer are live Objective-C objects, and NSView accepts CALayer.
        unsafe {
            layer.setDevice(Some(&device));
            layer.setPixelFormat(MTLPixelFormat::BGRA8Unorm);
            layer.setDrawableSize(drawable_size(viewport));
            view.setWantsLayer(true);
            view.setLayer(Some(layer.as_super()));
        }

        Ok(Self {
            _view: view,
            device,
            layer,
        })
    }

    pub fn context_descriptor(&self) -> maplibre_native::MetalContextDescriptor {
        maplibre_native::MetalContextDescriptor::new(self.device_pointer())
    }

    pub fn layer_pointer(&self) -> NativePointer {
        // SAFETY: The CAMetalLayer is retained by MetalContext for the session lifetime.
        unsafe { NativePointer::from_address(Retained::as_ptr(&self.layer) as usize) }
    }

    pub fn resize(&self, viewport: Viewport) {
        // SAFETY: The layer is live and accepts physical drawable dimensions.
        unsafe { self.layer.setDrawableSize(drawable_size(viewport)) };
    }

    pub fn wait_idle(&self) {}

    fn device_pointer(&self) -> NativePointer {
        // SAFETY: The MTLDevice is retained by MetalContext for the session lifetime.
        unsafe { NativePointer::from_address(Retained::as_ptr(&self.device) as usize) }
    }
}

impl MetalTextureCompositor {
    pub fn new(context: &MetalContext) -> maplibre_native::Result<Self> {
        let queue = context
            .device
            .newCommandQueue()
            .ok_or_else(|| metal_error("Metal command queue creation failed"))?;
        let pipeline = create_pipeline(&context.device)?;
        // SAFETY: Retain a second reference so the compositor can hold the layer independently.
        let layer = unsafe { Retained::retain(Retained::as_ptr(&context.layer).cast_mut()) }
            .ok_or_else(|| metal_error("Metal layer retain failed"))?;
        Ok(Self {
            layer,
            queue,
            pipeline,
        })
    }

    pub fn draw(&mut self, frame: &MetalOwnedTextureFrameHandle) -> maplibre_native::Result<()> {
        let metadata = frame.frame()?;
        if metadata.width == 0 || metadata.height == 0 {
            return Err(metal_error("owned Metal frame has an empty extent"));
        }
        // SAFETY: The frame keeps this texture pointer valid until frame release.
        let texture = unsafe { frame.texture()?.as_ptr::<ProtocolObject<dyn MTLTexture>>() };
        let texture = unsafe { texture.as_ref() }
            .ok_or_else(|| metal_error("owned Metal frame has a null texture"))?;
        self.draw_texture(texture)
    }

    pub fn draw_texture(
        &mut self,
        texture: &ProtocolObject<dyn MTLTexture>,
    ) -> maplibre_native::Result<()> {
        // SAFETY: The layer, command queue, pipeline, and texture are live for this call.
        unsafe {
            let drawable = self
                .layer
                .nextDrawable()
                .ok_or_else(|| metal_error("CAMetalLayer returned no drawable"))?;
            let drawable_texture = drawable.texture();
            let pass_descriptor = MTLRenderPassDescriptor::renderPassDescriptor();
            let color_attachment = pass_descriptor
                .colorAttachments()
                .objectAtIndexedSubscript(0);
            color_attachment.setTexture(Some(&drawable_texture));
            color_attachment.setLoadAction(MTLLoadAction::Clear);
            color_attachment.setStoreAction(MTLStoreAction::Store);
            color_attachment.setClearColor(MTLClearColor {
                red: 0.08,
                green: 0.09,
                blue: 0.11,
                alpha: 1.0,
            });

            let command_buffer = self
                .queue
                .commandBuffer()
                .ok_or_else(|| metal_error("Metal command buffer creation failed"))?;
            let encoder = command_buffer
                .renderCommandEncoderWithDescriptor(&pass_descriptor)
                .ok_or_else(|| metal_error("Metal render command encoder creation failed"))?;
            encoder.setRenderPipelineState(&self.pipeline);
            encoder.setFragmentTexture_atIndex(Some(texture), 0);
            encoder.drawPrimitives_vertexStart_vertexCount(MTLPrimitiveType::Triangle, 0, 3);
            encoder.endEncoding();
            let present_drawable: &ProtocolObject<dyn MTLDrawable> =
                ProtocolObject::from_ref(&*drawable);
            command_buffer.presentDrawable(present_drawable);
            command_buffer.commit();
            command_buffer.waitUntilCompleted();
        }
        Ok(())
    }
}

impl MetalBorrowedTexture {
    pub fn new(context: &MetalContext, viewport: Viewport) -> maplibre_native::Result<Self> {
        // SAFETY: The descriptor and texture are created on the live Metal device.
        let descriptor = unsafe {
            MTLTextureDescriptor::texture2DDescriptorWithPixelFormat_width_height_mipmapped(
                MTLPixelFormat::RGBA8Unorm,
                viewport.physical_width as _,
                viewport.physical_height as _,
                false,
            )
        };
        descriptor.setUsage(MTLTextureUsage::ShaderRead | MTLTextureUsage::RenderTarget);
        let texture = context
            .device
            .newTextureWithDescriptor(&descriptor)
            .ok_or_else(|| metal_error("Metal borrowed texture creation failed"))?;
        Ok(Self { texture })
    }

    pub fn pointer(&self) -> NativePointer {
        // SAFETY: The MTLTexture is retained by MetalBorrowedTexture for the session lifetime.
        unsafe { NativePointer::from_address(Retained::as_ptr(&self.texture) as usize) }
    }

    pub fn texture(&self) -> &ProtocolObject<dyn MTLTexture> {
        &self.texture
    }
}

fn create_pipeline(
    device: &ProtocolObject<dyn MTLDevice>,
) -> maplibre_native::Result<Retained<ProtocolObject<dyn MTLRenderPipelineState>>> {
    let library = device
        .newLibraryWithSource_options_error(
            ns_string!(include_str!("metal_texture_compositor/shader.metal")),
            None,
        )
        .map_err(|error| metal_error(format!("Metal shader library creation failed: {error:?}")))?;
    let vertex = library
        .newFunctionWithName(ns_string!("vertex_main"))
        .ok_or_else(|| metal_error("Metal vertex function lookup failed"))?;
    let fragment = library
        .newFunctionWithName(ns_string!("fragment_main"))
        .ok_or_else(|| metal_error("Metal fragment function lookup failed"))?;
    let descriptor = MTLRenderPipelineDescriptor::new();
    descriptor.setVertexFunction(Some(&vertex));
    descriptor.setFragmentFunction(Some(&fragment));
    // SAFETY: Attachment 0 exists on a fresh render pipeline descriptor.
    unsafe {
        descriptor
            .colorAttachments()
            .objectAtIndexedSubscript(0)
            .setPixelFormat(MTLPixelFormat::BGRA8Unorm);
    }
    device
        .newRenderPipelineStateWithDescriptor_error(&descriptor)
        .map_err(|error| metal_error(format!("Metal render pipeline creation failed: {error:?}")))
}

fn drawable_size(viewport: Viewport) -> CGSize {
    CGSize::new(
        f64::from(viewport.physical_width),
        f64::from(viewport.physical_height),
    )
}

fn metal_error(message: impl Into<String>) -> MaplibreError {
    MaplibreError::new(ErrorKind::NativeError, None, message)
}
