#[cfg(target_os = "linux")]
#[path = "platform_linux.rs"]
mod imp;
#[cfg(target_os = "windows")]
#[path = "platform_windows.rs"]
mod imp;

pub use imp::{
    OpenGLPlatformContext, configure_template, context_api, opengl_api_preference, uses_gles,
};
