use std::error::Error as StdError;

use maplibre_native_ffi::{
    MapHandle, RenderDriverKind, VulkanBorrowedTextureDescriptor, VulkanContextDescriptor,
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
        /// The image the compositor samples.
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
        // A core worker drives every target, except a texture target whose
        // device gave the session no queue of its own. That one shares the
        // host's queue, so it renders on the event loop through a caller
        // driver. A surface shares the host's queue too, since the host submits
        // nothing there.
        let driver = if mode == Mode::NativeSurface || vk.session_queue_pointer().is_some() {
            RenderDriverKind::CoreWorker
        } else {
            RenderDriverKind::CallerGraphicsThread
        };
        let options = attach_options(wakes, driver);
        match mode {
            Mode::OwnedTexture => {
                let descriptor = VulkanOwnedTextureDescriptor {
                    extent: extent(viewport),
                    context: context_descriptor(vk, mode),
                };
                let session = Session::new(
                    unsafe { map.vulkan_owned_texture_attach(&descriptor, &options) }?,
                    &options,
                    mode,
                    wakes,
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
                    &options,
                    mode,
                    wakes,
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
                    context: context_descriptor(vk, mode),
                    surface: vk.surface_handle(),
                };
                Ok(Self::Surface {
                    session: Session::new(
                        unsafe { map.vulkan_surface_attach(&descriptor, &options) }?,
                        &options,
                        mode,
                        wakes,
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
        wakes: &Wakes,
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
                replacements.push(completion, replacement, wakes);
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

    /// Switches the compositor to each replacement a rendered frame has drawn
    /// into, destroying the image it retires. The session stopped rendering
    /// into that image when the replacement completed, and the compositor
    /// waited for its own reads.
    pub fn show_replacements(
        &mut self,
        _graphics: &GraphicsContext,
        wakes: &Wakes,
    ) -> maplibre_native_ffi::Result<()> {
        if let Self::BorrowedTexture {
            session,
            image,
            replacements,
            ..
        } = self
        {
            while let Some(replacement) = replacements.take_shown(session, wakes)? {
                **image = replacement;
            }
        }
        Ok(())
    }

    /// Shows the newest rendered frame, reporting false when no frame reached
    /// the window. The compositor's sampling finishes before this returns, so
    /// the session may render into the sampled image again.
    pub fn present(
        &mut self,
        graphics: &GraphicsContext,
        wakes: &Wakes,
    ) -> maplibre_native_ffi::Result<bool> {
        self.show_replacements(graphics, wakes)?;
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
                require_cpu_complete_producer(frame)?;
                let presented = compositor.draw(frame);
                // The held frame is released with CPU-complete sync, so the
                // compositor's reads must finish first.
                compositor.wait_idle().map_err(|error| {
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
        context: context_descriptor(vk, Mode::BorrowedTexture),
        image: image.image_handle(),
        image_view: image.view_handle(),
        format: ash::vk::Format::R8G8B8A8_UNORM.as_raw() as u32,
        initial_layout: ash::vk::ImageLayout::UNDEFINED.as_raw() as u32,
        final_layout: ash::vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL.as_raw() as u32,
    }
}

/// Names the session's queue: its own in a texture mode when the device has
/// one, and the host's otherwise.
fn context_descriptor(vk: &VulkanContext, mode: Mode) -> VulkanContextDescriptor {
    let session_queue = match mode {
        Mode::NativeSurface => None,
        Mode::OwnedTexture | Mode::BorrowedTexture => vk.session_queue_pointer(),
    };
    let mut descriptor = VulkanContextDescriptor {
        instance: vk.instance_pointer(),
        physical_device: vk.physical_device_pointer(),
        device: vk.device_pointer(),
        graphics_queue: session_queue.unwrap_or_else(|| vk.graphics_queue_pointer()),
        graphics_queue_family_index: vk.graphics_queue_family_index(),
        ..Default::default()
    };
    descriptor.get_instance_proc_addr = vk.get_instance_proc_addr_pointer();
    descriptor.get_device_proc_addr = vk.get_device_proc_addr_pointer();
    descriptor
}
