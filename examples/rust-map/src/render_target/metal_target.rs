use std::error::Error as StdError;
use std::time::Duration;

use maplibre_native_ffi::{
    Error, ErrorKind, GpuSync, MapHandle, RenderSessionAttachOptions, RenderSessionAttachment,
    RenderSessionHandle,
};

use crate::graphics::GraphicsContext;
use crate::metal::{MetalBorrowedTexture, MetalTextureCompositor};
use crate::render_target::{Mode, extent, request_render_frame, require_cpu_complete_producer};
use crate::viewport::Viewport;

pub enum RenderTarget {
    OwnedTexture {
        session: RenderSessionHandle,
        compositor: Box<MetalTextureCompositor>,
    },
    BorrowedTexture {
        session: RenderSessionHandle,
        compositor: Box<MetalTextureCompositor>,
        texture: Box<MetalBorrowedTexture>,
    },
    Surface {
        session: RenderSessionHandle,
    },
}

impl RenderTarget {
    pub fn attach(
        mode: Mode,
        map: &MapHandle,
        graphics: &GraphicsContext,
        viewport: Viewport,
    ) -> maplibre_native_ffi::Result<Self> {
        let metal = graphics.metal();
        let options =
            RenderSessionAttachOptions::core_worker(if mode == Mode::OwnedTexture { 2 } else { 0 });
        match mode {
            Mode::OwnedTexture => {
                let descriptor = maplibre_native_ffi::MetalOwnedTextureDescriptor::new(
                    extent(viewport),
                    metal.context_descriptor(),
                );
                let session = finish_attach(map.attach_metal_owned_texture(&descriptor, options)?)?;
                let compositor = MetalTextureCompositor::new(metal).map_err(|error| {
                    compositor_error(format!("Metal compositor creation failed: {error:?}"))
                })?;
                Ok(Self::OwnedTexture {
                    session,
                    compositor: Box::new(compositor),
                })
            }
            Mode::BorrowedTexture => {
                let texture = MetalBorrowedTexture::new(metal, viewport)?;
                let descriptor = maplibre_native_ffi::MetalBorrowedTextureDescriptor::new(
                    extent(viewport),
                    viewport.physical_width,
                    viewport.physical_height,
                    texture.pointer(),
                );
                let session =
                    finish_attach(map.attach_metal_borrowed_texture(&descriptor, options)?)?;
                let compositor = MetalTextureCompositor::new(metal).map_err(|error| {
                    compositor_error(format!("Metal compositor creation failed: {error:?}"))
                })?;
                Ok(Self::BorrowedTexture {
                    session,
                    compositor: Box::new(compositor),
                    texture: Box::new(texture),
                })
            }
            Mode::NativeSurface => {
                let descriptor = maplibre_native_ffi::MetalSurfaceDescriptor::new(
                    extent(viewport),
                    metal.context_descriptor(),
                    metal.layer_pointer(),
                );
                Ok(Self::Surface {
                    session: finish_attach(map.attach_metal_surface(&descriptor, options)?)?,
                })
            }
        }
    }

    pub fn resize(
        &mut self,
        graphics: &GraphicsContext,
        viewport: Viewport,
    ) -> maplibre_native_ffi::Result<()> {
        match self {
            Self::BorrowedTexture {
                session, texture, ..
            } => {
                let replacement = MetalBorrowedTexture::new(graphics.metal(), viewport)?;
                let descriptor = maplibre_native_ffi::MetalBorrowedTextureDescriptor::new(
                    extent(viewport),
                    viewport.physical_width,
                    viewport.physical_height,
                    replacement.pointer(),
                );
                let operation = session.set_metal_borrowed_texture_target(&descriptor)?;
                wait_core(&operation, "Metal target replacement")?;
                **texture = replacement;
                Ok(())
            }
            Self::OwnedTexture { session, .. } | Self::Surface { session } => {
                drop(session.resize(&extent(viewport))?);
                Ok(())
            }
        }
    }

    pub fn render_update(
        &mut self,
        _graphics: &GraphicsContext,
    ) -> maplibre_native_ffi::Result<bool> {
        let present = matches!(self, Self::Surface { .. });
        let session = match self {
            Self::OwnedTexture { session, .. }
            | Self::BorrowedTexture { session, .. }
            | Self::Surface { session } => session,
        };
        let rendered = request_render_frame(session, present, false)?;
        if !rendered {
            return Ok(false);
        }
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                let frame = session.acquire_frame()?;
                require_cpu_complete_producer(&frame)?;
                let presented = compositor.draw(&frame)?;
                frame
                    .release(GpuSync::CPU_COMPLETE)
                    .map_err(|error| error.into_error())?;
                Ok(presented)
            }
            Self::BorrowedTexture {
                compositor,
                texture,
                ..
            } => compositor.draw_texture(texture.texture()),
            Self::Surface { .. } => Ok(true),
        }
    }

    pub fn close(self, _graphics: &GraphicsContext) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                detach_core(&session)?;
                drop(compositor);
                session
                    .destroy()
                    .map_err(|error| Box::new(error) as Box<dyn StdError>)
            }
            Self::BorrowedTexture {
                session,
                compositor,
                texture,
            } => {
                detach_core(&session)?;
                drop(compositor);
                drop(texture);
                session
                    .destroy()
                    .map_err(|error| Box::new(error) as Box<dyn StdError>)
            }
            Self::Surface { session } => {
                detach_core(&session)?;
                session
                    .destroy()
                    .map_err(|error| Box::new(error) as Box<dyn StdError>)
            }
        }
    }
}

fn finish_attach(
    attachment: RenderSessionAttachment,
) -> maplibre_native_ffi::Result<RenderSessionHandle> {
    if !attachment.completion.wait(Duration::from_secs(30))? {
        return Err(compositor_error("render attachment timed out"));
    }
    attachment.completion.take()?;
    let session = attachment.session;
    Ok(session)
}

fn wait_core(
    operation: &maplibre_native_ffi::NativeFuture<()>,
    name: &str,
) -> maplibre_native_ffi::Result<()> {
    if !operation.wait(Duration::from_secs(30))? {
        return Err(compositor_error(format!("{name} timed out")));
    }
    operation.take()
}

fn detach_core(session: &RenderSessionHandle) -> Result<(), Box<dyn StdError>> {
    let operation = session.detach()?;
    if !operation.wait(Duration::from_secs(30))? {
        return Err("render detach timed out".into());
    }
    operation.take()?;
    Ok(())
}

fn compositor_error(message: impl Into<String>) -> Error {
    Error::new(ErrorKind::NativeError, None, message)
}
