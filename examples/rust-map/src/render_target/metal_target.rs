use std::error::Error as StdError;

use maplibre_native_ffi::{GpuSync, MapHandle, RenderSessionAttachOptions};

use crate::graphics::GraphicsContext;
use crate::map_state::MapState;
use crate::metal::{MetalBorrowedTexture, MetalTextureCompositor};
use crate::render_target::{
    FrameDriver, FrameOutcome, Mode, compositor_error, extent, require_cpu_complete_producer,
};
use crate::viewport::Viewport;

pub enum RenderTarget {
    OwnedTexture {
        driver: FrameDriver,
        compositor: Box<MetalTextureCompositor>,
    },
    BorrowedTexture {
        driver: FrameDriver,
        compositor: Box<MetalTextureCompositor>,
        texture: Box<MetalBorrowedTexture>,
    },
    Surface {
        driver: FrameDriver,
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
        let options = RenderSessionAttachOptions {
            driver: maplibre_native_ffi::RenderDriverKind::CallerGraphicsThread,
            requested_texture_ring_depth: if mode == Mode::OwnedTexture { 2 } else { 0 },
            ..Default::default()
        };
        match mode {
            Mode::OwnedTexture => {
                let descriptor = maplibre_native_ffi::MetalOwnedTextureDescriptor {
                    extent: extent(viewport),
                    context: metal.context_descriptor(),
                };
                let driver = FrameDriver::new(unsafe {
                    map.metal_owned_texture_attach(&descriptor, &(options))
                }?)?;
                let compositor = MetalTextureCompositor::new(metal).map_err(|error| {
                    compositor_error(format!("Metal compositor creation failed: {error:?}"))
                })?;
                Ok(Self::OwnedTexture {
                    driver,
                    compositor: Box::new(compositor),
                })
            }
            Mode::BorrowedTexture => {
                let texture = MetalBorrowedTexture::new(metal, viewport)?;
                let descriptor = maplibre_native_ffi::MetalBorrowedTextureDescriptor {
                    extent: extent(viewport),
                    physical_width: viewport.physical_width,
                    physical_height: viewport.physical_height,
                    texture: texture.pointer(),
                };
                let driver = FrameDriver::new(unsafe {
                    map.metal_borrowed_texture_attach(&descriptor, &(options))
                }?)?;
                let compositor = MetalTextureCompositor::new(metal).map_err(|error| {
                    compositor_error(format!("Metal compositor creation failed: {error:?}"))
                })?;
                Ok(Self::BorrowedTexture {
                    driver,
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
                    driver: FrameDriver::new(unsafe {
                        map.metal_surface_attach(&descriptor, &(options))
                    }?)?,
                })
            }
        }
    }

    pub fn resize(
        &mut self,
        graphics: &GraphicsContext,
        map: &MapState,
        viewport: Viewport,
    ) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::BorrowedTexture {
                driver, texture, ..
            } => {
                let replacement = MetalBorrowedTexture::new(graphics.metal(), viewport)?;
                let descriptor = maplibre_native_ffi::MetalBorrowedTextureDescriptor {
                    extent: extent(viewport),
                    physical_width: viewport.physical_width,
                    physical_height: viewport.physical_height,
                    texture: replacement.pointer(),
                };
                let operation = unsafe {
                    driver
                        .session()
                        .metal_borrowed_texture_set_target(&descriptor)
                }?;
                driver.drive(&operation)?;
                **texture = replacement;
                // Target replacement changes only the graphics resource, so
                // the map takes the new extent directly.
                map.resize(viewport)
            }
            Self::OwnedTexture { driver, .. } | Self::Surface { driver } => {
                driver.resize(viewport)?;
                Ok(())
            }
        }
    }

    pub fn render_update(
        &mut self,
        _graphics: &GraphicsContext,
    ) -> maplibre_native_ffi::Result<FrameOutcome> {
        let present = matches!(self, Self::Surface { .. });
        let driver = match self {
            Self::OwnedTexture { driver, .. }
            | Self::BorrowedTexture { driver, .. }
            | Self::Surface { driver } => driver,
        };
        let mut outcome = driver.render_frame(present)?;
        if !outcome.rendered {
            return Ok(outcome);
        }
        match self {
            Self::OwnedTexture { driver, compositor } => {
                let Some(frame) = driver.acquire_frame()? else {
                    outcome.rendered = false;
                    return Ok(outcome);
                };
                require_cpu_complete_producer(&frame)?;
                outcome.rendered = compositor.draw(&frame)?;
                frame.release(&GpuSync::default())?;
            }
            Self::BorrowedTexture {
                compositor,
                texture,
                ..
            } => outcome.rendered = compositor.draw_texture(texture.texture())?,
            Self::Surface { .. } => {}
        }
        Ok(outcome)
    }

    pub fn close(self, _graphics: &GraphicsContext) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture { driver, compositor } => {
                driver.close()?;
                drop(compositor);
                Ok(())
            }
            Self::BorrowedTexture {
                driver,
                compositor,
                texture,
            } => {
                driver.close()?;
                drop(compositor);
                drop(texture);
                Ok(())
            }
            Self::Surface { driver } => driver.close(),
        }
    }
}
