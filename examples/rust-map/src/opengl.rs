use std::error::Error;
#[cfg(target_os = "windows")]
use std::ffi::c_void;
#[cfg(target_os = "windows")]
use std::ptr::null_mut;

#[cfg(target_os = "windows")]
use maplibre_native::WglContextDescriptor;
use maplibre_native::{NativePointer, OpenGLContextDescriptor};
#[cfg(target_os = "windows")]
use raw_window_handle::{HasWindowHandle, RawWindowHandle};
use winit::window::Window;

#[cfg(target_os = "windows")]
type Hwnd = *mut c_void;
#[cfg(target_os = "windows")]
type Hdc = *mut c_void;
#[cfg(target_os = "windows")]
type Hglrc = *mut c_void;

#[cfg(target_os = "windows")]
const PFD_DRAW_TO_WINDOW: u32 = 0x0000_0004;
#[cfg(target_os = "windows")]
const PFD_SUPPORT_OPENGL: u32 = 0x0000_0020;
#[cfg(target_os = "windows")]
const PFD_DOUBLEBUFFER: u32 = 0x0000_0001;
#[cfg(target_os = "windows")]
const PFD_TYPE_RGBA: u8 = 0;
#[cfg(target_os = "windows")]
const PFD_MAIN_PLANE: i32 = 0;

#[cfg(target_os = "windows")]
#[repr(C)]
#[allow(non_snake_case)]
struct PixelFormatDescriptor {
    nSize: u16,
    nVersion: u16,
    dwFlags: u32,
    iPixelType: u8,
    cColorBits: u8,
    cRedBits: u8,
    cRedShift: u8,
    cGreenBits: u8,
    cGreenShift: u8,
    cBlueBits: u8,
    cBlueShift: u8,
    cAlphaBits: u8,
    cAlphaShift: u8,
    cAccumBits: u8,
    cAccumRedBits: u8,
    cAccumGreenBits: u8,
    cAccumBlueBits: u8,
    cAccumAlphaBits: u8,
    cDepthBits: u8,
    cStencilBits: u8,
    cAuxBuffers: u8,
    iLayerType: u8,
    bReserved: u8,
    dwLayerMask: u32,
    dwVisibleMask: u32,
    dwDamageMask: u32,
}

pub struct OpenGLContext {
    #[cfg(target_os = "windows")]
    hwnd: Hwnd,
    #[cfg(target_os = "windows")]
    device_context: Hdc,
    #[cfg(target_os = "windows")]
    share_context: Hglrc,
}

impl OpenGLContext {
    pub fn new(window: &Window) -> Result<Self, Box<dyn Error>> {
        #[cfg(target_os = "windows")]
        {
            let hwnd = hwnd_from_window(window)?;
            // SAFETY: hwnd is a live winit-owned window on the event-loop thread.
            let hdc = unsafe { GetDC(hwnd) };
            if hdc.is_null() {
                return Err("GetDC returned null".into());
            }

            if let Err(error) = set_pixel_format(hdc) {
                // SAFETY: hdc came from this hwnd.
                unsafe {
                    ReleaseDC(hwnd, hdc);
                }
                return Err(error);
            }

            // SAFETY: hdc has an OpenGL-capable pixel format.
            let hglrc = unsafe { wglCreateContext(hdc) };
            if hglrc.is_null() {
                unsafe {
                    ReleaseDC(hwnd, hdc);
                }
                return Err("wglCreateContext returned null".into());
            }
            // SAFETY: hdc and hglrc belong to this helper and remain live.
            if unsafe { wglMakeCurrent(hdc, hglrc) } == 0 {
                unsafe {
                    wglDeleteContext(hglrc);
                    ReleaseDC(hwnd, hdc);
                }
                return Err("wglMakeCurrent failed".into());
            }
            println!("rust-map backend: OpenGL WGL");
            Ok(Self {
                hwnd,
                device_context: hdc,
                share_context: hglrc,
            })
        }

        #[cfg(target_os = "linux")]
        {
            let _ = window;
            // TODO(linux): Add an EGL/winit path after validating Mesa
            // llvmpipe on a Linux machine.
            Err("the Rust OpenGL example currently supports Windows WGL".into())
        }

        #[cfg(not(any(target_os = "windows", target_os = "linux")))]
        {
            let _ = window;
            Err("the Rust OpenGL example is only available on Windows WGL".into())
        }
    }

    pub fn descriptor(&self) -> OpenGLContextDescriptor {
        #[cfg(target_os = "windows")]
        {
            OpenGLContextDescriptor::wgl(
                WglContextDescriptor::new(
                    // SAFETY: Handles stay live until after the render target closes.
                    unsafe { NativePointer::from_ptr(self.device_context) },
                    unsafe { NativePointer::from_ptr(self.share_context) },
                )
                .with_proc_address(unsafe {
                    NativePointer::from_address(wglGetProcAddress as *const () as usize)
                }),
            )
        }

        #[cfg(not(target_os = "windows"))]
        {
            unreachable!("OpenGLContext::new is only implemented on Windows")
        }
    }

