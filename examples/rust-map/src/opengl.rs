use std::error::Error;
use std::ffi::CString;
use std::num::NonZeroU32;
use std::rc::Rc;

use glow::HasContext;
use glutin::config::{ConfigTemplateBuilder, GlConfig};
use glutin::context::{ContextAttributesBuilder, PossiblyCurrentContext};
use glutin::display::{GetGlDisplay, GlDisplay};
use glutin::prelude::*;
use glutin::surface::{Surface, SurfaceAttributesBuilder, WindowSurface};
use glutin_winit::DisplayBuilder;
use maplibre_native::{NativePointer, OpenGLContextDescriptor, OpenGLOwnedTextureFrameHandle};
use raw_window_handle::HasWindowHandle;
use winit::event_loop::ActiveEventLoop;
use winit::window::{Window, WindowAttributes};

use crate::viewport::Viewport;

mod platform;

const TEXTURE_TARGET: u32 = glow::TEXTURE_2D;

pub struct OpenGLContext {
    platform: platform::OpenGLPlatformContext,
    context: PossiblyCurrentContext,
    surface: Surface<WindowSurface>,
    gl: Rc<glow::Context>,
}

pub struct OpenGLTextureCompositor {
    gl: Rc<glow::Context>,
    program: glow::Program,
    vertex_array: glow::VertexArray,
    viewport: Viewport,
}

pub struct OpenGLBorrowedTexture {
    gl: Rc<glow::Context>,
    texture: glow::Texture,
}

impl OpenGLContext {
    pub fn new(
        event_loop: &ActiveEventLoop,
        window_attributes: WindowAttributes,
    ) -> Result<(Window, Self), Box<dyn Error>> {
        let template = platform::configure_template(
            ConfigTemplateBuilder::new()
                .with_alpha_size(8)
                .with_depth_size(24)
                .with_stencil_size(8),
        );

        let (window, config) = DisplayBuilder::new()
            .with_preference(platform::opengl_api_preference())
            .with_window_attributes(Some(window_attributes))
            .build(event_loop, template, |configs| {
                configs
                    .max_by_key(GlConfig::num_samples)
                    .expect("glutin returned no OpenGL configs")
            })?;
        let window = window.ok_or("glutin did not create a window")?;
        let raw_window_handle = window.window_handle()?.as_raw();
        let context_attributes = ContextAttributesBuilder::new()
            .with_context_api(platform::context_api())
            .build(Some(raw_window_handle));
        let display = config.display();
        // SAFETY: raw_window_handle comes from a live window and the config was
        // selected from this display for that window.
        let not_current = unsafe { display.create_context(&config, &context_attributes)? };
        let size = window.inner_size();
        let surface_attributes = SurfaceAttributesBuilder::<WindowSurface>::new().build(
            raw_window_handle,
            nonzero_dimension(size.width),
            nonzero_dimension(size.height),
        );
        // SAFETY: raw_window_handle is live and surface attributes match it.
        let surface = unsafe { display.create_window_surface(&config, &surface_attributes)? };
        let context = not_current.make_current(&surface)?;
        // SAFETY: The loader returns function pointers from the current GL display.
        let gl = Rc::new(unsafe {
            glow::Context::from_loader_function(|symbol| {
                let symbol = CString::new(symbol).expect("GL symbol names do not contain NULs");
                display.get_proc_address(&symbol).cast()
            })
        });

        let platform = platform::OpenGLPlatformContext::new(&window, config)?;

        let context = Self {
            platform,
            context,
            surface,
            gl,
        };
        println!("OpenGL context: {}", context.context_api_name());
        Ok((window, context))
    }

    pub fn descriptor(&self) -> Result<OpenGLContextDescriptor, Box<dyn Error>> {
        self.platform.descriptor(&self.context)
    }

    pub fn surface_pointer(&self) -> Result<NativePointer, Box<dyn Error>> {
        self.platform.surface_pointer(&self.surface)
    }

    pub fn gl(&self) -> Rc<glow::Context> {
        Rc::clone(&self.gl)
    }

