//! The slice of GLES 3.0 the browser render fixtures use.
//!
//! glow supports Emscripten in its source, but its manifest declares js-sys and
//! wasm-bindgen for every `wasm32` target, and those do not build there. The
//! fixtures need a small set of entry points, which Emscripten links into the
//! module directly. This module preserves the names and shapes that the shared
//! fixture call sites use on every target. The surface readback the Windows
//! fixture performs has no browser counterpart and is absent here.

#![allow(non_snake_case)]

use std::ffi::c_void;
use std::num::NonZeroU32;

pub const COLOR_ATTACHMENT0: u32 = 0x8CE0;
pub const FRAMEBUFFER: u32 = 0x8D40;
pub const FRAMEBUFFER_COMPLETE: u32 = 0x8CD5;
pub const NEAREST: u32 = 0x2600;
pub const NO_ERROR: u32 = 0;
pub const RGBA: u32 = 0x1908;
pub const RGBA8: u32 = 0x8058;
pub const TEXTURE_2D: u32 = 0x0DE1;
pub const TEXTURE_MAG_FILTER: u32 = 0x2800;
pub const TEXTURE_MIN_FILTER: u32 = 0x2801;
pub const UNSIGNED_BYTE: u32 = 0x1401;

/// A GL object name that is never zero, matching `glow::NativeTexture`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct NativeTexture(pub NonZeroU32);

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct NativeFramebuffer(pub NonZeroU32);

/// Destination for `read_pixels`, matching glow's enum for the one form the
/// fixtures use.
pub enum PixelPackData<'a> {
    Slice(Option<&'a mut [u8]>),
}

/// Source for `tex_image_2d`, matching glow's enum for the one form the fixtures
/// use.
pub enum PixelUnpackData<'a> {
    Slice(Option<&'a [u8]>),
}

unsafe extern "C" {
    fn glGetError() -> u32;
    fn glFinish();
    fn glGenTextures(count: i32, textures: *mut u32);
    fn glBindTexture(target: u32, texture: u32);
    fn glTexParameteri(target: u32, name: u32, parameter: i32);
    fn glTexImage2D(
        target: u32,
        level: i32,
        internal_format: i32,
        width: i32,
        height: i32,
        border: i32,
        format: u32,
        kind: u32,
        pixels: *const c_void,
    );
    fn glDeleteTextures(count: i32, textures: *const u32);
    fn glGenFramebuffers(count: i32, framebuffers: *mut u32);
    fn glBindFramebuffer(target: u32, framebuffer: u32);
    fn glFramebufferTexture2D(
        target: u32,
        attachment: u32,
        texture_target: u32,
        texture: u32,
        level: i32,
    );
    fn glCheckFramebufferStatus(target: u32) -> u32;
    fn glDeleteFramebuffers(count: i32, framebuffers: *const u32);
    fn glReadPixels(
        x: i32,
        y: i32,
        width: i32,
        height: i32,
        format: u32,
        kind: u32,
        pixels: *mut c_void,
    );
}

/// The context the fixtures hold. Emscripten binds GL calls to whichever WebGL
/// context is current on the calling thread, so this carries no state; the
/// fixture makes its context current before it draws.
pub struct Context;

impl Context {
    /// Matches `glow::Context::from_loader_function`, which resolves entry
    /// points a browser module has already linked.
    ///
    /// # Safety
    ///
    /// Matches glow's contract: a GL context must be current on this thread
    /// before any call below.
    pub unsafe fn from_loader_function<F>(_loader: F) -> Self
    where
        F: FnMut(&str) -> *const c_void,
    {
        Self
    }

    /// # Safety
    ///
    /// Every method here calls GL and requires a current context, exactly as
    /// glow's `HasContext` does.
    pub unsafe fn get_error(&self) -> u32 {
        unsafe { glGetError() }
    }

    /// # Safety
    /// See [`Context::get_error`].
    pub unsafe fn finish(&self) {
        unsafe { glFinish() };
    }

