//! A WebGPU device and a host texture of the test's own, standing in for a
//! browser host's.

use std::error::Error as StdError;

use maplibre_native_ffi::*;

/// The WebGPU header the module links, bound by this crate's build script.
///
/// Generated rather than taken from a crate: the fixtures hand their device to
/// a session as an opaque handle, so it has to come from the very emdawnwebgpu
/// instance the core links.
#[allow(
    non_snake_case,
    non_camel_case_types,
    non_upper_case_globals,
    dead_code
)]
mod webgpu_sys {
    include!(concat!(env!("OUT_DIR"), "/webgpu.rs"));
}

/// A WebGPU device of the fixture's own, standing in for a browser host's.
///
/// One per fixture rather than one per thread: a device outliving its fixture
/// holds a runtime keepalive, and libtest offers no hook to end one afterwards.
/// A device also belongs to the thread that created it, because emdawnwebgpu
/// keeps WebGPU objects in the JS realm of that worker, and the suite attaches
/// sessions from more than one thread.
pub(super) struct WebGpuTestContext {
    instance: webgpu_sys::WGPUInstance,
    adapter: webgpu_sys::WGPUAdapter,
    device: webgpu_sys::WGPUDevice,
}

/// A slot an asynchronous WebGPU callback writes into, owned by the callback.
///
/// A wait here is bounded, and WebGPU delivers the callback whether or not the
/// wait is still listening. A slot on the waiting stack, or inside the fixture
/// the wait gives up on, would be written through after it is gone, so the
/// callback holds a reference of its own and drops it when it fires.
type CallbackSlot<T> = std::rc::Rc<std::cell::Cell<Option<T>>>;

fn callback_slot<T>() -> (CallbackSlot<T>, *mut std::ffi::c_void) {
    let slot: CallbackSlot<T> = std::rc::Rc::new(std::cell::Cell::new(None));
    (slot.clone(), std::rc::Rc::into_raw(slot).cast_mut().cast())
}

/// Stores a callback's result and releases the reference it was given.
///
/// # Safety
///
/// `user_data` is the pointer [`callback_slot`] produced for this one callback,
/// which WebGPU delivers exactly once, and `T` is the type that slot was made
/// with. The pointer carries no type of its own, so each caller names it.
unsafe fn fill_callback_slot<T>(user_data: *mut std::ffi::c_void, value: T) {
    // SAFETY: the caller guarantees this pointer came from callback_slot and
    // reaches here once, so this takes back that one reference.
    let slot = unsafe {
        std::rc::Rc::from_raw(user_data.cast_const().cast::<std::cell::Cell<Option<T>>>())
    };
    slot.set(Some(value));
}

unsafe extern "C" fn on_adapter(
    status: webgpu_sys::WGPURequestAdapterStatus,
    adapter: webgpu_sys::WGPUAdapter,
    _message: webgpu_sys::WGPUStringView,
    user_data: *mut std::ffi::c_void,
    _reserved: *mut std::ffi::c_void,
) {
    let adapter = if status == webgpu_sys::WGPURequestAdapterStatus_Success {
        adapter
    } else {
        std::ptr::null_mut()
    };
    // SAFETY: user_data is this request's adapter slot, delivered once.
    unsafe { fill_callback_slot::<webgpu_sys::WGPUAdapter>(user_data, adapter) };
}

unsafe extern "C" fn on_device(
    status: webgpu_sys::WGPURequestDeviceStatus,
    device: webgpu_sys::WGPUDevice,
    _message: webgpu_sys::WGPUStringView,
    user_data: *mut std::ffi::c_void,
    _reserved: *mut std::ffi::c_void,
) {
    let device = if status == webgpu_sys::WGPURequestDeviceStatus_Success {
        device
    } else {
        std::ptr::null_mut()
    };
    // SAFETY: user_data is this request's device slot, delivered once.
    unsafe { fill_callback_slot::<webgpu_sys::WGPUDevice>(user_data, device) };
}

