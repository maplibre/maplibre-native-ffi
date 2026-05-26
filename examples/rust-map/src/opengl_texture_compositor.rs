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

            self.make_current()
                .map_err(|error| compositor_error(error.to_string()))?;
            // SAFETY: The WGL context is current on this thread, and the texture
            // name belongs to a shared context while the frame handle is open.
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
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
                glBindTexture(GL_TEXTURE_2D, 0);
                glDisable(GL_TEXTURE_2D);
            }
            self.swap_buffers()
                .map_err(|error| compositor_error(error.to_string()))
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
    fn glDisable(cap: u32);
    fn glEnable(cap: u32);
    fn glEnd();
    fn glLoadIdentity();
    fn glMatrixMode(mode: u32);
    fn glOrtho(left: f64, right: f64, bottom: f64, top: f64, near: f64, far: f64);
    fn glTexCoord2f(s: f32, t: f32);
    fn glTexParameteri(target: u32, pname: u32, param: i32);
    fn glVertex2f(x: f32, y: f32);
    fn glViewport(x: i32, y: i32, width: i32, height: i32);
}
