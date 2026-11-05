// Callback registrations (wakes, completions) and delivery queues.
//
// A registration without a queue runs on the library thread that produces the
// call; with one, the call waits in the queue until the caller dispatches it,
// and the queue's wake says when the empty queue gains a call. On a runtime
// thread, calls wait in its outbox until the runtime's state is free, so a
// callback can call the library back.

use std::cell::RefCell;
use std::collections::VecDeque;
use std::sync::{Arc, Mutex};

use crate::base::{Failure, Result, UserData, boundary, put, view};
use crate::handles::{self, Object};
use crate::types::*;

pub type Call = Box<dyn FnOnce() + Send + 'static>;

thread_local! {
    /// The calls a runtime thread delivers once its state is free.
    static OUTBOX: RefCell<Option<Vec<Call>>> = const { RefCell::new(None) };
}

/// Makes this thread a runtime thread, whose deliveries wait for [`flush`].
pub fn start_outbox() {
    OUTBOX.with(|o| *o.borrow_mut() = Some(Vec::new()));
}

/// Runs the calls in this runtime thread's outbox.
pub fn flush() {
    loop {
        let calls = OUTBOX.with(|o| {
            o.borrow_mut()
                .as_mut()
                .map(std::mem::take)
                .unwrap_or_default()
        });
        if calls.is_empty() {
            return;
        }
        for call in calls {
            call();
        }
    }
}

/// Runs `call` on this thread (after the runtime's state is free) or hands it
/// to `queue`. A call for a queue that is gone never runs.
pub fn deliver(queue: u64, call: Call) {
    if queue != 0 {
        if let Some(Object::Queue(queue)) = handles::get(queue) {
            queue.push(call);
        }
        return;
    }
    let call = OUTBOX.with(|o| match o.borrow_mut().as_mut() {
        Some(outbox) => {
            outbox.push(call);
            None
        }
        None => Some(call),
    });
    if let Some(call) = call {
        call();
    }
}

fn release_later(queue: u64, release: mln_user_data_release, user_data: UserData) {
    if let Some(release) = release {
        deliver(
            queue,
            Box::new(move || {
                // SAFETY: the registration's release, called once.
                unsafe { release(user_data.get()) }
            }),
        );
    }
}

/// An `mln_wake`.
#[derive(Clone, Copy, Debug)]
pub struct Wake {
    callback: unsafe extern "C" fn(*mut std::ffi::c_void),
    user_data: UserData,
    release_user_data: mln_user_data_release,
    queue: u64,
}

impl Wake {
    pub fn from_c(wake: &mln_wake) -> Option<Self> {
        wake.callback.map(|callback| Self {
            callback,
            user_data: UserData(wake.user_data),
            release_user_data: wake.release_user_data,
            queue: wake.queue.0,
        })
    }

    pub fn signal(&self) {
        let wake = *self;
        deliver(
            self.queue,
            Box::new(move || {
                // SAFETY: the caller registered this callback and user_data.
                unsafe { (wake.callback)(wake.user_data.get()) }
            }),
        );
    }

    pub fn release(self) {
        release_later(self.queue, self.release_user_data, self.user_data);
    }
}

/// What a completion reports.
pub enum Value {
    None,
    /// An `mln_buffer_view` of this text, terminated.
    Text(String),
    /// An `mln_texture_readback_result` of these pixels.
    Readback {
        pixels: Vec<u8>,
        info: mln_texture_image_info,
    },
    /// A plain C struct, as its bytes in 8-byte words so doubles align.
    Struct {
        words: Vec<u64>,
        size: usize,
    },
}

impl Value {
    pub fn of<T: Copy>(value: T) -> Self {
        let size = std::mem::size_of::<T>();
        let mut words = vec![0u64; size.div_ceil(8)];
        // SAFETY: `words` holds `size` bytes, 8-aligned; T is plain C data.
        unsafe { words.as_mut_ptr().cast::<T>().write(value) };
        Value::Struct { words, size }
    }
}

/// An `mln_completion`: runs once, then releases its user data.
pub struct Completion {
    callback: unsafe extern "C" fn(*mut std::ffi::c_void, *const mln_completion_result),
    user_data: UserData,
    release_user_data: mln_user_data_release,
    queue: u64,
}

