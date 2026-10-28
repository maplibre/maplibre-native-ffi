/// The process's one delivery queue, and the thread that dispatches it.
///
/// Every registration and completion that runs Python code names this queue,
/// so native threads never call into the interpreter: native code queues each
/// call, and the dispatcher thread runs it with the interpreter attached. The
/// thread starts with the first registration or completion, and the hook that
/// stops it at interpreter shutdown registers then.
struct GeneratedDispatcher {
    queue: sys::mln_delivery_queue,
    wake: &'static GeneratedDispatcherWake,
}

/// What the queue's wake raises, and what the dispatcher thread waits on.
struct GeneratedDispatcherWake {
    state: Mutex<GeneratedDispatcherState>,
    signal: std::sync::Condvar,
}

#[derive(Default)]
struct GeneratedDispatcherState {
    woken: bool,
    stopping: bool,
    thread: Option<std::thread::JoinHandle<()>>,
}

/// The dispatcher, once it starts. It stays after it stops, so later calls
/// fail instead of starting another.
static GENERATED_DISPATCHER: Mutex<Option<&'static GeneratedDispatcher>> = Mutex::new(None);

thread_local! {
    #[cfg_attr(any(target_os = "android", target_env = "ohos"), allow(clippy::missing_const_for_thread_local))]
    static GENERATED_ON_DISPATCHER: std::cell::Cell<bool> = const { std::cell::Cell::new(false) };
}

fn generated_lock<T>(mutex: &Mutex<T>) -> MutexGuard<'_, T> {
    mutex.lock().unwrap_or_else(|p| p.into_inner())
}

/// Raises the dispatcher's signal. Native calls this from any thread, and it
/// returns without waiting for the dispatcher.
unsafe extern "C" fn generated_dispatcher_wake(user_data: *mut c_void) {
    // SAFETY: the queue's wake carries its signal, which is never freed.
    let wake = unsafe { &*user_data.cast::<GeneratedDispatcherWake>() };
    generated_lock(&wake.state).woken = true;
    wake.signal.notify_one();
}

impl GeneratedDispatcher {
    /// Runs every call queued so far after each wake, until shutdown stops it.
    fn serve(&self) {
        loop {
            {
                let mut state = generated_lock(&self.wake.state);
                while !state.woken && !state.stopping {
                    state = self
                        .wake
                        .signal
                        .wait(state)
                        .unwrap_or_else(|p| p.into_inner());
                }
                if state.stopping {
                    return;
                }
                state.woken = false;
            }
            // SAFETY: the queue is a valid handle, which shutdown may have
            // released; dispatch then runs nothing. Only this thread
            // dispatches it, so the call never reports busy.
            unsafe { sys::mln_delivery_queue_dispatch(self.queue, ptr::null_mut()) };
        }
    }
}

/// The queue that every Python registration and completion names, which
/// starts the dispatcher on first use.
fn generated_dispatch_queue() -> PyResult<sys::mln_delivery_queue> {
    let mut started = generated_lock(&GENERATED_DISPATCHER);
    if let Some(dispatcher) = *started {
        if generated_lock(&dispatcher.wake.state).stopping {
            return Err(invalid_state_error(
                "the binding's dispatcher stopped at interpreter shutdown",
            ));
        }
        return Ok(dispatcher.queue);
    }
    // The queue never releases its wake's context, so it is leaked, as is the
    // dispatcher that outlives the interpreter.
    let wake: &'static GeneratedDispatcherWake = Box::leak(Box::new(GeneratedDispatcherWake {
        state: Mutex::new(GeneratedDispatcherState::default()),
        signal: std::sync::Condvar::new(),
    }));
    let options = sys::mln_delivery_queue_options {
        size: std::mem::size_of::<sys::mln_delivery_queue_options>() as u32,
        wake: sys::mln_wake {
            callback: Some(generated_dispatcher_wake),
            user_data: ptr::from_ref(wake).cast_mut().cast(),
            release_user_data: None,
            queue: sys::mln_delivery_queue(0),
        },
        release_abandoned_user_data: None,
    };
    let mut queue = sys::mln_delivery_queue(0);
    // SAFETY: options and queue are valid for the call, and the wake's
    // context is never freed.
    maplibre_core::check(|diagnostic| unsafe {
        sys::mln_delivery_queue_create(&options, &mut queue, diagnostic)
    })
    .map_err(map_error)?;
    let dispatcher: &'static GeneratedDispatcher =
        Box::leak(Box::new(GeneratedDispatcher { queue, wake }));
    let thread = std::thread::Builder::new()
        .name("maplibre-dispatcher".into())
        .spawn(move || {
            GENERATED_ON_DISPATCHER.set(true);
            // One thread state serves every call, attached only while a call
            // runs Python code.
            if Python::try_attach(|py| py.detach(|| dispatcher.serve())).is_none() {
                dispatcher.serve();
            }
        });
    let thread = match thread {
        Ok(thread) => thread,
        Err(error) => {
            // SAFETY: no registration names the queue yet.
            unsafe { sys::mln_delivery_queue_release(queue) };
            return Err(pyo3::exceptions::PyRuntimeError::new_err(format!(
                "could not start the binding's dispatcher: {error}"
            )));
        }
    };
    generated_lock(&wake.state).thread = Some(thread);
    *started = Some(dispatcher);
    drop(started);
    Python::attach(|py| {
        let registered = wrap_pyfunction!(generated_stop_dispatcher, py)
            .and_then(|hook| py.import("atexit")?.call_method1("register", (hook,)));
        if let Err(error) = registered {
            error.write_unraisable(py, None);
        }
    });
    Ok(queue)
}

/// Whether the calling thread is the dispatcher thread, where waiting for a
/// pending completion would wait forever.
#[pyfunction]
#[pyo3(name = "_on_dispatcher_thread")]
fn generated_on_dispatcher_thread() -> bool {
    GENERATED_ON_DISPATCHER.get()
}

/// Stops the dispatcher at interpreter shutdown, before the interpreter
/// finalizes, so no Python code runs after it. Releasing the queue abandons
/// every pending call and refuses later ones; the call that runs when the hook
/// does finishes first. Each live render session then ends its graphics calls
/// before the host's own exit teardown can destroy its graphics objects.
#[pyfunction]
fn generated_stop_dispatcher(py: Python<'_>) {
    let Some(dispatcher) = *generated_lock(&GENERATED_DISPATCHER) else {
        return;
    };
    let thread = {
        let mut state = generated_lock(&dispatcher.wake.state);
        state.stopping = true;
        state.thread.take()
    };
    dispatcher.wake.signal.notify_one();
    // The running call may need the interpreter to finish.
    py.detach(move || {
        // Released before the join, so the running dispatch abandons the
        // calls it has not started.
        // SAFETY: the queue is released once, and stopping refuses new uses.
        unsafe { sys::mln_delivery_queue_release(dispatcher.queue) };
        if let Some(thread) = thread {
            let _ = thread.join();
        }
        generated_abandon_sessions_at_exit();
    });
}
