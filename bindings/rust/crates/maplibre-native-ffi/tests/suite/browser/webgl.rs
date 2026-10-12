//! A WebGL2 context and a host texture of the test's own, standing in for a
//! browser host's.

use std::error::Error as StdError;
use std::ffi::{CString, c_char, c_void};
use std::sync::atomic::{AtomicUsize, Ordering};

use maplibre_native_ffi::*;

use crate::browser::webgl_gl as gl;

unsafe extern "C" {
    // bindings/rust/crates/maplibre-native-ffi/emscripten/test_support.js.
    fn mln_test_register_offscreen_canvas(name: *const c_char, width: i32, height: i32);
    fn mln_test_unregister_offscreen_canvas(name: *const c_char);
}

/// Registers an OffscreenCanvas of the fixture's own and returns its id. The
/// context takes its canvas from the registry, and an OffscreenCanvas belongs
/// to the one thread that asked for it.
fn register_offscreen_canvas(width: u32, height: u32) -> CString {
    static SERIAL: AtomicUsize = AtomicUsize::new(1);
    let serial = SERIAL.fetch_add(1, Ordering::Relaxed);
    let id = CString::new(format!("mln-rust-test-{serial}"))
        .expect("the generated canvas id contains no NUL");
    // SAFETY: id is a live NUL-terminated string.
    unsafe { mln_test_register_offscreen_canvas(id.as_ptr(), width as i32, height as i32) };
    id
}

fn unregister_offscreen_canvas(id: &CString) {
    // SAFETY: id names a canvas this module registered.
    unsafe { mln_test_unregister_offscreen_canvas(id.as_ptr()) };
}

// emscripten/html5_webgl.h. Fields in declaration order; `bool` there is C23
// `_Bool`, which Rust's `bool` matches.
#[repr(C)]
struct ContextAttributes {
    alpha: bool,
    depth: bool,
    stencil: bool,
    antialias: bool,
    premultiplied_alpha: bool,
    preserve_drawing_buffer: bool,
    power_preference: i32,
    fail_if_major_performance_caveat: bool,
    major_version: i32,
    minor_version: i32,
    enable_extensions_by_default: bool,
    explicit_swap_control: bool,
    proxy_context_to_main_thread: i32,
    render_via_offscreen_back_buffer: bool,
}

/// EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW.
const PROXY_DISALLOW: i32 = 0;
/// EMSCRIPTEN_RESULT_SUCCESS.
const RESULT_SUCCESS: i32 = 0;

unsafe extern "C" {
    fn emscripten_webgl_init_context_attributes(attributes: *mut ContextAttributes);
    fn emscripten_webgl_create_context(
        target: *const c_char,
        attributes: *const ContextAttributes,
    ) -> i32;
    fn emscripten_webgl_make_context_current(context: i32) -> i32;
    fn emscripten_webgl_destroy_context(context: i32) -> i32;
}

/// A WebGL2 context on a canvas of the fixture's own, current on the thread
/// that created it, which drives the sessions that share it.
pub(super) struct WebGlTestContext {
    context: i32,
    id: CString,
}

impl WebGlTestContext {
    pub(super) fn new(width: u32, height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        let id = register_offscreen_canvas(width, height);

        let mut attributes = std::mem::MaybeUninit::<ContextAttributes>::uninit();
        // SAFETY: emscripten fills every field of the struct this points at.
        let mut attributes = unsafe {
            emscripten_webgl_init_context_attributes(attributes.as_mut_ptr());
            attributes.assume_init()
        };
        // WebGL2 is the GLES 3.0 the OpenGL backend targets.
        attributes.major_version = 2;
        attributes.minor_version = 0;
        attributes.depth = true;
        attributes.stencil = true;
        attributes.antialias = false;
        // The session renders into a texture rather than presenting, so the
        // context needs no drawing buffer preservation and no swap control. It
        // stays on the thread that created it.
        attributes.preserve_drawing_buffer = false;
        attributes.explicit_swap_control = false;
        attributes.proxy_context_to_main_thread = PROXY_DISALLOW;

        let target = CString::new(format!("#{}", id.to_str().expect("ASCII id")))
            .expect("the generated selector contains no NUL");
        // SAFETY: both pointers are live for the call.
        let context = unsafe { emscripten_webgl_create_context(target.as_ptr(), &attributes) };
        if context <= 0 {
            unregister_offscreen_canvas(&id);
            return Err(format!("creating the fixture's WebGL context failed: {context}").into());
        }
        let created = Self { context, id };
        created.make_current()?;
        Ok(created)
    }

    pub(super) fn descriptor(&self) -> OpenglContextDescriptor {
        OpenglContextDescriptor {
            data: OpenglContextDescriptorData::Webgl(WebglContextDescriptor {
                context: self.context,
                ..Default::default()
            }),
            ownership: OpenglContextOwnership::Shared,
        }
    }