impl Completion {
    /// # Safety
    /// `completion` is null or a readable `mln_completion`.
    pub unsafe fn from_c(completion: *const mln_completion) -> Result<Option<Self>> {
        if completion.is_null() {
            return Ok(None);
        }
        // SAFETY: the caller's contract.
        let c = unsafe { crate::base::sized(completion, "completion")? };
        let callback = c
            .callback
            .ok_or_else(|| Failure::argument("completion.callback is null"))?;
        Ok(Some(Self {
            callback,
            user_data: UserData(c.user_data),
            release_user_data: c.release_user_data,
            queue: c.queue.0,
        }))
    }

    /// Delivers the result: `generation` is the map generation that shows it.
    pub fn complete(self, result: Result<Value>, generation: u64) {
        let Completion {
            callback,
            user_data,
            release_user_data,
            queue,
        } = self;
        deliver(
            queue,
            Box::new(move || {
                let (status, mut message, mut value) = match result {
                    Ok(value) => (MLN_STATUS_OK, String::new(), value),
                    Err(failure) => (failure.status, failure.message, Value::None),
                };
                let bytes_view;
                let readback;
                let diagnostic = crate::base::lend(&mut message);
                let (value, value_size, value_count): (*const std::ffi::c_void, usize, usize) =
                    match &mut value {
                        Value::None => (std::ptr::null(), 0, 0),
                        Value::Text(text) => {
                            bytes_view = crate::base::lend(text);
                            (
                                std::ptr::from_ref(&bytes_view).cast(),
                                std::mem::size_of::<mln_buffer_view>(),
                                1,
                            )
                        }
                        Value::Readback { pixels, info } => {
                            readback = mln_texture_readback_result {
                                data: view(pixels),
                                info: *info,
                            };
                            (
                                std::ptr::from_ref(&readback).cast(),
                                std::mem::size_of::<mln_texture_readback_result>(),
                                1,
                            )
                        }
                        Value::Struct { words, size } => (words.as_ptr().cast(), *size, 1),
                    };
                let result = mln_completion_result {
                    size: std::mem::size_of::<mln_completion_result>() as u32,
                    status,
                    value_size: value_size as u32,
                    generation,
                    render_update_generation: generation,
                    diagnostic,
                    value,
                    value_count,
                };
                // SAFETY: the caller registered this callback and user_data;
                // `result` and what it points at live until it returns.
                unsafe { callback(user_data.get(), &result) };
                if let Some(release) = release_user_data {
                    // SAFETY: the registration's release, called once.
                    unsafe { release(user_data.get()) };
                }
            }),
        );
    }
}

/// An `mln_delivery_queue`.
pub struct DeliveryQueue {
    calls: Mutex<VecDeque<Call>>,
    dispatching: Mutex<()>,
    wake: Option<Wake>,
}

impl DeliveryQueue {
    fn push(&self, call: Call) {
        let was_empty = {
            let mut calls = self.calls.lock().unwrap();
            calls.push_back(call);
            calls.len() == 1
        };
        if was_empty && let Some(wake) = &self.wake {
            wake.signal();
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_delivery_queue_create(
    options: *const mln_delivery_queue_options,
    out_queue: *mut mln_delivery_queue,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let wake = if options.is_null() {
                None
            } else {
                Wake::from_c(&crate::base::sized(options, "options")?.wake)
            };
            let queue = DeliveryQueue {
                calls: Mutex::default(),
                dispatching: Mutex::default(),
                wake,
            };
            let handle = handles::insert(Object::Queue(Arc::new(queue)));
            put(out_queue, mln_delivery_queue(handle), "out_queue")
        })
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_delivery_queue_release(queue: mln_delivery_queue) {
    if let Some(Object::Queue(queue)) = handles::remove(queue.0) {
        queue.calls.lock().unwrap().clear();
        if let Some(wake) = queue.wake {
            wake.release();
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_delivery_queue_dispatch(
    queue: mln_delivery_queue,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointer.
    unsafe {
        boundary(out_diagnostic, || {
            let queue = handles::queue(queue.0)?;
            let Ok(_dispatching) = queue.dispatching.try_lock() else {
                return Err(Failure::new(
                    MLN_STATUS_BUSY,
                    "another dispatch of the queue is running",
                ));
            };
            loop {
                let Some(call) = queue.calls.lock().unwrap().pop_front() else {
                    return Ok(());
                };
                call();
            }
        })
    }
}
