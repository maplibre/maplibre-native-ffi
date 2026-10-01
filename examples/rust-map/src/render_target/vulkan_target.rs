use std::error::Error as StdError;

use maplibre_native_ffi::{
    GpuSync, MapHandle, VulkanBorrowedTextureDescriptor, VulkanContextDescriptor,
    VulkanOwnedTextureDescriptor, VulkanSurfaceDescriptor,
};

use crate::graphics::GraphicsContext;
use crate::map_state::MapState;
use crate::render_target::{
    Mode, Replacements, Session, attach_options, compositor_error, extent,
    require_cpu_complete_producer,
};
use crate::shell::Wakes;
use crate::viewport::Viewport;
use crate::vulkan::{BorrowedImage, VulkanContext};
use crate::vulkan_texture_compositor::VulkanTextureCompositor;

pub enum RenderTarget {
    OwnedTexture {
        session: Session,
        compositor: Box<VulkanTextureCompositor>,
    },
    BorrowedTexture {
        session: Session,
        compositor: Box<VulkanTextureCompositor>,
        /// The image the session renders into as far as completed replacements
        /// show.
        image: Box<BorrowedImage>,
        replacements: Replacements<BorrowedImage>,
    },
    Surface {
        session: Session,
    },
}

impl RenderTarget {
    pub fn attach(
        mode: Mode,
        map: &MapHandle,
        graphics: &GraphicsContext,
        viewport: Viewport,
        wakes: &Wakes,
    ) -> maplibre_native_ffi::Result<Self> {
        let vk = graphics.vulkan();
        // The window thread submits and presents on the same VkQueue this
        // descriptor hands over, so the session shares that thread rather than
        // driving the queue from a core worker.
        let options = attach_options(wakes, mode);
        match mode {
            Mode::OwnedTexture => {
                let descriptor = VulkanOwnedTextureDescriptor {
                    extent: extent(viewport),
                    context: context_descriptor(vk),
                };
                let session = Session::new(
                    unsafe { map.vulkan_owned_texture_attach(&descriptor, &options) }?,
                    false,
                )?;
                Ok(Self::OwnedTexture {
                    session,
                    compositor: Box::new(compositor(vk, viewport)?),
                })
            }
            Mode::BorrowedTexture => {
                let image = BorrowedImage::new(vk, viewport).map_err(|error| {
                    compositor_error(format!("Vulkan image creation failed: {error:?}"))
                })?;
                let descriptor = borrowed_descriptor(vk, viewport, &image);
                let session = Session::new(
                    unsafe { map.vulkan_borrowed_texture_attach(&descriptor, &options) }?,
                    false,
                )?;
                Ok(Self::BorrowedTexture {
                    session,
                    compositor: Box::new(compositor(vk, viewport)?),
                    image: Box::new(image),
                    replacements: Replacements::default(),
                })
            }
            Mode::NativeSurface => {
                let descriptor = VulkanSurfaceDescriptor {
                    extent: extent(viewport),
                    context: context_descriptor(vk),
                    surface: vk.surface_handle(),
                };
                Ok(Self::Surface {
                    session: Session::new(
                        unsafe { map.vulkan_surface_attach(&descriptor, &options) }?,
                        true,
                    )?,
                })
            }
        }
    }

    pub fn session_mut(&mut self) -> &mut Session {
        match self {
            Self::OwnedTexture { session, .. }
            | Self::BorrowedTexture { session, .. }
            | Self::Surface { session } => session,
        }
    }