    pub fn make_current(&self) -> Result<(), Box<dyn Error>> {
        self.context.make_current(&self.surface)?;
        Ok(())
    }

    pub fn resize(&self, viewport: Viewport) -> Result<(), Box<dyn Error>> {
        self.make_current()?;
        self.surface.resize(
            &self.context,
            nonzero_dimension(viewport.physical_width),
            nonzero_dimension(viewport.physical_height),
        );
        Ok(())
    }

    pub fn swap_buffers(&self) -> Result<(), Box<dyn Error>> {
        self.surface.swap_buffers(&self.context)?;
        Ok(())
    }

    pub fn wait_idle(&self) {
        let _ = self.make_current();
        // SAFETY: The context is current for this thread.
        unsafe {
            self.gl.finish();
        }
    }

    fn context_api_name(&self) -> &'static str {
        self.platform.context_api_name()
    }
}

impl Drop for OpenGLContext {
    fn drop(&mut self) {
        self.wait_idle();
    }
}

impl OpenGLTextureCompositor {
    pub fn new(context: &OpenGLContext, viewport: Viewport) -> Result<Self, Box<dyn Error>> {
        context.make_current()?;
        let gl = context.gl();
        let program = create_texture_program(&gl)?;
        let mut program_guard = GlCleanupGuard::new(gl.clone(), program, delete_program);
        let vertex_array = match unsafe { gl.create_vertex_array() } {
            Ok(vertex_array) => vertex_array,
            Err(error) => {
                return Err(error.into());
            }
        };
        let mut vertex_array_guard =
            GlCleanupGuard::new(gl.clone(), vertex_array, delete_vertex_array);
        unsafe {
            gl.use_program(Some(program));
            if let Some(sampler) = gl.get_uniform_location(program, "map_texture") {
                gl.uniform_1_i32(Some(&sampler), 0);
            }
            gl.use_program(None);
        }
        check_gl_error(&gl, "initialize OpenGL texture compositor")?;
        let program = program_guard.take();
        let vertex_array = vertex_array_guard.take();
        Ok(Self {
            gl: gl.clone(),
            program,
            vertex_array,
            viewport,
        })
    }

    pub fn resize(&mut self, viewport: Viewport) {
        self.viewport = viewport;
    }

    pub fn draw_frame(
        &self,
        context: &OpenGLContext,
        frame: &OpenGLOwnedTextureFrameHandle,
    ) -> maplibre_native::Result<()> {
        let metadata = frame.frame()?;
        if metadata.width == 0 || metadata.height == 0 {
            return Err(compositor_error("owned OpenGL frame has an empty extent"));
        }
        if metadata.target != TEXTURE_TARGET {
            return Err(compositor_error(format!(
                "owned OpenGL frame has target {}, expected TEXTURE_2D",
                metadata.target
            )));
        }
        let texture = unsafe { frame.texture()?.value() };
        self.draw_texture(context, texture)
    }

    pub fn draw_texture(
        &self,
        context: &OpenGLContext,
        texture: u32,
    ) -> maplibre_native::Result<()> {
        context
            .make_current()
            .map_err(|error| compositor_error(format!("OpenGL make-current failed: {error}")))?;
        unsafe {
            self.gl.bind_framebuffer(glow::FRAMEBUFFER, None);
            self.gl.disable(glow::CULL_FACE);
            self.gl.disable(glow::DEPTH_TEST);
            self.gl.disable(glow::SCISSOR_TEST);
            self.gl.viewport(
                0,
                0,
                self.viewport.physical_width as i32,
                self.viewport.physical_height as i32,
            );
            self.gl.clear_color(0.08, 0.09, 0.11, 1.0);
            self.gl.clear(glow::COLOR_BUFFER_BIT);
            self.gl.use_program(Some(self.program));
            self.gl.bind_vertex_array(Some(self.vertex_array));
            self.gl.active_texture(glow::TEXTURE0);
            let texture = NonZeroU32::new(texture)
                .ok_or_else(|| compositor_error("OpenGL texture name is zero"))?;
            self.gl
                .bind_texture(TEXTURE_TARGET, Some(glow::NativeTexture(texture)));
            self.gl.tex_parameter_i32(
                TEXTURE_TARGET,
                glow::TEXTURE_MIN_FILTER,
                glow::LINEAR as i32,
            );
            self.gl.tex_parameter_i32(
                TEXTURE_TARGET,
                glow::TEXTURE_MAG_FILTER,
                glow::LINEAR as i32,
            );
            self.gl.draw_arrays(glow::TRIANGLES, 0, 3);
            self.gl.bind_texture(TEXTURE_TARGET, None);
            self.gl.bind_vertex_array(None);
            self.gl.use_program(None);
        }
        check_gl_error(&self.gl, "draw OpenGL texture")
            .map_err(|error| compositor_error(error.to_string()))?;
        context
            .swap_buffers()
            .map_err(|error| compositor_error(format!("OpenGL swap buffers failed: {error}")))
    }

