use std::cell::Cell;
use std::sync::{OnceLock, mpsc};

thread_local! {
    #[cfg_attr(any(target_os = "android", target_env = "ohos"), allow(clippy::missing_const_for_thread_local))]
    static DEPTH: Cell<usize> = const { Cell::new(0) };
}

/// Marks host code running on a native callback stack.
pub struct CallbackScope(std::marker::PhantomData<std::rc::Rc<()>>);
impl CallbackScope {
    pub fn enter() -> Self {
        DEPTH.set(DEPTH.get() + 1);
        Self(std::marker::PhantomData)
    }
}
impl Drop for CallbackScope {
    fn drop(&mut self) {
        DEPTH.set(DEPTH.get() - 1);
    }
}

/// Runs finalization outside the native callback stack.
pub fn finalize(action: impl FnOnce() + Send + 'static) {
    if DEPTH.get() == 0 {
        action();
        return;
    }
    static QUEUE: OnceLock<mpsc::Sender<Box<dyn FnOnce() + Send>>> = OnceLock::new();
    let queue = QUEUE.get_or_init(|| {
        let (sender, receiver) = mpsc::channel::<Box<dyn FnOnce() + Send>>();
        std::thread::Builder::new()
            .name("maplibre-finalize".into())
            .spawn(move || {
                for action in receiver {
                    let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(action));
                }
            })
            .expect("start native finalization worker");
        sender
    });
    let _ = queue.send(Box::new(action));
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn finalization_inside_a_callback_runs_on_another_thread() {
        let _callback = CallbackScope::enter();
        let (sender, receiver) = std::sync::mpsc::channel();
        finalize(move || sender.send(std::thread::current().id()).unwrap());
        let thread = receiver
            .recv_timeout(std::time::Duration::from_secs(10))
            .expect("finalizer must progress while the callback thread waits");
        assert_ne!(thread, std::thread::current().id());
    }
}