    /// # Safety
    /// See [`Context::get_error`].
    pub unsafe fn create_texture(&self) -> Result<NativeTexture, String> {
        let mut name = 0_u32;
        unsafe { glGenTextures(1, &mut name) };
        NonZeroU32::new(name)
            .map(NativeTexture)
            .ok_or_else(|| "glGenTextures produced no texture".to_owned())
    }

    /// # Safety
    /// See [`Context::get_error`].
    pub unsafe fn bind_texture(&self, target: u32, texture: Option<NativeTexture>) {
        unsafe { glBindTexture(target, texture.map_or(0, |texture| texture.0.get())) };
    }

    /// # Safety
    /// See [`Context::get_error`].
    pub unsafe fn tex_parameter_i32(&self, target: u32, name: u32, parameter: i32) {
        unsafe { glTexParameteri(target, name, parameter) };
    }

    /// # Safety
    /// See [`Context::get_error`].
    #[allow(clippy::too_many_arguments)]
    pub unsafe fn tex_image_2d(
        &self,
        target: u32,
        level: i32,
        internal_format: i32,
        width: i32,
        height: i32,
        border: i32,
        format: u32,
        kind: u32,
        pixels: PixelUnpackData<'_>,
    ) {
        let PixelUnpackData::Slice(pixels) = pixels;
        let data = pixels.map_or(std::ptr::null(), |pixels| pixels.as_ptr().cast());
        unsafe {
            glTexImage2D(
                target,
                level,
                internal_format,
                width,
                height,
                border,
                format,
                kind,
                data,
            );
        }
    }

    /// # Safety
    /// See [`Context::get_error`].
    pub unsafe fn delete_texture(&self, texture: NativeTexture) {
        let name = texture.0.get();
        unsafe { glDeleteTextures(1, &name) };
    }

    /// # Safety
    /// See [`Context::get_error`].
    pub unsafe fn create_framebuffer(&self) -> Result<NativeFramebuffer, String> {
        let mut name = 0_u32;
        unsafe { glGenFramebuffers(1, &mut name) };
        NonZeroU32::new(name)
            .map(NativeFramebuffer)
            .ok_or_else(|| "glGenFramebuffers produced no framebuffer".to_owned())
    }

    /// # Safety
    /// See [`Context::get_error`].
    pub unsafe fn bind_framebuffer(&self, target: u32, framebuffer: Option<NativeFramebuffer>) {
        unsafe {
            glBindFramebuffer(
                target,
                framebuffer.map_or(0, |framebuffer| framebuffer.0.get()),
            );
        }
    }

    /// # Safety
    /// See [`Context::get_error`].
    pub unsafe fn framebuffer_texture_2d(
        &self,
        target: u32,
        attachment: u32,
        texture_target: u32,
        texture: Option<NativeTexture>,
        level: i32,
    ) {
        unsafe {
            glFramebufferTexture2D(
                target,
                attachment,
                texture_target,
                texture.map_or(0, |texture| texture.0.get()),
                level,
            );
        }
    }

    /// # Safety
    /// See [`Context::get_error`].
    pub unsafe fn check_framebuffer_status(&self, target: u32) -> u32 {
        unsafe { glCheckFramebufferStatus(target) }
    }

    /// # Safety
    /// See [`Context::get_error`].
    pub unsafe fn delete_framebuffer(&self, framebuffer: NativeFramebuffer) {
        let name = framebuffer.0.get();
        unsafe { glDeleteFramebuffers(1, &name) };
    }

    /// # Safety
    /// See [`Context::get_error`].
    #[allow(clippy::too_many_arguments)]
    pub unsafe fn read_pixels(
        &self,
        x: i32,
        y: i32,
        width: i32,
        height: i32,
        format: u32,
        kind: u32,
        pixels: PixelPackData<'_>,
    ) {
        let PixelPackData::Slice(pixels) = pixels;
        let data = pixels.map_or(std::ptr::null_mut(), |pixels| pixels.as_mut_ptr().cast());
        unsafe { glReadPixels(x, y, width, height, format, kind, data) };
    }
}