    pub fn close(self, context: Option<&OpenGLContext>) {
        if let Some(context) = context {
            let _ = context.make_current();
        }
        unsafe {
            self.gl.finish();
            self.gl.delete_vertex_array(self.vertex_array);
            self.gl.delete_program(self.program);
        }
    }
}

impl OpenGLBorrowedTexture {
    pub fn new(context: &OpenGLContext, viewport: Viewport) -> Result<Self, Box<dyn Error>> {
        context.make_current()?;
        let gl = context.gl();
        let texture = unsafe { gl.create_texture()? };
        let mut texture_guard = GlCleanupGuard::new(gl.clone(), texture, delete_texture);
        unsafe {
            gl.bind_texture(TEXTURE_TARGET, Some(texture));
            gl.tex_parameter_i32(
                TEXTURE_TARGET,
                glow::TEXTURE_MIN_FILTER,
                glow::LINEAR as i32,
            );
            gl.tex_parameter_i32(
                TEXTURE_TARGET,
                glow::TEXTURE_MAG_FILTER,
                glow::LINEAR as i32,
            );
            gl.tex_image_2d(
                TEXTURE_TARGET,
                0,
                glow::RGBA8 as i32,
                viewport.physical_width as i32,
                viewport.physical_height as i32,
                0,
                glow::RGBA,
                glow::UNSIGNED_BYTE,
                glow::PixelUnpackData::Slice(None),
            );
            gl.bind_texture(TEXTURE_TARGET, None);
        }
        check_gl_error(&gl, "create OpenGL borrowed texture")?;
        let texture = texture_guard.take();
        Ok(Self {
            gl: gl.clone(),
            texture,
        })
    }

    pub fn texture(&self) -> u32 {
        self.texture.0.get()
    }

    pub fn target(&self) -> u32 {
        TEXTURE_TARGET
    }

    pub fn close(self, context: Option<&OpenGLContext>) {
        if let Some(context) = context {
            let _ = context.make_current();
        }
        unsafe {
            self.gl.delete_texture(self.texture);
        }
    }
}

fn create_texture_program(gl: &glow::Context) -> Result<glow::Program, Box<dyn Error>> {
    let vertex = compile_shader(gl, glow::VERTEX_SHADER, vertex_shader_source(), "vertex")?;
    let fragment = match compile_shader(
        gl,
        glow::FRAGMENT_SHADER,
        fragment_shader_source(),
        "fragment",
    ) {
        Ok(fragment) => fragment,
        Err(error) => {
            unsafe {
                gl.delete_shader(vertex);
            }
            return Err(error);
        }
    };
    let program = match unsafe { gl.create_program() } {
        Ok(program) => program,
        Err(error) => {
            unsafe {
                gl.delete_shader(vertex);
                gl.delete_shader(fragment);
            }
            return Err(error.into());
        }
    };
    unsafe {
        gl.attach_shader(program, vertex);
        gl.attach_shader(program, fragment);
        gl.link_program(program);
        gl.detach_shader(program, vertex);
        gl.detach_shader(program, fragment);
        gl.delete_shader(vertex);
        gl.delete_shader(fragment);
        if !gl.get_program_link_status(program) {
            let log = gl.get_program_info_log(program);
            gl.delete_program(program);
            return Err(format!("OpenGL texture compositor link failed: {log}").into());
        }
    }
    Ok(program)
}

