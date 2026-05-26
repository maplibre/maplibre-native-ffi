use std::error::Error;

use maplibre_native::{
    Error as MaplibreError, ErrorKind, NativePointer, OpenGLOwnedTextureFrameHandle,
};

use crate::opengl::OpenGLContext;
use crate::viewport::Viewport;

#[cfg(target_os = "windows")]
const GL_COLOR_BUFFER_BIT: u32 = 0x0000_4000;
#[cfg(target_os = "windows")]
const GL_TEXTURE_2D: u32 = 0x0de1;
#[cfg(target_os = "windows")]
const GL_RGBA: u32 = 0x1908;
#[cfg(target_os = "windows")]
const GL_UNSIGNED_BYTE: u32 = 0x1401;
#[cfg(target_os = "windows")]
const GL_QUADS: u32 = 0x0007;
#[cfg(target_os = "windows")]
const GL_PROJECTION: u32 = 0x1701;
#[cfg(target_os = "windows")]
const GL_MODELVIEW: u32 = 0x1700;
#[cfg(target_os = "windows")]
const GL_LINEAR: i32 = 0x2601;
#[cfg(target_os = "windows")]
const GL_TEXTURE_MIN_FILTER: u32 = 0x2801;
#[cfg(target_os = "windows")]
const GL_TEXTURE_MAG_FILTER: u32 = 0x2800;

pub struct OpenGLTextureCompositor {
    device_context: NativePointer,
    share_context: NativePointer,
    viewport: Viewport,
    closed: bool,
}

impl OpenGLTextureCompositor {
    pub fn new(context: &OpenGLContext, viewport: Viewport) -> Result<Self, Box<dyn Error>> {
        context.make_current()?;
        Ok(Self {
            device_context: context.device_context_pointer(),
            share_context: context.share_context_pointer(),
            viewport,
            closed: false,
        })
    }

    pub fn resize(&mut self, viewport: Viewport) {
        self.viewport = viewport;
    }

    pub fn draw(&mut self, frame: &OpenGLOwnedTextureFrameHandle) -> maplibre_native::Result<()> {
        #[cfg(target_os = "windows")]
        {
            let metadata = frame.frame()?;
            if metadata.width == 0 || metadata.height == 0 {
                return Err(compositor_error("owned OpenGL frame has an empty extent"));
            }
            if metadata.target != GL_TEXTURE_2D {
                return Err(compositor_error(format!(
                    "owned OpenGL frame has target 0x{:x}, expected GL_TEXTURE_2D",
                    metadata.target
                )));
            }
            let texture = unsafe { frame.texture()?.value() };
            if texture == 0 {
                return Err(compositor_error("owned OpenGL frame has texture name 0"));
            }

            self.draw_texture(GL_TEXTURE_2D, texture)
        }

        #[cfg(not(target_os = "windows"))]
        {
            let _ = frame;
            // TODO(linux): Draw EGL-owned texture frames after validating the
            // Rust EGL helper on a Linux machine.
            Err(compositor_error(
                "OpenGL texture compositing is only available on Windows WGL",
            ))
        }
    }

    pub fn draw_texture(&mut self, target: u32, texture: u32) -> maplibre_native::Result<()> {
        #[cfg(target_os = "windows")]
        {
            if texture == 0 {
                return Err(compositor_error("OpenGL texture name is 0"));
            }
            self.make_current()
                .map_err(|error| compositor_error(error.to_string()))?;
            // SAFETY: The WGL context is current on this thread, and the texture
            // name belongs to a context shared with MapLibre.
            unsafe {
                glViewport(
                    0,
                    0,
                    self.viewport.physical_width as i32,
                    self.viewport.physical_height as i32,
                );
                glClearColor(0.08, 0.09, 0.11, 1.0);
                glClear(GL_COLOR_BUFFER_BIT);
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glEnable(target);
                glBindTexture(target, texture);
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glBegin(GL_QUADS);
                glTexCoord2f(0.0, 0.0);
                glVertex2f(-1.0, -1.0);
                glTexCoord2f(1.0, 0.0);
                glVertex2f(1.0, -1.0);
                glTexCoord2f(1.0, 1.0);
                glVertex2f(1.0, 1.0);
                glTexCoord2f(0.0, 1.0);
                glVertex2f(-1.0, 1.0);
                glEnd();
                glBindTexture(target, 0);
                glDisable(target);
            }
            self.swap_buffers()
                .map_err(|error| compositor_error(error.to_string()))
        }

        #[cfg(not(target_os = "windows"))]
        {
            let _ = (target, texture);
            // TODO(linux): Draw EGL borrowed textures after validating the
            // Rust EGL helper on a Linux machine.
            Err(compositor_error(
                "OpenGL texture compositing is only available on Windows WGL",
            ))
        }
    }

    pub fn close(&mut self) -> Result<(), Box<dyn Error>> {
        if self.closed {
            return Ok(());
        }
        self.closed = true;
        self.make_current()
    }

    fn make_current(&self) -> Result<(), Box<dyn Error>> {
        #[cfg(target_os = "windows")]
        {
            let hdc = unsafe { self.device_context.as_ptr::<std::ffi::c_void>() };
            let hglrc = unsafe { self.share_context.as_ptr::<std::ffi::c_void>() };
            // SAFETY: The WGL handles belong to the OpenGLContext that outlives
            // this compositor.
            if unsafe { wglMakeCurrent(hdc, hglrc) } == 0 {
                Err("wglMakeCurrent failed".into())
            } else {
                Ok(())
            }
        }

        #[cfg(not(target_os = "windows"))]
        {
            // TODO(linux): Make the EGL context current once the Linux helper exists.
            Err("OpenGL make-current is only available on Windows WGL".into())
        }
    }