/// Waits for one of WebGPU's futures.
///
/// Adapter and device requests are asynchronous. The fixtures run on a worker
/// where waiting is legal, so this blocks rather than unwinding the test into a
/// callback. Bounded, so a browser without a WebGPU adapter fails the fixture
/// rather than hanging the suite until the runner's timeout.
///
/// A non-zero timeout is only legal on an instance that asked for timed waits;
/// see [`WebGpuTestContext::new`].
fn await_future(
    instance: webgpu_sys::WGPUInstance,
    future: webgpu_sys::WGPUFuture,
) -> std::result::Result<(), Box<dyn StdError>> {
    const TIMEOUT_NS: u64 = 5 * 1000 * 1000 * 1000;
    let mut wait = webgpu_sys::WGPUFutureWaitInfo {
        future,
        completed: 0,
    };
    // SAFETY: instance is live and wait points at one writable entry.
    let status = unsafe { webgpu_sys::wgpuInstanceWaitAny(instance, 1, &mut wait, TIMEOUT_NS) };
    if status != webgpu_sys::WGPUWaitStatus_Success || wait.completed == 0 {
        return Err(format!(
            "waiting on a WebGPU future failed (status {status}, completed {})",
            wait.completed
        )
        .into());
    }
    Ok(())
}

impl WebGpuTestContext {
    pub(super) fn new() -> std::result::Result<Self, Box<dyn StdError>> {
        // Waiting on a future with a timeout has to be asked for up front, or
        // wgpuInstanceWaitAny rejects every non-zero timeout instead of waiting.
        // The capability needs Asyncify or JSPI, which the emdawnwebgpu port
        // enables, and wgpuCreateInstance answers null when asked without it.
        let mut descriptor: webgpu_sys::WGPUInstanceDescriptor = unsafe { std::mem::zeroed() };
        descriptor.capabilities.timedWaitAnyEnable = 1;
        // SAFETY: descriptor is live for the call.
        let instance = unsafe { webgpu_sys::wgpuCreateInstance(&descriptor) };
        if instance.is_null() {
            return Err("creating a WebGPU instance with timed waits enabled failed".into());
        }
        let context = Self {
            instance,
            adapter: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
        };
        context.request_adapter_and_device()
    }

    fn request_adapter_and_device(mut self) -> std::result::Result<Self, Box<dyn StdError>> {
        let options: webgpu_sys::WGPURequestAdapterOptions = unsafe { std::mem::zeroed() };
        let (adapter_slot, adapter_user_data) = callback_slot::<webgpu_sys::WGPUAdapter>();
        let mut adapter_info: webgpu_sys::WGPURequestAdapterCallbackInfo =
            unsafe { std::mem::zeroed() };
        adapter_info.mode = webgpu_sys::WGPUCallbackMode_AllowProcessEvents;
        adapter_info.callback = Some(on_adapter);
        adapter_info.userdata1 = adapter_user_data;
        // SAFETY: instance is live, options outlives the call, and the slot
        // outlives the request whatever the wait below does.
        let future = unsafe {
            webgpu_sys::wgpuInstanceRequestAdapter(self.instance, &options, adapter_info)
        };
        await_future(self.instance, future)?;
        self.adapter = adapter_slot
            .take()
            .filter(|adapter| !adapter.is_null())
            .ok_or("this browser provided no WebGPU adapter")?;

        let device_descriptor: webgpu_sys::WGPUDeviceDescriptor = unsafe { std::mem::zeroed() };
        let (device_slot, device_user_data) = callback_slot::<webgpu_sys::WGPUDevice>();
        let mut device_info: webgpu_sys::WGPURequestDeviceCallbackInfo =
            unsafe { std::mem::zeroed() };
        device_info.mode = webgpu_sys::WGPUCallbackMode_AllowProcessEvents;
        device_info.callback = Some(on_device);
        device_info.userdata1 = device_user_data;
        // SAFETY: adapter is live, the descriptor outlives the call, and the
        // slot outlives the request whatever the wait below does.
        let future = unsafe {
            webgpu_sys::wgpuAdapterRequestDevice(self.adapter, &device_descriptor, device_info)
        };
        await_future(self.instance, future)?;
        self.device = device_slot
            .take()
            .filter(|device| !device.is_null())
            .ok_or("this browser provided no WebGPU device")?;
        Ok(self)
    }

    pub(super) fn descriptor(&self) -> WebgpuContextDescriptor {
        WebgpuContextDescriptor {
            device: self.device.cast::<std::ffi::c_void>(),
            instance: self.instance.cast::<std::ffi::c_void>(),
            // Null asks the session for the device's default queue, which is
            // what a browser host without a queue of its own hands over.
            queue: std::ptr::null_mut(),
        }
    }
}

