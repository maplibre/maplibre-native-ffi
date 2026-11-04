// Ports maplibre-native-ffi src/delivery (delivery queues and wakes) and
// src/completion (how an asynchronous call reports its result).
//
// A callback registration without a queue runs on the library thread that
// produces the call; with a queue, the call waits in the queue until the
// caller dispatches it, and the queue's wake tells the caller when the empty
// queue gains a call. After a registration's last call, its
// `release_user_data` runs (through the same queue).

use crate::diagnostics::{Failure, boundary};
use crate::handles::{self, Object};
use crate::types::*;
use std::collections::VecDeque;
use std::ffi::c_void;
use std::sync::{Arc, Mutex};

/// A call that a delivery queue holds.
pub type Call = Box<dyn FnOnce() + Send + 'static>;

/// A caller's `user_data` pointer, which crosses threads with its callback.
#[derive(Clone, Copy)]
pub struct UserData(pub *mut c_void);
// SAFETY: the C API lets the library call a registration's callback, with
// its user_data, on any library thread (or the dispatching thread).
unsafe impl Send for UserData {}
unsafe impl Sync for UserData {}

impl UserData {
    /// The pointer. A method, so a closure that calls it captures the
    /// `Send` wrapper rather than the raw pointer field.
    pub fn get(self) -> *mut c_void {
        self.0
    }
}

/// `mln_wake`.
#[derive(Clone, Copy)]
pub struct Wake {
    callback: Option<unsafe extern "C" fn(*mut c_void)>,
    user_data: UserData,
    release_user_data: mln_user_data_release,
    queue: u64,
}

impl Wake {
    pub fn from_c(wake: &mln_wake) -> Option<Wake> {
        wake.callback.map(|callback| Wake {
            callback: Some(callback),
            user_data: UserData(wake.user_data),
            release_user_data: wake.release_user_data,
            queue: wake.queue.0,
        })
    }

    pub fn signal(&self) {
        let wake = *self;
        deliver(self.queue, Box::new(move || {
            if let Some(callback) = wake.callback {
                // SAFETY: the caller registered this callback and user_data.
                unsafe { callback(wake.user_data.get()) };
            }
        }));
    }

    #[allow(dead_code)] // TODO(ffi): wakes released with their registrations.
    pub fn release(self) {
        if let Some(release) = self.release_user_data {
            let user_data = self.user_data;
            deliver(self.queue, Box::new(move || {
                // SAFETY: the registration's release, called once.
                unsafe { release(user_data.get()) };
            }));
        }
    }
}

/// `mln_delivery_queue`.
pub struct DeliveryQueue {
    calls: Mutex<VecDeque<Call>>,
    dispatching: Mutex<()>,
    wake: Option<Wake>,
}

impl DeliveryQueue {
    fn push(&self, call: Call) {
        let was_empty = {
            let mut calls = self.calls.lock().unwrap();
            let was_empty = calls.is_empty();
            calls.push_back(call);
            was_empty
        };
        if was_empty {
            if let Some(wake) = &self.wake {
                wake.signal();
            }
        }
    }
}

/// Runs `call` now (no queue) or hands it to the queue `queue`. A call for a
/// queue that is gone never runs.
pub fn deliver(queue: u64, call: Call) {
    if queue == 0 {
        call();
        return;
    }
    if let Some(Object::Queue(queue)) = handles::get(queue) {
        queue.push(call);
    }
}

/// A registered `mln_completion`, which runs once and then releases its
/// user_data.
pub struct Completion {
    callback: mln_completion_callback,
    user_data: UserData,
    release_user_data: mln_user_data_release,
    queue: u64,
}

/// The value that a completion reports.
pub enum Value {
    None,
    /// An `mln_buffer_view` of these bytes.
    BufferView(Vec<u8>),
    /// An `mln_texture_readback_result` of these pixels.
    Readback { pixels: Vec<u8>, info: mln_texture_image_info },
    /// An `mln_camera_query_result`.
    Camera(mln_camera_query_result),
}

impl Completion {
    /// Validates a caller's completion, which must not be null.
    ///
    /// # Safety
    /// As for `optional`.
    pub unsafe fn required(completion: *const mln_completion) -> Result<Completion, Failure> {
        // SAFETY: the caller's contract.
        unsafe { Completion::optional(completion) }?
            .ok_or_else(|| Failure::invalid_argument("completion must not be null"))
    }