    fn swap_buffers(&self) -> Result<(), Box<dyn Error>> {
        #[cfg(target_os = "windows")]
        {
            let hdc = unsafe { self.device_context.as_ptr::<std::ffi::c_void>() };
            // SAFETY: The HDC belongs to the winit window and remains live.
            if unsafe { SwapBuffers(hdc) } == 0 {
                Err("SwapBuffers failed".into())
            } else {
                Ok(())
            }
        }

        #[cfg(not(target_os = "windows"))]
        {
            // TODO(linux): Swap the EGLSurface once the Linux helper exists.
            Err("OpenGL swap-buffers is only available on Windows WGL".into())
        }
    }
}

pub struct OpenGLBorrowedTexture {
    device_context: NativePointer,
    share_context: NativePointer,
    texture: u32,
    closed: bool,
}

impl OpenGLBorrowedTexture {
    pub fn new(context: &OpenGLContext, viewport: Viewport) -> Result<Self, Box<dyn Error>> {
        #[cfg(target_os = "windows")]
        {
            context.make_current()?;
            let mut texture = 0;
            // SAFETY: The WGL context is current on this thread. The texture
            // storage is allocated empty for MapLibre to render into.
            unsafe {
                glGenTextures(1, &mut texture);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGBA as i32,
                    viewport.physical_width as i32,
                    viewport.physical_height as i32,
                    0,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    std::ptr::null(),
                );
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            if texture == 0 {
                return Err("glGenTextures returned 0".into());
            }
            Ok(Self {
                device_context: context.device_context_pointer(),
                share_context: context.share_context_pointer(),
                texture,
                closed: false,
            })
        }

        #[cfg(not(target_os = "windows"))]
        {
            let _ = (context, viewport);
            // TODO(linux): Allocate EGL borrowed textures after validating the
            // Rust EGL helper on a Linux machine.
            Err("OpenGL borrowed textures are only available on Windows WGL".into())
        }
    }

    pub fn texture(&self) -> u32 {
        self.texture
    }

    pub fn target(&self) -> u32 {
        #[cfg(target_os = "windows")]
        {
            GL_TEXTURE_2D
        }

        #[cfg(not(target_os = "windows"))]
        {
            // TODO(linux): Return the EGL texture target once the Linux helper exists.
            0
        }
    }

    pub fn close(&mut self) -> Result<(), Box<dyn Error>> {
        if self.closed {
            return Ok(());
        }
        self.closed = true;
        #[cfg(target_os = "windows")]
        {
            self.make_current()?;
            // SAFETY: The WGL context is current and the texture was created by
            // this helper.
            unsafe {
                glDeleteTextures(1, &self.texture);
            }
            self.texture = 0;
        }
        Ok(())
    }

    fn make_current(&self) -> Result<(), Box<dyn Error>> {
        #[cfg(target_os = "windows")]
        {
            let hdc = unsafe { self.device_context.as_ptr::<std::ffi::c_void>() };
            let hglrc = unsafe { self.share_context.as_ptr::<std::ffi::c_void>() };
            // SAFETY: The WGL handles belong to the OpenGLContext that outlives
            // this borrowed texture.
            if unsafe { wglMakeCurrent(hdc, hglrc) } == 0 {
                Err("wglMakeCurrent failed".into())
            } else {
                Ok(())
            }
        }

        #[cfg(not(target_os = "windows"))]
        {
            // TODO(linux): Make the EGL context current once the Linux helper exists.
            Err("OpenGL make-current is only available on Windows WGL".into())
        }
    }
}

impl Drop for OpenGLBorrowedTexture {
    fn drop(&mut self) {
        let _ = self.close();
    }
}

fn compositor_error(message: impl Into<String>) -> MaplibreError {
    MaplibreError::new(ErrorKind::NativeError, None, message)
}

#[cfg(target_os = "windows")]
#[link(name = "gdi32")]
unsafe extern "system" {
    fn SwapBuffers(hdc: *mut std::ffi::c_void) -> i32;
}

#[cfg(target_os = "windows")]
#[link(name = "opengl32")]
unsafe extern "system" {
    fn wglMakeCurrent(hdc: *mut std::ffi::c_void, context: *mut std::ffi::c_void) -> i32;
    fn glBegin(mode: u32);
    fn glBindTexture(target: u32, texture: u32);
    fn glClear(mask: u32);
    fn glClearColor(red: f32, green: f32, blue: f32, alpha: f32);
    fn glDeleteTextures(n: i32, textures: *const u32);
    fn glDisable(cap: u32);
    fn glEnable(cap: u32);
    fn glEnd();
    fn glGenTextures(n: i32, textures: *mut u32);
    fn glLoadIdentity();
    fn glMatrixMode(mode: u32);
    fn glOrtho(left: f64, right: f64, bottom: f64, top: f64, near: f64, far: f64);
    fn glTexImage2D(
        target: u32,
        level: i32,
        internal_format: i32,
        width: i32,
        height: i32,
        border: i32,
        format: u32,
        texture_type: u32,
        pixels: *const std::ffi::c_void,
    );
    fn glTexCoord2f(s: f32, t: f32);
    fn glTexParameteri(target: u32, pname: u32, param: i32);
    fn glVertex2f(x: f32, y: f32);
    fn glViewport(x: i32, y: i32, width: i32, height: i32);
}
