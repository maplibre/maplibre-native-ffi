use std::error::Error;

use glutin::config::{AsRawConfig, Config, ConfigSurfaceTypes, ConfigTemplateBuilder, RawConfig};
use glutin::context::{AsRawContext, ContextApi, PossiblyCurrentContext, RawContext, Version};
use glutin::display::{AsRawDisplay, GetGlDisplay, RawDisplay};
use glutin::surface::{AsRawSurface, RawSurface, Surface, WindowSurface};
use glutin_winit::ApiPreference;
use maplibre_native::{EglContextDescriptor, NativePointer, OpenGLContextDescriptor};
use winit::window::Window;

pub struct OpenGLPlatformContext {
    config: Config,
}

impl OpenGLPlatformContext {
    pub fn new(_window: &Window, config: Config) -> Result<Self, Box<dyn Error>> {
        Ok(Self { config })
    }

    pub fn descriptor(
        &self,
        context: &PossiblyCurrentContext,
    ) -> Result<OpenGLContextDescriptor, Box<dyn Error>> {
        let RawDisplay::Egl(raw_display) = self.config.display().raw_display() else {
            return Err("glutin did not create an EGL display".into());
        };
        let RawConfig::Egl(raw_config) = self.config.raw_config() else {
            return Err("glutin did not choose an EGL config".into());
        };
        let RawContext::Egl(raw_context) = context.raw_context() else {
            return Err("glutin did not create an EGL context".into());
        };
        Ok(OpenGLContextDescriptor::Egl(EglContextDescriptor::new(
            unsafe { NativePointer::from_ptr(raw_display.cast_mut()) },
            unsafe { NativePointer::from_ptr(raw_config.cast_mut()) },
            unsafe { NativePointer::from_ptr(raw_context.cast_mut()) },
        )))
    }

    pub fn surface_pointer(
        &self,
        surface: &Surface<WindowSurface>,
    ) -> Result<NativePointer, Box<dyn Error>> {
        let RawSurface::Egl(raw_surface) = surface.raw_surface() else {
            return Err("glutin did not create an EGL surface".into());
        };
        Ok(unsafe { NativePointer::from_ptr(raw_surface.cast_mut()) })
    }

    pub fn context_api_name(&self) -> &'static str {
        "EGL/GLES"
    }
}

pub fn configure_template(template: ConfigTemplateBuilder) -> ConfigTemplateBuilder {
    template.with_surface_type(ConfigSurfaceTypes::WINDOW)
}

pub fn context_api() -> ContextApi {
    ContextApi::Gles(Some(Version::new(3, 0)))
}

pub fn opengl_api_preference() -> ApiPreference {
    ApiPreference::PreferEgl
}

pub fn uses_gles() -> bool {
    true
}