    /// Starts the session resize or target replacement a new viewport needs.
    pub fn resize(
        &mut self,
        graphics: &GraphicsContext,
        map: &MapState,
        viewport: Viewport,
    ) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                compositor.resize(viewport).map_err(|error| {
                    compositor_error(format!("Vulkan resize failed: {error:?}"))
                })?;
                session.resize(viewport)?;
                Ok(())
            }
            Self::BorrowedTexture {
                session,
                compositor,
                replacements,
                ..
            } => {
                let replacement =
                    BorrowedImage::new(graphics.vulkan(), viewport).map_err(|error| {
                        compositor_error(format!("Vulkan image creation failed: {error:?}"))
                    })?;
                let descriptor = borrowed_descriptor(graphics.vulkan(), viewport, &replacement);
                let completion = unsafe {
                    session
                        .handle()
                        .vulkan_borrowed_texture_set_target(&descriptor)
                }?;
                replacements.push(completion, replacement);
                compositor.resize(viewport).map_err(|error| {
                    compositor_error(format!("Vulkan resize failed: {error:?}"))
                })?;
                // Target replacement changes only the graphics resource, so
                // the map takes the new extent directly.
                map.resize(viewport)
            }
            Self::Surface { session } => {
                session.resize(viewport)?;
                Ok(())
            }
        }
    }

    /// Services caller-driver work, then releases what completed target
    /// replacements retired.
    pub fn service(&mut self, graphics: &GraphicsContext) -> maplibre_native_ffi::Result<()> {
        self.session_mut().service()?;
        if let Self::BorrowedTexture {
            image,
            replacements,
            ..
        } = self
        {
            while let Some(replacement) = replacements.take_completed()? {
                // The compositor waits for its own sampling, but the session's
                // last render into the outgoing image may still be in flight.
                graphics.vulkan().wait_idle().map_err(|error| {
                    compositor_error(format!("Vulkan device wait failed: {error:?}"))
                })?;
                **image = replacement;
            }
        }
        Ok(())
    }

    /// Shows the newest rendered frame, reporting false when no frame reached
    /// the window. The compositor's sampling finishes before this returns, so
    /// the session may render into the sampled image again.
    pub fn present(&mut self, _graphics: &GraphicsContext) -> maplibre_native_ffi::Result<bool> {
        let presented = match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                // Without a new frame, the window keeps the one it already
                // shows.
                let Some(frame) = session.acquire_newest()? else {
                    return Ok(true);
                };
                require_cpu_complete_producer(&frame)?;
                let presented = compositor.draw(&frame);
                let waited = compositor.wait_idle();
                frame.release(&GpuSync::default())?;
                waited.map_err(|error| {
                    compositor_error(format!("Vulkan consumer wait failed: {error:?}"))
                })?;
                presented?
            }
            Self::BorrowedTexture {
                compositor, image, ..
            } => {
                let presented = compositor
                    .draw_image_view(image.view())
                    .map_err(|error| compositor_error(format!("Vulkan draw failed: {error:?}")))?;
                compositor.wait_idle().map_err(|error| {
                    compositor_error(format!("Vulkan consumer wait failed: {error:?}"))
                })?;
                presented
            }
            // The driver already presented the frame.
            Self::Surface { .. } => true,
        };
        Ok(presented)
    }

    pub fn close(self, _graphics: &GraphicsContext) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture {
                session,
                mut compositor,
            }
            | Self::BorrowedTexture {
                session,
                mut compositor,
                ..
            } => {
                session.close()?;
                compositor.close()?;
                Ok(())
            }
            Self::Surface { session } => session.close(),
        }
    }
}

fn compositor(
    vk: &VulkanContext,
    viewport: Viewport,
) -> maplibre_native_ffi::Result<VulkanTextureCompositor> {
    VulkanTextureCompositor::new(vk, viewport)
        .map_err(|error| compositor_error(format!("Vulkan compositor creation failed: {error:?}")))
}

fn borrowed_descriptor(
    vk: &VulkanContext,
    viewport: Viewport,
    image: &BorrowedImage,
) -> VulkanBorrowedTextureDescriptor {
    VulkanBorrowedTextureDescriptor {
        extent: extent(viewport),
        physical_width: viewport.physical_width,
        physical_height: viewport.physical_height,
        context: context_descriptor(vk),
        image: image.image_handle(),
        image_view: image.view_handle(),
        format: ash::vk::Format::R8G8B8A8_UNORM.as_raw() as u32,
        initial_layout: ash::vk::ImageLayout::UNDEFINED.as_raw() as u32,
        final_layout: ash::vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL.as_raw() as u32,
    }
}

fn context_descriptor(vk: &VulkanContext) -> VulkanContextDescriptor {
    let mut descriptor = VulkanContextDescriptor {
        instance: vk.instance_pointer(),
        physical_device: vk.physical_device_pointer(),
        device: vk.device_pointer(),
        graphics_queue: vk.graphics_queue_pointer(),
        graphics_queue_family_index: vk.graphics_queue_family_index(),
        ..Default::default()
    };
    descriptor.get_instance_proc_addr = vk.get_instance_proc_addr_pointer();
    descriptor.get_device_proc_addr = vk.get_device_proc_addr_pointer();
    descriptor
}
