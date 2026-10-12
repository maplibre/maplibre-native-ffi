use std::cell::RefCell;
use std::sync::{OnceLock, mpsc};

struct Policy {
    operations: &'static [&'static str],
    owner: u64,
}
thread_local! {
    // Rust 1.95's Android/OHOS TLS macro wraps this const initializer in a generated
    // non-const function, which triggers the lint on the macro expansion.
    #[cfg_attr(any(target_os = "android", target_env = "ohos"), allow(clippy::missing_const_for_thread_local))]
    static POLICIES: RefCell<Vec<Policy>> = const { RefCell::new(Vec::new()) };
}

/// Restricts native entry while a native callback invokes host code.
pub struct PolicyScope(std::marker::PhantomData<std::rc::Rc<()>>);
impl PolicyScope {
    pub fn enter(operations: &'static [&'static str], owner: u64) -> Self {
        POLICIES.with(|policies| policies.borrow_mut().push(Policy { operations, owner }));
        Self(std::marker::PhantomData)
    }
}
impl Drop for PolicyScope {
    fn drop(&mut self) {
        POLICIES.with(|policies| {
            policies.borrow_mut().pop();
        });
    }
}

/// Checks every enclosing callback's operation and owner contract.
pub fn check(operation: &str, owner: u64) -> crate::Result<()> {
    let accepted = POLICIES.with(|policies| {
        policies
            .borrow()
            .iter()
            .all(|policy| policy.owner == owner && policy.operations.contains(&operation))
    });
    if accepted {
        Ok(())
    } else {
        Err(crate::Error::new(
            crate::ErrorKind::InvalidState,
            None,
            "native operation is unavailable from this callback",
        ))
    }
}

/// Runs finalization outside the native callback stack.
pub fn finalize(action: impl FnOnce() + Send + 'static) {
    if POLICIES.with(|policies| policies.borrow().is_empty()) {
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
    fn nested_callback_contracts_intersect_and_restore_on_unwind() {
        let _outer = PolicyScope::enter(&["complete", "release"], 7);
        assert!(check("complete", 7).is_ok());
        assert!(check("complete", 8).is_err());
        let _ = std::panic::catch_unwind(|| {
            let _inner = PolicyScope::enter(&["release"], 7);
            assert!(check("complete", 7).is_err());
            assert!(check("release", 7).is_ok());
            panic!("host callback panicked");
        });
        assert!(check("complete", 7).is_ok());
    }

    #[test]
    fn finalization_inside_a_callback_runs_on_another_thread() {
        let _callback = PolicyScope::enter(&[], 0);
        let (sender, receiver) = std::sync::mpsc::channel();
        finalize(move || {
            sender
                .send((std::thread::current().id(), check("outside", 0)))
                .unwrap()
        });
        let (thread, policy) = receiver
            .recv_timeout(std::time::Duration::from_secs(10))
            .expect("finalizer must progress while the callback thread waits");
        assert_ne!(thread, std::thread::current().id());
        // The finalization thread runs outside every callback's contract.
        policy.unwrap();
    }
}
