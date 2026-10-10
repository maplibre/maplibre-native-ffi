use std::error::Error as StdError;

use maplibre_native_ffi::{
    MapHandle, QueueLock, RenderDriverKind, VulkanBorrowedTexture, VulkanBorrowedTextureDescriptor,
    VulkanContextDescriptor, VulkanOwnedTextureDescriptor, VulkanSurfaceDescriptor,
};

use crate::graphics::GraphicsContext;
use crate::map_state::MapState;
use crate::render_target::{
    BORROWED_RING_DEPTH, Mode, Replacements, Session, attach_options, compositor_error, extent,
    require_cpu_complete_producer,
};
use crate::shell::Wakes;
use crate::viewport::Viewport;
use crate::vulkan::{BorrowedImage, VulkanContext};
use crate::vulkan_texture_compositor::VulkanTextureCompositor;

/// The images of a borrowed ring, one per slot.
type VulkanRing = [BorrowedImage; BORROWED_RING_DEPTH];

pub enum RenderTarget {
    OwnedTexture {
        session: Session,
        compositor: Box<VulkanTextureCompositor>,
    },
    BorrowedTexture {
        session: Session,
        compositor: Box<VulkanTextureCompositor>,
        /// The ring the session renders into.
        ring: Box<VulkanRing>,
        replacements: Replacements<Box<VulkanRing>>,
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
        // The core worker submits to the host's queue, so it takes the host's
        // queue lock around each call on it.
        let mut options = attach_options(wakes, mode, RenderDriverKind::CoreWorker);
        let (lock, unlock) = (vk.queue_mutex().clone(), vk.queue_mutex().clone());
        options.queue_lock = QueueLock::default()
            .with_lock(move || lock.lock())
            .with_unlock(move || unlock.unlock());
        match mode {
            Mode::OwnedTexture => {
                let descriptor = VulkanOwnedTextureDescriptor {
                    extent: extent(viewport),
                    context: context_descriptor(vk),
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
                let ring = ring(vk, viewport)?;
                let descriptor = borrowed_descriptor(vk, viewport, &ring);
                let session = Session::new(
                    unsafe { map.vulkan_borrowed_texture_attach(&descriptor, &options) }?,
                    &options,
                    mode,
                    wakes,
                )?;
                Ok(Self::BorrowedTexture {
                    session,
                    compositor: Box::new(compositor(vk, viewport)?),
                    ring,
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
                ring: current,
                replacements,
            } => {
                // The replacement waits until the host holds no frame, and
                // the window keeps showing what it last presented.
                session.release_held()?;
                let replacement = ring(graphics.vulkan(), viewport)?;
                let descriptor = borrowed_descriptor(graphics.vulkan(), viewport, &replacement);
                let completion = unsafe {
                    session
                        .handle()
                        .vulkan_borrowed_texture_set_target(&descriptor)
                }?;
                let retired = std::mem::replace(current, replacement);
                replacements.push(completion, retired, wakes);
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

    /// Destroys each ring that a completed replacement retired. The session
    /// stopped rendering into it when the replacement completed, and the
    /// compositor waited for its own reads before the held frame was released.
    pub fn retire_replaced(
        &mut self,
        _graphics: &GraphicsContext,
        wakes: &Wakes,
    ) -> maplibre_native_ffi::Result<()> {
        if let Self::BorrowedTexture {
            session,
            replacements,
            ..
        } = self
        {
            while replacements.take_completed(session, wakes)?.is_some() {}
        }
        Ok(())
    }

    /// Shows the newest rendered frame, reporting false when no frame reached
    /// the window. The compositor's sampling finishes before this returns, so
    /// the session may render into the sampled image again once the frame is
    /// released.
    pub fn present(
        &mut self,
        _graphics: &GraphicsContext,
        _wakes: &Wakes,
    ) -> maplibre_native_ffi::Result<bool> {
        let presented = match self {
            Self::OwnedTexture {
                session,
                compositor,
            }
            | Self::BorrowedTexture {
                session,
                compositor,
                ..
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

fn ring(vk: &VulkanContext, viewport: Viewport) -> maplibre_native_ffi::Result<Box<VulkanRing>> {
    let image = || {
        BorrowedImage::new(vk, viewport)
            .map_err(|error| compositor_error(format!("Vulkan image creation failed: {error:?}")))
    };
    Ok(Box::new([image()?, image()?]))
}

fn borrowed_descriptor(
    vk: &VulkanContext,
    viewport: Viewport,
    ring: &VulkanRing,
) -> VulkanBorrowedTextureDescriptor {
    VulkanBorrowedTextureDescriptor {
        extent: extent(viewport),
        physical_width: viewport.physical_width,
        physical_height: viewport.physical_height,
        context: context_descriptor(vk),
        textures: ring
            .iter()
            .map(|image| VulkanBorrowedTexture::new(image.image_handle(), image.view_handle()))
            .collect(),
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