    fn make_current(&self) -> std::result::Result<(), Box<dyn StdError>> {
        // SAFETY: the context is live until this value drops.
        let result = unsafe { emscripten_webgl_make_context_current(self.context) };
        if result == RESULT_SUCCESS {
            Ok(())
        } else {
            Err(format!("making the fixture's WebGL context current failed: {result}").into())
        }
    }

    fn gl(&self) -> gl::Context {
        // SAFETY: Emscripten links the GLES entry points into the module, so
        // there is nothing to look up.
        unsafe { gl::Context::from_loader_function(|_| std::ptr::null::<c_void>()) }
    }

    fn check_gl_error(&self, operation: &str) -> std::result::Result<(), Box<dyn StdError>> {
        // SAFETY: the context is current on this thread.
        let error = unsafe { self.gl().get_error() };
        if error == gl::NO_ERROR {
            Ok(())
        } else {
            Err(format!("{operation} failed with OpenGL error 0x{error:x}").into())
        }
    }
}

impl Drop for WebGlTestContext {
    fn drop(&mut self) {
        // SAFETY: the context and the registration are this value's, and both
        // are released exactly once.
        unsafe { emscripten_webgl_destroy_context(self.context) };
        unregister_offscreen_canvas(&self.id);
    }
}

/// An RGBA8 texture that the host allocates in its context and hands to a
/// session as its render target.
pub(super) struct WebGlBorrowedTexture {
    context: WebGlTestContext,
    texture: gl::NativeTexture,
    width: u32,
    height: u32,
}

impl WebGlBorrowedTexture {
    pub(super) fn new(width: u32, height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        let context = WebGlTestContext::new(width, height)?;
        let gl = context.gl();
        // SAFETY: the context is current on this thread, and the zeroed
        // pixels outlive the upload.
        let texture = unsafe {
            let texture = gl.create_texture()?;
            gl.bind_texture(gl::TEXTURE_2D, Some(texture));
            gl.tex_parameter_i32(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST as i32);
            gl.tex_parameter_i32(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST as i32);
            // Zeroed, so a readback that proves the session rendered cannot
            // pass on undefined contents.
            let blank = vec![0_u8; width as usize * height as usize * 4];
            gl.tex_image_2d(
                gl::TEXTURE_2D,
                0,
                gl::RGBA8 as i32,
                width as i32,
                height as i32,
                0,
                gl::RGBA,
                gl::UNSIGNED_BYTE,
                gl::PixelUnpackData::Slice(Some(&blank)),
            );
            gl.bind_texture(gl::TEXTURE_2D, None);
            texture
        };
        context.check_gl_error("creating the host texture")?;
        Ok(Self {
            context,
            texture,
            width,
            height,
        })
    }

    pub(super) fn descriptor(&self, extent: LogicalExtent) -> OpenglBorrowedTextureDescriptor {
        OpenglBorrowedTextureDescriptor {
            extent,
            physical_width: self.width,
            physical_height: self.height,
            context: self.context.descriptor(),
            textures: vec![OpenglBorrowedTexture::new(self.texture.0.get())],
            target: gl::TEXTURE_2D,
        }
    }

    /// Reads the texture back through a framebuffer of the host's own.
    pub(super) fn read_rgba(&self) -> std::result::Result<Vec<u8>, Box<dyn StdError>> {
        self.context.make_current()?;
        let gl = self.context.gl();
        let mut pixels = vec![0_u8; self.width as usize * self.height as usize * 4];
        // SAFETY: the context is current on this thread, and the pixel buffer
        // holds the whole texture.
        unsafe {
            let framebuffer = gl.create_framebuffer()?;
            gl.bind_framebuffer(gl::FRAMEBUFFER, Some(framebuffer));
            gl.framebuffer_texture_2d(
                gl::FRAMEBUFFER,
                gl::COLOR_ATTACHMENT0,
                gl::TEXTURE_2D,
                Some(self.texture),
                0,
            );
            let status = gl.check_framebuffer_status(gl::FRAMEBUFFER);
            if status == gl::FRAMEBUFFER_COMPLETE {
                gl.read_pixels(
                    0,
                    0,
                    self.width as i32,
                    self.height as i32,
                    gl::RGBA,
                    gl::UNSIGNED_BYTE,
                    gl::PixelPackData::Slice(Some(&mut pixels)),
                );
            }
            gl.bind_framebuffer(gl::FRAMEBUFFER, None);
            gl.delete_framebuffer(framebuffer);
            if status != gl::FRAMEBUFFER_COMPLETE {
                return Err(format!("the host framebuffer is incomplete: 0x{status:x}").into());
            }
        }
        self.context.check_gl_error("reading the host texture")?;
        Ok(pixels)
    }
}

impl Drop for WebGlBorrowedTexture {
    fn drop(&mut self) {
        if self.context.make_current().is_ok() {
            // SAFETY: the texture is this value's and is deleted once.
            unsafe { self.context.gl().delete_texture(self.texture) };
        }
    }
}
