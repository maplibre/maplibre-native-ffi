use std::error::Error;

use glutin::config::{Config, ConfigTemplateBuilder};
use glutin::context::{AsRawContext, ContextApi, PossiblyCurrentContext, RawContext, Version};
use glutin::surface::{Surface, WindowSurface};
use glutin_winit::ApiPreference;
use maplibre_native::{NativePointer, OpenGLContextDescriptor, WglContextDescriptor};
use raw_window_handle::{HasWindowHandle, RawWindowHandle};
use windows_sys::Win32::Foundation::HWND;
use windows_sys::Win32::Graphics::Gdi::{GetDC, HDC, ReleaseDC};
use winit::window::Window;

pub struct OpenGLPlatformContext {
    hwnd: HWND,
    hdc: HDC,
}

impl OpenGLPlatformContext {
    pub fn new(window: &Window, _config: Config) -> Result<Self, Box<dyn Error>> {
        let hwnd = hwnd_from_window(window)?;
        let hdc = unsafe { GetDC(hwnd) };
        if hdc.is_null() {
            return Err("GetDC returned null".into());
        }
        Ok(Self { hwnd, hdc })
    }

    pub fn descriptor(
        &self,
        context: &PossiblyCurrentContext,
    ) -> Result<OpenGLContextDescriptor, Box<dyn Error>> {
        let RawContext::Wgl(raw_context) = context.raw_context() else {
            return Err("glutin did not create a WGL context".into());
        };
        Ok(OpenGLContextDescriptor::wgl(WglContextDescriptor::new(
            unsafe { NativePointer::from_ptr(self.hdc) },
            unsafe { NativePointer::from_ptr(raw_context.cast_mut()) },
        )))
    }

    pub fn surface_pointer(
        &self,
        _surface: &Surface<WindowSurface>,
    ) -> Result<NativePointer, Box<dyn Error>> {
        Ok(unsafe { NativePointer::from_ptr(self.hdc) })
    }

    pub fn context_api_name(&self) -> &'static str {
        "WGL/OpenGL"
    }
}

impl Drop for OpenGLPlatformContext {
    fn drop(&mut self) {
        if !self.hdc.is_null() {
            // SAFETY: hdc was acquired from this live window with GetDC.
            unsafe {
                ReleaseDC(self.hwnd, self.hdc);
            }
            self.hdc = std::ptr::null_mut();
        }
    }
}

pub fn configure_template(template: ConfigTemplateBuilder) -> ConfigTemplateBuilder {
    template
}

pub fn context_api() -> ContextApi {
    ContextApi::OpenGl(Some(Version::new(3, 0)))
}

pub fn opengl_api_preference() -> ApiPreference {
    ApiPreference::FallbackEgl
}

pub fn uses_gles() -> bool {
    false
}

fn hwnd_from_window(window: &Window) -> Result<HWND, Box<dyn Error>> {
    let RawWindowHandle::Win32(handle) = window.window_handle()?.as_raw() else {
        return Err("winit did not return a Win32 window handle".into());
    };
    Ok(handle.hwnd.get() as HWND)
}