    pub fn surface_pointer(&self) -> NativePointer {
        #[cfg(target_os = "windows")]
        {
            // SAFETY: The HDC stays live until after the render target closes.
            unsafe { NativePointer::from_ptr(self.device_context) }
        }

        #[cfg(target_os = "linux")]
        {
            // TODO(linux): Return an EGLSurface once the Rust EGL helper exists.
            unreachable!("OpenGLContext::new is not implemented on Linux yet")
        }

        #[cfg(not(any(target_os = "windows", target_os = "linux")))]
        {
            unreachable!("OpenGL surfaces are only available on Windows WGL")
        }
    }

    pub fn device_context_pointer(&self) -> NativePointer {
        #[cfg(target_os = "windows")]
        {
            // SAFETY: The HDC stays live until after the render target closes.
            unsafe { NativePointer::from_ptr(self.device_context) }
        }

        #[cfg(not(target_os = "windows"))]
        {
            unreachable!("OpenGLContext::new is only implemented on Windows")
        }
    }

    pub fn share_context_pointer(&self) -> NativePointer {
        #[cfg(target_os = "windows")]
        {
            // SAFETY: The HGLRC stays live until after the render target closes.
            unsafe { NativePointer::from_ptr(self.share_context) }
        }

        #[cfg(not(target_os = "windows"))]
        {
            unreachable!("OpenGLContext::new is only implemented on Windows")
        }
    }

    pub fn make_current(&self) -> Result<(), Box<dyn Error>> {
        #[cfg(target_os = "windows")]
        {
            // SAFETY: The WGL handles belong to this helper and remain live.
            if unsafe { wglMakeCurrent(self.device_context, self.share_context) } == 0 {
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

    pub fn wait_idle(&self) -> Result<(), Box<dyn Error>> {
        self.make_current()?;
        // SAFETY: A current context exists on this thread.
        unsafe {
            glFinish();
        }
        Ok(())
    }
}

#[cfg(target_os = "windows")]
impl Drop for OpenGLContext {
    fn drop(&mut self) {
        // SAFETY: Handles belong to this context and are released after the
        // render target that borrowed them has closed.
        unsafe {
            let _ = wglMakeCurrent(null_mut(), null_mut());
            wglDeleteContext(self.share_context);
            ReleaseDC(self.hwnd, self.device_context);
        }
    }
}

#[cfg(target_os = "windows")]
fn hwnd_from_window(window: &Window) -> Result<Hwnd, Box<dyn Error>> {
    match window.window_handle()?.as_raw() {
        RawWindowHandle::Win32(handle) => Ok(handle.hwnd.get() as Hwnd),
        other => Err(format!("expected Win32 window handle, got {other:?}").into()),
    }
}

#[cfg(target_os = "windows")]
fn set_pixel_format(hdc: Hdc) -> Result<(), Box<dyn Error>> {
    let descriptor = PixelFormatDescriptor {
        nSize: std::mem::size_of::<PixelFormatDescriptor>() as u16,
        nVersion: 1,
        dwFlags: PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        iPixelType: PFD_TYPE_RGBA,
        cColorBits: 32,
        cRedBits: 0,
        cRedShift: 0,
        cGreenBits: 0,
        cGreenShift: 0,
        cBlueBits: 0,
        cBlueShift: 0,
        cAlphaBits: 8,
        cAlphaShift: 0,
        cAccumBits: 0,
        cAccumRedBits: 0,
        cAccumGreenBits: 0,
        cAccumBlueBits: 0,
        cAccumAlphaBits: 0,
        cDepthBits: 24,
        cStencilBits: 8,
        cAuxBuffers: 0,
        iLayerType: PFD_MAIN_PLANE as u8,
        bReserved: 0,
        dwLayerMask: 0,
        dwVisibleMask: 0,
        dwDamageMask: 0,
    };
    // SAFETY: hdc is live and descriptor points to initialized storage.
    let pixel_format = unsafe { ChoosePixelFormat(hdc, &descriptor) };
    if pixel_format == 0 {
        return Err("ChoosePixelFormat returned 0".into());
    }
    // SAFETY: hdc is live and descriptor describes the selected pixel format.
    if unsafe { SetPixelFormat(hdc, pixel_format, &descriptor) } == 0 {
        return Err("SetPixelFormat failed".into());
    }
    Ok(())
}

#[cfg(target_os = "windows")]
#[link(name = "user32")]
unsafe extern "system" {
    fn GetDC(hwnd: Hwnd) -> Hdc;
    fn ReleaseDC(hwnd: Hwnd, hdc: Hdc) -> i32;
}

#[cfg(target_os = "windows")]
#[link(name = "gdi32")]
unsafe extern "system" {
    fn ChoosePixelFormat(hdc: Hdc, descriptor: *const PixelFormatDescriptor) -> i32;
    fn SetPixelFormat(hdc: Hdc, format: i32, descriptor: *const PixelFormatDescriptor) -> i32;
}

#[cfg(target_os = "windows")]
#[link(name = "opengl32")]
unsafe extern "system" {
    fn wglCreateContext(hdc: Hdc) -> Hglrc;
    fn wglDeleteContext(context: Hglrc) -> i32;
    fn wglMakeCurrent(hdc: Hdc, context: Hglrc) -> i32;
    pub(crate) fn wglGetProcAddress(name: *const i8) -> *const c_void;
    pub(crate) fn glFinish();
}
