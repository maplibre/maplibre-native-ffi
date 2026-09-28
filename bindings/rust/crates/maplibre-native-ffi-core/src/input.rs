use std::any::Any;

/// Owns temporary C input allocations through native submission.
#[derive(Default)]
pub struct InputArena {
    roots: Vec<Box<dyn Any>>,
    registrations: Vec<PendingRegistration>,
}

struct PendingRegistration {
    pointer: *mut std::ffi::c_void,
    release: unsafe extern "C" fn(*mut std::ffi::c_void),
}

impl Drop for InputArena {
    fn drop(&mut self) {
        for registration in self.registrations.drain(..) {
            // SAFETY: rejected submission left each pending root owned by this arena.
            unsafe { (registration.release)(registration.pointer) };
        }
    }
}

impl InputArena {
    /// Stores a pending native registration.
    ///
    /// # Safety
    /// `release` must reclaim exactly one box of `T` without unwinding.
    pub unsafe fn registration<T: 'static>(
        &mut self,
        state: T,
        release: unsafe extern "C" fn(*mut std::ffi::c_void),
    ) -> *mut std::ffi::c_void {
        let pointer = Box::into_raw(Box::new(state)).cast();
        self.registrations
            .push(PendingRegistration { pointer, release });
        pointer
    }

    /// Transfers pending callback roots after native accepts their registration.
    pub fn accept_registrations(&mut self) {
        self.registrations.clear();
    }

    pub fn store<T: 'static>(&mut self, value: T) -> *const T {
        let value = Box::new(value);
        let pointer = &*value as *const T;
        self.roots.push(value);
        pointer
    }

    pub fn array<T: 'static>(&mut self, values: Vec<T>) -> *const T {
        let pointer = values.as_ptr();
        self.roots.push(Box::new(values));
        pointer
    }

    pub fn c_string(&mut self, value: &str) -> crate::Result<*const std::ffi::c_char> {
        let value = crate::string::c_string(value)?;
        let pointer = value.as_ptr();
        self.roots.push(Box::new(value));
        Ok(pointer)
    }
}

/// Borrows a native array after checking its pointer and addressable length.
///
/// # Safety
/// A nonempty array must point to `count` initialized values valid for the borrow.
pub unsafe fn slice<'a, T>(pointer: *const T, count: usize) -> crate::Result<&'a [T]> {
    if count == 0 {
        return Ok(&[]);
    }
    if pointer.is_null() || count > isize::MAX as usize / std::mem::size_of::<T>().max(1) {
        return Err(crate::Error::invalid_argument("invalid native array"));
    }
    Ok(unsafe { std::slice::from_raw_parts(pointer, count) })
}

/// Copies a strided native array whose entries may have unaligned addresses.
/// # Safety
/// The pointer must address `count` initialized entries at the declared stride.
pub unsafe fn strided_values<T: Copy>(
    pointer: *const T,
    count: usize,
    stride: usize,
) -> crate::Result<Vec<T>> {
    if count == 0 {
        return Ok(Vec::new());
    }
    if pointer.is_null()
        || stride < std::mem::size_of::<T>()
        || count > isize::MAX as usize / stride
    {
        return Err(crate::Error::invalid_argument("invalid native stride"));
    }
    Ok((0..count)
        .map(|index| unsafe {
            pointer
                .cast::<u8>()
                .add(index * stride)
                .cast::<T>()
                .read_unaligned()
        })
        .collect())
}
/// Copies a UTF-8 range from a native batch's shared message storage.
/// # Safety
/// The data pointer must address `size` readable bytes.
pub unsafe fn arena_string(
    data: *const u8,
    size: usize,
    offset: usize,
    length: usize,
) -> crate::Result<String> {
    let end = offset
        .checked_add(length)
        .ok_or_else(|| crate::Error::invalid_argument("native message range overflow"))?;
    let bytes = unsafe { slice(data, size) }?;
    let value = bytes.get(offset..end).ok_or_else(|| {
        crate::Error::invalid_argument("native message range is outside its batch")
    })?;
    String::from_utf8(value.to_vec())
        .map_err(|_| crate::Error::invalid_argument("native message is invalid UTF-8"))
}

#[cfg(test)]
mod tests {
    use super::InputArena;
    use crate::generated::{CanonicalTileId, CustomGeometrySourceOptions};
    use std::sync::{
        Arc,
        atomic::{AtomicUsize, Ordering},
    };

    struct DropCount(Arc<AtomicUsize>);
    impl Drop for DropCount {
        fn drop(&mut self) {
            self.0.fetch_add(1, Ordering::SeqCst);
        }
    }

    #[test]
    fn input_arrays_preserve_empty_presence_and_retain_moved_storage() {
        let mut arena = InputArena::default();
        let empty = arena.array(Vec::<u64>::new());
        assert!(!empty.is_null());
        assert!(empty.is_aligned());
        let values = arena.array(vec![3_u64, 7, 11]);
        // SAFETY: the arena retains the three initialized entries until it drops.
        assert_eq!(unsafe { super::slice(values, 3) }.unwrap(), &[3, 7, 11]);
    }

    #[test]
    fn generated_registration_transfers_only_after_acceptance_and_contains_panics() {
        let drops = Arc::new(AtomicUsize::new(0));
        let calls = Arc::new(AtomicUsize::new(0));
        for accepted in [false, true] {
            let capture = DropCount(Arc::clone(&drops));
            let seen = Arc::clone(&calls);
            let options = CustomGeometrySourceOptions::new(move |tile| {
                let _ = &capture;
                assert_eq!(tile, CanonicalTileId::new(3, 4, 5));
                seen.fetch_add(1, Ordering::SeqCst);
                panic!("contained callback panic");
            });
            let mut arena = InputArena::default();
            let raw = options.to_native(&mut arena).unwrap();
            drop(options);
            if accepted {
                arena.accept_registrations();
            }
            drop(arena);
            if accepted {
                assert_eq!(drops.load(Ordering::SeqCst), 1);
                // SAFETY: accepted registration retains its generated root until release.
                unsafe {
                    raw.fetch_tile.unwrap()(
                        raw.user_data,
                        CanonicalTileId::new(3, 4, 5).to_native(),
                    );
                    raw.release_user_data.unwrap()(raw.user_data);
                }
            }
        }
        assert_eq!(calls.load(Ordering::SeqCst), 1);
        assert_eq!(drops.load(Ordering::SeqCst), 2);
    }
}