    /// Validates a caller's completion. `Ok(None)` is a null completion,
    /// which only commands accept.
    ///
    /// # Safety
    /// `completion` is null or points at a readable `mln_completion`.
    pub unsafe fn optional(completion: *const mln_completion) -> Result<Option<Completion>, Failure> {
        if completion.is_null() {
            return Ok(None);
        }
        // SAFETY: the caller's contract.
        let Some(completion) = (unsafe { read_sized(completion) }) else {
            return Err(Failure::invalid_argument("completion.size is smaller than mln_completion"));
        };
        if completion.callback.is_none() {
            return Err(Failure::invalid_argument("completion.callback must not be null"));
        }
        if completion.queue.0 != 0 {
            handles::queue(completion.queue)?;
        }
        Ok(Some(Completion {
            callback: completion.callback,
            user_data: UserData(completion.user_data),
            release_user_data: completion.release_user_data,
            queue: completion.queue.0,
        }))
    }

    pub fn ok(self) {
        self.finish(MLN_STATUS_OK, 0, String::new(), Value::None);
    }

    pub fn ok_at(self, generation: u64) {
        self.finish(MLN_STATUS_OK, generation, String::new(), Value::None);
    }

    pub fn ok_with(self, generation: u64, value: Value) {
        self.finish(MLN_STATUS_OK, generation, String::new(), value);
    }

    pub fn fail(self, status: i32, message: impl Into<String>) {
        self.finish(status, 0, message.into(), Value::None);
    }

    pub fn finish(self, status: i32, generation: u64, message: String, value: Value) {
        let Completion { callback, user_data, release_user_data, queue } = self;
        deliver(queue, Box::new(move || {
            // The value's C struct and what it points at, which live until
            // the callback returns.
            let text;
            let buffer_view;
            let readback;
            let (value_ptr, value_size, value_count): (*const c_void, usize, usize) = match value {
                Value::None => (std::ptr::null(), 0, 0),
                Value::BufferView(bytes) => {
                    text = LentText::new(bytes);
                    buffer_view = mln_buffer_view::of_text(&text);
                    ((&raw const buffer_view).cast(), size_of::<mln_buffer_view>(), 1)
                }
                Value::Readback { ref pixels, info } => {
                    readback = mln_texture_readback_result { data: mln_buffer_view::of(pixels), info };
                    ((&raw const readback).cast(), size_of::<mln_texture_readback_result>(), 1)
                }
                Value::Camera(ref camera) => ((&raw const *camera).cast(), size_of::<mln_camera_query_result>(), 1),
            };
            let disposition = if status == MLN_STATUS_OK {
                MLN_COMMAND_DISPOSITION_COMMITTED
            } else if status == MLN_STATUS_CANCELLED {
                MLN_COMMAND_DISPOSITION_CANCELLED
            } else {
                MLN_COMMAND_DISPOSITION_FAILED
            };
            let diagnostic = LentText::new(message);
            let result = mln_completion_result {
                size: size_of::<mln_completion_result>() as u32,
                status,
                disposition,
                value_size: value_size as u32,
                generation,
                render_update_generation: 0,
                diagnostic: mln_buffer_view::of_text(&diagnostic),
                value: value_ptr,
                value_count,
            };
            if let Some(callback) = callback {
                // SAFETY: the caller registered this callback and user_data;
                // `result` and what it points at live until it returns.
                unsafe { callback(user_data.get(), &result) };
            }
            if let Some(release) = release_user_data {
                // SAFETY: the registration's release, called once.
                unsafe { release(user_data.get()) };
            }
        }));
    }
}

/// # Safety
/// C API contract (delivery.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_delivery_queue_create(
    options: *const mln_delivery_queue_options,
    out_queue: *mut mln_delivery_queue,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let Some(options) = read_sized(options) else {
                return Err(Failure::invalid_argument("options must be an mln_delivery_queue_options"));
            };
            if out_queue.is_null() {
                return Err(Failure::invalid_argument("out_queue must not be null"));
            }
            let queue = Arc::new(DeliveryQueue {
                calls: Mutex::new(VecDeque::new()),
                dispatching: Mutex::new(()),
                wake: Wake::from_c(&options.wake),
            });
            *out_queue = handles::insert(Object::Queue(queue)).into();
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (delivery.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_delivery_queue_release(queue: mln_delivery_queue) {
    if let Some(Object::Queue(queue)) = handles::remove(queue) {
        queue.calls.lock().unwrap().clear();
        if let Some(wake) = queue.wake {
            if let Some(release) = wake.release_user_data {
                // SAFETY: the wake registration's release, called once.
                unsafe { release(wake.user_data.get()) };
            }
        }
    }
}

/// # Safety
/// C API contract (delivery.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_delivery_queue_dispatch(queue: mln_delivery_queue, out_diagnostic: *mut mln_diagnostic) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let queue = handles::queue(queue)?;
            let Ok(_guard) = queue.dispatching.try_lock() else {
                return Err(Failure::new(MLN_STATUS_BUSY, "another dispatch of the queue is running"));
            };
            loop {
                let call = queue.calls.lock().unwrap().pop_front();
                match call {
                    Some(call) => call(),
                    None => break,
                }
            }
            Ok(())
        })
    }
}
