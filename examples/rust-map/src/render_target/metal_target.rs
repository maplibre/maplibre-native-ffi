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
        let options =
            RenderSessionAttachOptions::caller_graphics_thread(if mode == Mode::OwnedTexture {
                2
            } else {
                0
            });
        match mode {
            Mode::OwnedTexture => {
                let descriptor = maplibre_native_ffi::MetalOwnedTextureDescriptor::new(
                    extent(viewport),
                    metal.context_descriptor(),
                );
                let driver =
                    FrameDriver::new(map.attach_metal_owned_texture(&descriptor, options)?)?;
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
                let descriptor = maplibre_native_ffi::MetalBorrowedTextureDescriptor::new(
                    extent(viewport),
                    viewport.physical_width,
                    viewport.physical_height,
                    texture.pointer(),
                );
                let driver =
                    FrameDriver::new(map.attach_metal_borrowed_texture(&descriptor, options)?)?;
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
                    driver: FrameDriver::new(map.attach_metal_surface(&descriptor, options)?)?,
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
                let descriptor = maplibre_native_ffi::MetalBorrowedTextureDescriptor::new(
                    extent(viewport),
                    viewport.physical_width,
                    viewport.physical_height,
                    replacement.pointer(),
                );
                let operation = driver
                    .session()
                    .set_metal_borrowed_texture_target(&descriptor)?;
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
                frame
                    .release(GpuSync::CpuComplete)
                    .map_err(|error| error.into_error())?;
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