impl Drop for WebGpuTestContext {
    /// Destroys rather than only releasing the device: emdawnwebgpu takes a
    /// runtime keepalive per device and returns it when `device.lost` settles,
    /// which destroying is what resolves. Releasing alone leaves the keepalive
    /// standing.
    ///
    /// Each browser test runs in a process of its own and requests one device,
    /// so no later request on this thread waits on the continuations that the
    /// destroy queues. A test's map can outlive this fixture and hold the last
    /// keepalive; the entry point forces the exit for that one, see
    /// bindings/rust/emscripten_proxy_main.c.
    fn drop(&mut self) {
        // SAFETY: these handles are this value's and are released exactly once.
        unsafe {
            if !self.device.is_null() {
                webgpu_sys::wgpuDeviceDestroy(self.device);
                webgpu_sys::wgpuDeviceRelease(self.device);
            }
            if !self.adapter.is_null() {
                webgpu_sys::wgpuAdapterRelease(self.adapter);
            }
            if !self.instance.is_null() {
                webgpu_sys::wgpuInstanceRelease(self.instance);
            }
        }
    }
}

/// A caller-owned WebGPU texture and view, the way a host that allocates its own
/// render target hands one over.
pub(super) struct WebGpuBorrowedTexture {
    texture: webgpu_sys::WGPUTexture,
    texture_view: webgpu_sys::WGPUTextureView,
    format: u32,
    width: u32,
    height: u32,
}

impl WebGpuBorrowedTexture {
    pub(super) fn new(
        context: &WebGpuTestContext,
        width: u32,
        height: u32,
    ) -> std::result::Result<Self, Box<dyn StdError>> {
        let format = webgpu_sys::WGPUTextureFormat_RGBA8Unorm;
        let mut descriptor: webgpu_sys::WGPUTextureDescriptor = unsafe { std::mem::zeroed() };
        // Render attachment because the session draws into it, and texture
        // binding because a host samples it afterwards.
        descriptor.usage = webgpu_sys::WGPUTextureUsage_RenderAttachment
            | webgpu_sys::WGPUTextureUsage_TextureBinding
            | webgpu_sys::WGPUTextureUsage_CopySrc;
        descriptor.dimension = webgpu_sys::WGPUTextureDimension_2D;
        descriptor.size = webgpu_sys::WGPUExtent3D {
            width,
            height,
            depthOrArrayLayers: 1,
        };
        descriptor.format = format;
        descriptor.mipLevelCount = 1;
        descriptor.sampleCount = 1;

        // SAFETY: the device is live and descriptor is live for the call.
        let texture = unsafe { webgpu_sys::wgpuDeviceCreateTexture(context.device, &descriptor) };
        if texture.is_null() {
            return Err("creating the fixture's WebGPU texture failed".into());
        }
        // SAFETY: texture is the handle just created.
        let texture_view = unsafe { webgpu_sys::wgpuTextureCreateView(texture, std::ptr::null()) };
        if texture_view.is_null() {
            // SAFETY: releasing the texture this call created.
            unsafe { webgpu_sys::wgpuTextureRelease(texture) };
            return Err("creating the fixture's WebGPU texture view failed".into());
        }

        Ok(Self {
            texture,
            texture_view,
            format,
            width,
            height,
        })
    }

    pub(super) fn descriptor(
        &self,
        extent: RenderTargetExtent,
        context: &WebGpuTestContext,
    ) -> WebgpuBorrowedTextureDescriptor {
        WebgpuBorrowedTextureDescriptor {
            extent,
            physical_width: self.width,
            physical_height: self.height,
            context: context.descriptor(),
            textures: vec![WebgpuBorrowedTexture::new(
                self.texture.cast::<std::ffi::c_void>(),
                self.texture_view.cast::<std::ffi::c_void>(),
            )],
            format: self.format,
        }
    }
}

unsafe extern "C" fn on_buffer_mapped(
    status: webgpu_sys::WGPUMapAsyncStatus,
    _message: webgpu_sys::WGPUStringView,
    user_data: *mut std::ffi::c_void,
    _reserved: *mut std::ffi::c_void,
) {
    // SAFETY: user_data is this request's status slot, delivered once.
    unsafe { fill_callback_slot::<webgpu_sys::WGPUMapAsyncStatus>(user_data, status) };
}

