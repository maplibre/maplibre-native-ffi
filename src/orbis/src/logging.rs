// The process's log callback. Orbis reports problems as map events; those
// MapLibre Native logs instead (a tile's or a sprite's failure) come here.

use std::sync::Mutex;

use crate::base::{UserData, boundary, sized};
use crate::delivery::deliver;
use crate::types::*;

struct Handler {
    callback: unsafe extern "C" fn(
        *mut std::ffi::c_void,
        mln_log_severity,
        mln_log_event,
        i64,
        mln_buffer_view,
    ),
    user_data: UserData,
    release_user_data: mln_user_data_release,
    queue: u64,
}

static HANDLER: Mutex<Option<Handler>> = Mutex::new(None);

fn release(old: Option<Handler>) {
    if let Some(Handler {
        user_data,
        release_user_data: Some(release),
        queue,
        ..
    }) = old
    {
        deliver(
            queue,
            Box::new(move || {
                // SAFETY: the registration's release, called once.
                unsafe { release(user_data.get()) }
            }),
        );
    }
}

pub fn warn(message: &str) {
    let handler = HANDLER.lock().unwrap();
    let Some(h) = handler.as_ref() else {
        eprintln!("orbis: {message}");
        return;
    };
    let (callback, user_data) = (h.callback, h.user_data);
    let mut message = message.to_owned();
    deliver(
        h.queue,
        Box::new(move || {
            // SAFETY: the caller's registration; the message lives until it returns.
            unsafe {
                callback(
                    user_data.get(),
                    MLN_LOG_SEVERITY_WARNING,
                    MLN_LOG_EVENT_GENERAL,
                    0,
                    crate::base::lend(&mut message),
                )
            }
        }),
    );
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_log_set_callback(
    handler: *const mln_log_handler,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let h = sized(handler, "handler")?;
            let callback = h
                .callback
                .ok_or_else(|| crate::base::Failure::argument("handler.callback is null"))?;
            let new = Handler {
                callback,
                user_data: UserData(h.user_data),
                release_user_data: h.release_user_data,
                queue: h.queue.0,
            };
            release(HANDLER.lock().unwrap().replace(new));
            Ok(())
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_log_clear_callback(out_diagnostic: *mut mln_diagnostic) -> mln_status {
    // SAFETY: the caller's pointer.
    unsafe {
        boundary(out_diagnostic, || {
            release(HANDLER.lock().unwrap().take());
            Ok(())
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_log_set_async_severity_mask(
    _mask: mln_log_severity_flags,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // Every message is delivered as it's raised; there is no synchronous path
    // to choose against.
    // SAFETY: the caller's pointer.
    unsafe { boundary(out_diagnostic, || Ok(())) }
}