fn compile_shader(
    gl: &glow::Context,
    kind: u32,
    source: &str,
    name: &str,
) -> Result<glow::Shader, Box<dyn Error>> {
    let shader = unsafe { gl.create_shader(kind)? };
    unsafe {
        gl.shader_source(shader, source);
        gl.compile_shader(shader);
        if !gl.get_shader_compile_status(shader) {
            let log = gl.get_shader_info_log(shader);
            gl.delete_shader(shader);
            return Err(
                format!("OpenGL texture compositor {name} shader compile failed: {log}").into(),
            );
        }
    }
    Ok(shader)
}

fn vertex_shader_source() -> &'static str {
    if platform::uses_gles() {
        "#version 300 es\n\
         out vec2 out_uv;\n\
         const vec2 positions[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));\n\
         const vec2 uvs[3] = vec2[3](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));\n\
         void main() {\n\
           gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);\n\
           out_uv = uvs[gl_VertexID];\n\
         }"
    } else {
        "#version 130\n\
         out vec2 out_uv;\n\
         vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));\n\
         vec2 uvs[3] = vec2[](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));\n\
         void main() {\n\
           gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);\n\
           out_uv = uvs[gl_VertexID];\n\
         }"
    }
}

fn fragment_shader_source() -> &'static str {
    if platform::uses_gles() {
        "#version 300 es\n\
         precision mediump float;\n\
         uniform sampler2D map_texture;\n\
         in vec2 out_uv;\n\
         out vec4 out_color;\n\
         void main() {\n\
           out_color = texture(map_texture, out_uv);\n\
         }"
    } else {
        "#version 130\n\
         uniform sampler2D map_texture;\n\
         in vec2 out_uv;\n\
         out vec4 out_color;\n\
         void main() {\n\
           out_color = texture(map_texture, out_uv);\n\
         }"
    }
}

fn check_gl_error(gl: &glow::Context, operation: &str) -> Result<(), Box<dyn Error>> {
    let error = unsafe { gl.get_error() };
    if error == glow::NO_ERROR {
        Ok(())
    } else {
        Err(format!("{operation} failed with OpenGL error 0x{error:x}").into())
    }
}

fn compositor_error(message: impl Into<String>) -> maplibre_native::Error {
    maplibre_native::Error::new(maplibre_native::ErrorKind::NativeError, None, message)
}

struct GlCleanupGuard<T> {
    gl: Rc<glow::Context>,
    object: Option<T>,
    cleanup: fn(&glow::Context, T),
}

impl<T> GlCleanupGuard<T> {
    fn new(gl: Rc<glow::Context>, object: T, cleanup: fn(&glow::Context, T)) -> Self {
        Self {
            gl,
            object: Some(object),
            cleanup,
        }
    }

    fn take(&mut self) -> T {
        self.object.take().expect("GL cleanup guard is live")
    }
}

impl<T> Drop for GlCleanupGuard<T> {
    fn drop(&mut self) {
        if let Some(object) = self.object.take() {
            (self.cleanup)(&self.gl, object);
        }
    }
}

fn delete_program(gl: &glow::Context, program: glow::Program) {
    // SAFETY: Constructor guards run before ownership escapes and while the
    // OpenGL context made current for construction is still current.
    unsafe {
        gl.delete_program(program);
    }
}

fn delete_vertex_array(gl: &glow::Context, vertex_array: glow::VertexArray) {
    // SAFETY: Constructor guards run before ownership escapes and while the
    // OpenGL context made current for construction is still current.
    unsafe {
        gl.delete_vertex_array(vertex_array);
    }
}

fn delete_texture(gl: &glow::Context, texture: glow::Texture) {
    // SAFETY: Constructor guards run before ownership escapes and while the
    // OpenGL context made current for construction is still current.
    unsafe {
        gl.delete_texture(texture);
    }
}

fn nonzero_dimension(value: u32) -> NonZeroU32 {
    NonZeroU32::new(value.max(1)).expect("dimension is clamped to at least one")
}