impl WebGpuBorrowedTexture {
    /// Reads this texture back, so a test can tell what the session drew into
    /// it rather than only that the call returned.
    ///
    /// Independent of the readback the C API offers: this waits on the
    /// fixture's own instance, which asked for timed waits, so a blank result
    /// here means the frame is blank rather than that a wait went wrong.
    pub(super) fn read_rgba(
        &self,
        context: &WebGpuTestContext,
    ) -> std::result::Result<Vec<u8>, Box<dyn StdError>> {
        // WebGPU pads every row of a texture-to-buffer copy to 256 bytes.
        const ROW_ALIGNMENT: u32 = 256;
        let row_stride = self.width * 4;
        let aligned_row_stride = row_stride.div_ceil(ROW_ALIGNMENT) * ROW_ALIGNMENT;
        let mapped_size = u64::from(aligned_row_stride) * u64::from(self.height);

        // SAFETY: every handle below is live for the calls it is passed to, and
        // each descriptor outlives its own call.
        unsafe {
            let mut buffer_descriptor: webgpu_sys::WGPUBufferDescriptor = std::mem::zeroed();
            buffer_descriptor.size = mapped_size;
            buffer_descriptor.usage =
                webgpu_sys::WGPUBufferUsage_CopyDst | webgpu_sys::WGPUBufferUsage_MapRead;
            let staging = webgpu_sys::wgpuDeviceCreateBuffer(context.device, &buffer_descriptor);
            if staging.is_null() {
                return Err("creating the fixture's readback buffer failed".into());
            }

            let encoder =
                webgpu_sys::wgpuDeviceCreateCommandEncoder(context.device, std::ptr::null());
            let mut source: webgpu_sys::WGPUTexelCopyTextureInfo = std::mem::zeroed();
            source.texture = self.texture;
            source.aspect = webgpu_sys::WGPUTextureAspect_All;
            let mut target: webgpu_sys::WGPUTexelCopyBufferInfo = std::mem::zeroed();
            target.buffer = staging;
            target.layout.bytesPerRow = aligned_row_stride;
            target.layout.rowsPerImage = self.height;
            let extent = webgpu_sys::WGPUExtent3D {
                width: self.width,
                height: self.height,
                depthOrArrayLayers: 1,
            };
            webgpu_sys::wgpuCommandEncoderCopyTextureToBuffer(encoder, &source, &target, &extent);
            let commands = webgpu_sys::wgpuCommandEncoderFinish(encoder, std::ptr::null());
            webgpu_sys::wgpuCommandEncoderRelease(encoder);
            let queue = webgpu_sys::wgpuDeviceGetQueue(context.device);
            webgpu_sys::wgpuQueueSubmit(queue, 1, &commands);
            webgpu_sys::wgpuCommandBufferRelease(commands);
            webgpu_sys::wgpuQueueRelease(queue);

            let (status_slot, status_user_data) = callback_slot::<webgpu_sys::WGPUMapAsyncStatus>();
            let mut callback_info: webgpu_sys::WGPUBufferMapCallbackInfo = std::mem::zeroed();
            callback_info.mode = webgpu_sys::WGPUCallbackMode_WaitAnyOnly;
            callback_info.callback = Some(on_buffer_mapped);
            callback_info.userdata1 = status_user_data;
            let future = webgpu_sys::wgpuBufferMapAsync(
                staging,
                webgpu_sys::WGPUMapMode_Read,
                0,
                mapped_size as usize,
                callback_info,
            );
            // Releases the buffer however the wait ends, which a `?` straight
            // out of here would skip.
            let waited = await_future(context.instance, future);
            let status = status_slot.take();
            if waited.is_err() || status != Some(webgpu_sys::WGPUMapAsyncStatus_Success) {
                webgpu_sys::wgpuBufferRelease(staging);
                waited?;
                return Err(
                    format!("mapping the fixture's readback buffer failed ({status:?})").into(),
                );
            }

            let mapped =
                webgpu_sys::wgpuBufferGetConstMappedRange(staging, 0, mapped_size as usize)
                    .cast::<u8>();
            if mapped.is_null() {
                webgpu_sys::wgpuBufferUnmap(staging);
                webgpu_sys::wgpuBufferRelease(staging);
                return Err("the fixture's readback buffer produced no mapped range".into());
            }
            let mut pixels = Vec::with_capacity((row_stride * self.height) as usize);
            for row in 0..self.height {
                let offset = (row * aligned_row_stride) as usize;
                pixels.extend_from_slice(std::slice::from_raw_parts(
                    mapped.add(offset),
                    row_stride as usize,
                ));
            }
            webgpu_sys::wgpuBufferUnmap(staging);
            webgpu_sys::wgpuBufferRelease(staging);
            Ok(pixels)
        }
    }
}

impl Drop for WebGpuBorrowedTexture {
    fn drop(&mut self) {
        // SAFETY: both handles are this value's and are released exactly once.
        unsafe {
            webgpu_sys::wgpuTextureViewRelease(self.texture_view);
            webgpu_sys::wgpuTextureRelease(self.texture);
        }
    }
}
