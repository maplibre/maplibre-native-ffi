//! Conversions between this crate's values and their C representations.
//!
//! Generated records and operations convert every field and argument through
//! [`ToNative`] and [`FromNative`], and through the helpers here for the shapes
//! that a Rust type alone does not select: presence fields, counted and
//! strided arrays, and borrowed references.

use std::ffi::{c_char, c_void};

use maplibre_native_ffi_sys as sys;

use crate::{Error, Result};

/// Converts a value into the C representation `N` for one native call.
///
/// Pointers in the result borrow `self` or `arena`, so the result stays valid
/// while both live.
pub(crate) trait ToNative<N> {
    fn to_native(&self, arena: &mut InputArena) -> Result<N>;
}

/// Copies a C value of type `N`, and everything it points to, into an owned
/// value.
pub(crate) trait FromNative<N>: Sized {
    /// # Safety
    ///
    /// Every pointer in `native` must be readable for the lengths its C
    /// declaration states, for the duration of this call.
    unsafe fn from_native(native: N) -> Result<Self>;
}

/// Converts `value` for one call.
pub(crate) fn to_native<N, T: ToNative<N> + ?Sized>(
    value: &T,
    arena: &mut InputArena,
) -> Result<N> {
    value.to_native(arena)
}

/// Copies a C value.
///
/// # Safety
///
/// As for [`FromNative::from_native`].
pub(crate) unsafe fn from_native<N, T: FromNative<N>>(native: N) -> Result<T> {
    // SAFETY: the caller upholds the pointer contract.
    unsafe { <T as FromNative<N>>::from_native(native) }
}

impl<N, T: ToNative<N> + ?Sized> ToNative<N> for &T {
    fn to_native(&self, arena: &mut InputArena) -> Result<N> {
        (**self).to_native(arena)
    }
}

macro_rules! plain {
    ($($type:ty),* $(,)?) => {$(
        impl ToNative<$type> for $type {
            fn to_native(&self, _: &mut InputArena) -> Result<$type> {
                Ok(*self)
            }
        }
        impl FromNative<$type> for $type {
            unsafe fn from_native(native: $type) -> Result<Self> {
                Ok(native)
            }
        }
    )*};
}

plain!(
    bool,
    f32,
    f64,
    i8,
    u8,
    i16,
    u16,
    i32,
    u32,
    i64,
    u64,
    isize,
    usize,
    *mut c_void,
    *const c_void,
);

/// A C type with an absent value, which `None` converts to.
pub(crate) trait Nullable {
    const NULL: Self;
}

impl Nullable for sys::mln_buffer_view {
    const NULL: Self = sys::mln_buffer_view {
        data: std::ptr::null(),
        size: 0,
    };
}

impl<T> Nullable for *const T {
    const NULL: Self = std::ptr::null();
}

impl<N: Nullable, T: ToNative<N>> ToNative<N> for Option<T> {
    fn to_native(&self, arena: &mut InputArena) -> Result<N> {
        match self {
            Some(value) => value.to_native(arena),
            None => Ok(N::NULL),
        }
    }
}

// A present value always has a nonnull pointer, even when it is empty, since a
// nullable view denotes absence with a null pointer alone.
impl ToNative<sys::mln_buffer_view> for [u8] {
    fn to_native(&self, _: &mut InputArena) -> Result<sys::mln_buffer_view> {
        Ok(sys::mln_buffer_view {
            data: self.as_ptr().cast(),
            size: self.len(),
        })
    }
}

impl ToNative<sys::mln_buffer_view> for str {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_buffer_view> {
        self.as_bytes().to_native(arena)
    }
}

impl ToNative<sys::mln_buffer_view> for String {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_buffer_view> {
        self.as_str().to_native(arena)
    }
}

impl ToNative<sys::mln_buffer_view> for Vec<u8> {
    fn to_native(&self, arena: &mut InputArena) -> Result<sys::mln_buffer_view> {
        self.as_slice().to_native(arena)
    }
}

impl ToNative<*const c_char> for str {
    fn to_native(&self, arena: &mut InputArena) -> Result<*const c_char> {
        arena.c_string(self)
    }
}

impl ToNative<*const c_char> for String {
    fn to_native(&self, arena: &mut InputArena) -> Result<*const c_char> {
        arena.c_string(self)
    }
}

impl FromNative<sys::mln_buffer_view> for String {
    unsafe fn from_native(native: sys::mln_buffer_view) -> Result<Self> {
        // SAFETY: the caller guarantees the view is readable.
        unsafe { crate::string::copy_string_view(native) }
    }
}

impl FromNative<sys::mln_buffer_view> for Vec<u8> {
    unsafe fn from_native(native: sys::mln_buffer_view) -> Result<Self> {
        // SAFETY: the caller guarantees the view is readable.
        unsafe { crate::string::copy_string_view_bytes(native) }
    }
}

/// A nullable view: a null pointer is absent, and a nonnull empty view is an
/// empty value.
impl<T: FromNative<sys::mln_buffer_view>> FromNative<sys::mln_buffer_view> for Option<T> {
    unsafe fn from_native(native: sys::mln_buffer_view) -> Result<Self> {
        if native.data.is_null() {
            return Ok(None);
        }
        // SAFETY: the caller guarantees the view is readable.
        unsafe { T::from_native(native) }.map(Some)
    }
}

impl FromNative<*const c_char> for String {
    unsafe fn from_native(native: *const c_char) -> Result<Self> {
        // SAFETY: the caller guarantees a terminated string or null.
        unsafe { crate::string::copy_c_string(native) }
    }
}

impl FromNative<*const c_char> for Option<String> {
    unsafe fn from_native(native: *const c_char) -> Result<Self> {
        if native.is_null() {
            return Ok(None);
        }
        // SAFETY: the caller guarantees a terminated string.
        unsafe { String::from_native(native) }.map(Some)
    }
}

/// Copies `count` units that a pointer and a separate length field describe.
///
/// # Safety
///
/// A nonzero count must address readable units.
pub(crate) unsafe fn counted<P, T: FromNative<sys::mln_buffer_view>>(
    data: *const P,
    count: impl TryInto<usize>,
) -> Result<T> {
    let size = length(count)?;
    let view = sys::mln_buffer_view {
        data: data.cast(),
        size,
    };
    // SAFETY: the caller guarantees the units are readable.
    unsafe { T::from_native(view) }
}

/// A C bit mask that records which optional fields are present.
pub(crate) trait Presence: Copy {
    fn has(self, bit: Self) -> bool;
    fn mark(&mut self, bit: Self);
}

macro_rules! mask {
    ($($type:ty),*) => {$(
        impl Presence for $type {
            fn has(self, bit: Self) -> bool {
                self & bit != 0
            }
            fn mark(&mut self, bit: Self) {
                *self |= bit;
            }
        }
    )*};
}

mask!(u32, u64);

/// Copies an optional field when `mask` has `bit`.
///
/// # Safety
///
/// As for [`FromNative::from_native`].
pub(crate) unsafe fn present<M: Presence, N, T: FromNative<N>>(
    mask: M,
    bit: M,
    native: N,
) -> Result<Option<T>> {
    if !mask.has(bit) {
        return Ok(None);
    }
    // SAFETY: the caller upholds the pointer contract.
    unsafe { T::from_native(native) }.map(Some)
}

/// Sets `bit` in `mask` for a true flag.
pub(crate) fn set_flag<M: Presence>(mask: &mut M, bit: M, value: bool) {
    if value {
        mask.mark(bit);
    }
}

/// A length as a C count field.
pub(crate) fn count<C: TryFrom<usize>>(length: usize) -> Result<C> {
    C::try_from(length).map_err(|_| Error::invalid_argument("input exceeds native count range"))
}

/// Makes a call that writes text into storage the caller sizes, and copies
/// the text into `text`. A call reports the text's length whether or not it
/// fit, and reports the same text for the same inputs, so a call that did not
/// fit is made again with enough storage.
pub(crate) fn caller_sized_text(
    text: &mut Vec<u8>,
    mut call: impl FnMut(*mut c_char, usize, *mut usize) -> sys::mln_status,
) -> sys::mln_status {
    let mut storage = vec![0u8; 256];
    loop {
        let mut length = 0;
        let status = call(storage.as_mut_ptr().cast(), storage.len(), &mut length);
        if status != sys::MLN_STATUS_OK {
            return status;
        }
        if length < storage.len() {
            storage.truncate(length);
            *text = storage;
            return status;
        }
        storage = vec![0; length + 1];
    }
}

/// The text that [`caller_sized_text`] copied, which must be UTF-8.
pub(crate) fn utf8(text: Vec<u8>) -> Result<String> {
    String::from_utf8(text).map_err(|error| {
        Error::invalid_argument(format!("native text was not valid UTF-8: {error}"))
    })
}

fn length(count: impl TryInto<usize>) -> Result<usize> {
    count
        .try_into()
        .map_err(|_| Error::invalid_argument("native count exceeds the address space"))
}

/// Converts a slice into a C array that `arena` owns.
pub(crate) fn array<N: 'static, T: ToNative<N>>(
    values: &[T],
    arena: &mut InputArena,
) -> Result<*const N> {
    let values = values
        .iter()
        .map(|value| value.to_native(arena))
        .collect::<Result<Vec<_>>>()?;
    Ok(arena.array(values))
}

/// Converts one value into a C pointer that `arena` owns.
pub(crate) fn reference<N: 'static, T: ToNative<N> + ?Sized>(
    value: &T,
    arena: &mut InputArena,
) -> Result<*const N> {
    let value = value.to_native(arena)?;
    Ok(arena.store(value))
}

/// Converts an optional value into a C pointer, or a null pointer.
pub(crate) fn optional_reference<N: 'static, T: ToNative<N> + ?Sized>(
    value: Option<&T>,
    arena: &mut InputArena,
) -> Result<*const N> {
    value.map_or(Ok(std::ptr::null()), |value| reference(value, arena))
}

/// Copies `count` values from a C array.
///
/// # Safety
///
/// A nonzero count must address initialized values.
pub(crate) unsafe fn copy_array<N: Copy, T: FromNative<N>>(
    pointer: *const N,
    count: impl TryInto<usize>,
) -> Result<Vec<T>> {
    // SAFETY: the caller guarantees count initialized values.
    let values = unsafe { slice(pointer, length(count)?) }?;
    values
        .iter()
        // SAFETY: as above.
        .map(|value| unsafe { T::from_native(*value) })
        .collect()
}

/// Copies records whose native stride may exceed their Rust size.
///
/// # Safety
/// The pointer must address `count` initialized records at the given stride.
pub(crate) unsafe fn copy_strided<N: Copy, T: FromNative<N>>(
    pointer: *const N,
    count: impl TryInto<usize>,
    stride: impl TryInto<usize>,
) -> Result<Vec<T>> {
    // SAFETY: the caller guarantees the strided records.
    unsafe { strided_values(pointer, length(count)?, length(stride)?) }?
        .into_iter()
        // SAFETY: as above.
        .map(|value| unsafe { T::from_native(value) })
        .collect()
}

/// Copies `count` strided C records without converting them.
///
/// # Safety
///
/// As for [`copy_strided`].
pub(crate) unsafe fn strided_items<N: Copy>(
    pointer: *const N,
    count: impl TryInto<usize>,
    stride: impl TryInto<usize>,
) -> Result<Vec<N>> {
    // SAFETY: the caller guarantees the strided records.
    unsafe { strided_values(pointer, length(count)?, length(stride)?) }
}

/// Copies the one value a C pointer addresses.
///
/// # Safety
///
/// A nonnull pointer must address an initialized value.
pub(crate) unsafe fn copy_reference<N: Copy, T: FromNative<N>>(pointer: *const N) -> Result<T> {
    // SAFETY: the caller guarantees an initialized value.
    let value =
        unsafe { pointer.as_ref() }.ok_or_else(|| Error::invalid_argument("null record value"))?;
    // SAFETY: as above.
    unsafe { T::from_native(*value) }
}

/// A registration's handler in queue mode, which runs its record's callback
/// for the drained `mln_delivery` it receives a pointer to. `take` transfers
/// the delivery's owned handles to the binding before a callback adopts them.
pub type RegistrationHandler =
    std::sync::Arc<dyn Fn(*const c_void, &dyn Fn() -> Result<()>) + Send + Sync>;

/// A local registration's handler, which only its queue's thread runs.
pub type LocalRegistrationHandler = std::sync::Arc<dyn Fn(*const c_void, &dyn Fn() -> Result<()>)>;

/// A registration's handler, of the kind of its record.
pub enum Handler {
    Shared(RegistrationHandler),
    Local(LocalRegistrationHandler),
}

/// The delivery queue that a queue-mode call stores its registrations' handlers
/// in.
pub(crate) trait QueueSink {
    /// Stores a handler before its registration enters C, and returns the
    /// queue and the token that the registration names.
    /// A local handler fails on a shared queue.
    fn hold(&self, handler: Handler) -> Result<(sys::mln_delivery_queue, *mut c_void)>;
    /// Native accepted the registration, so the queue keeps its handler, or
    /// drops it when the queue closed during the call.
    fn accept(&self, user_data: *mut c_void);
    /// Native rejected the registration, so its handler drops unrun.
    fn forget(&self, user_data: *mut c_void);
}

/// Owns temporary C input allocations through native submission, and the
/// registrations that native has not accepted yet.
#[derive(Default)]
pub(crate) struct InputArena<'q> {
    roots: Vec<Box<dyn std::any::Any>>,
    registrations: Vec<PendingRegistration<'q>>,
    /// The queue that this call's registrations name, in queue mode.
    queue: Option<&'q dyn QueueSink>,
}

enum PendingRegistration<'q> {
    Callback {
        pointer: *mut c_void,
        release: unsafe extern "C" fn(*mut c_void),
    },
    Queued(&'q dyn QueueSink, *mut c_void),
}

impl Drop for InputArena<'_> {
    fn drop(&mut self) {
        for registration in self.registrations.drain(..) {
            match registration {
                // SAFETY: rejected submission left each pending root owned by this arena.
                PendingRegistration::Callback { pointer, release } => unsafe { release(pointer) },
                PendingRegistration::Queued(queue, user_data) => queue.forget(user_data),
            }
        }
    }
}

impl<'q> InputArena<'q> {
    /// Puts this call's registrations in queue mode on `queue`.
    pub(crate) fn set_queue(&mut self, queue: &'q dyn QueueSink) {
        self.queue = Some(queue);
    }

    /// Stores a registration's handler in this call's queue, and returns the
    /// queue and token that the registration names; or `None` in callback
    /// mode, which a `local` registration can't use.
    pub(crate) fn queued_registration(
        &mut self,
        local: bool,
        handler: impl FnOnce() -> Handler,
    ) -> Result<Option<(sys::mln_delivery_queue, *mut c_void)>> {
        let Some(queue) = self.queue else {
            if local {
                return Err(Error::invalid_argument(
                    "a local registration needs a local delivery queue",
                ));
            }
            return Ok(None);
        };
        let (raw, user_data) = queue.hold(handler())?;
        self.registrations
            .push(PendingRegistration::Queued(queue, user_data));
        Ok(Some((raw, user_data)))
    }

    /// Stores a pending native registration.
    ///
    /// # Safety
    /// `release` must reclaim exactly one box of `T` without unwinding.
    pub(crate) unsafe fn registration<T: 'static>(
        &mut self,
        state: T,
        release: unsafe extern "C" fn(*mut c_void),
    ) -> *mut c_void {
        let pointer = Box::into_raw(Box::new(state)).cast();
        // SAFETY: the caller's release reclaims the box once.
        unsafe { self.track(pointer, release) };
        pointer
    }

    /// Holds an existing context until native accepts its registration, and
    /// releases it through `release` otherwise.
    ///
    /// # Safety
    /// `release` must free `pointer` exactly once without unwinding.
    pub(crate) unsafe fn track(
        &mut self,
        pointer: *mut c_void,
        release: unsafe extern "C" fn(*mut c_void),
    ) {
        self.registrations
            .push(PendingRegistration::Callback { pointer, release });
    }

    /// Transfers pending callback roots after native accepts their registration.
    pub(crate) fn accept_registrations(&mut self) {
        for registration in self.registrations.drain(..) {
            if let PendingRegistration::Queued(queue, user_data) = registration {
                queue.accept(user_data);
            }
        }
    }

    pub(crate) fn store<T: 'static>(&mut self, value: T) -> *const T {
        let value = Box::new(value);
        let pointer = &*value as *const T;
        self.roots.push(value);
        pointer
    }
    pub(crate) fn array<T: 'static>(&mut self, values: Vec<T>) -> *const T {
        let pointer = values.as_ptr();
        self.roots.push(Box::new(values));
        pointer
    }
    pub(crate) fn c_string(&mut self, value: &str) -> Result<*const c_char> {
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
unsafe fn slice<'a, T>(pointer: *const T, count: usize) -> Result<&'a [T]> {
    unsafe { crate::ptr::slice(pointer, count) }
}

/// Copies a strided native array whose entries may have unaligned addresses.
///
/// # Safety
/// The pointer must address `count` initialized entries at the declared stride.
unsafe fn strided_values<T: Copy>(
    pointer: *const T,
    count: usize,
    stride: usize,
) -> Result<Vec<T>> {
    // SAFETY: the caller guarantees each strided entry.
    Ok(unsafe { crate::ptr::strided_records(pointer, count, stride) }?.collect())
}

/// Declares an open C enum: one variant per known value, and `$unknown` for a
/// value this binding predates, which converts back unchanged.
macro_rules! native_enum {
    (
        $(#[$meta:meta])*
        pub enum $name:ident: $raw:ty {
            $($(#[$variant_meta:meta])* $variant:ident = $value:expr),* $(,)?
        } $unknown:ident
    ) => {
        $(#[$meta])*
        #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
        pub enum $name {
            $($(#[$variant_meta])* $variant,)*
            $unknown($raw),
        }
        impl $name {
            pub fn from_raw(raw: $raw) -> Self {
                #[allow(unreachable_patterns)]
                match raw {
                    $($value => Self::$variant,)*
                    value => Self::$unknown(value),
                }
            }
            pub fn as_raw(self) -> $raw {
                match self {
                    $(Self::$variant => $value,)*
                    Self::$unknown(value) => value,
                }
            }
        }
        impl $crate::convert::ToNative<$raw> for $name {
            fn to_native(&self, _: &mut $crate::convert::InputArena) -> $crate::Result<$raw> {
                Ok(self.as_raw())
            }
        }
        impl $crate::convert::FromNative<$raw> for $name {
            unsafe fn from_native(native: $raw) -> $crate::Result<Self> {
                Ok(Self::from_raw(native))
            }
        }
    };
}

/// Declares a C bit mask that keeps bits this binding predates.
macro_rules! native_flags {
    (
        $(#[$meta:meta])*
        pub struct $name:ident: $raw:ty {
            $($(#[$($flag_meta:tt)*])* const $flag:ident = $value:expr;)*
        }
    ) => {
        bitflags::bitflags! {
            $(#[$meta])*
            #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
            pub struct $name: $raw {
                // bitflags matches each flag attribute's tokens, so they pass
                // through unparsed.
                $($(#[$($flag_meta)*])* const $flag = $value;)*
                const _ = !0;
            }
        }
        impl $crate::convert::ToNative<$raw> for $name {
            fn to_native(&self, _: &mut $crate::convert::InputArena) -> $crate::Result<$raw> {
                Ok(self.bits())
            }
        }
        impl $crate::convert::FromNative<$raw> for $name {
            unsafe fn from_native(native: $raw) -> $crate::Result<Self> {
                Ok(Self::from_bits_retain(native))
            }
        }
    };
}

pub(crate) use {native_enum, native_flags};

#[cfg(test)]
mod tests {
    use std::sync::Arc;
    use std::sync::atomic::{AtomicUsize, Ordering};

    use super::*;
    use crate::{CanonicalTileId, CustomGeometrySourceOptions};

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
        assert_eq!(unsafe { slice(values, 3) }.unwrap(), &[3, 7, 11]);
    }

    #[test]
    fn generated_registration_transfers_only_after_acceptance_and_contains_panics() {
        let drops = Arc::new(AtomicUsize::new(0));
        let calls = Arc::new(AtomicUsize::new(0));
        let tile = CanonicalTileId::new(3, 4, 5);
        for accepted in [false, true] {
            let capture = DropCount(Arc::clone(&drops));
            let seen = Arc::clone(&calls);
            let options = CustomGeometrySourceOptions::new(move |received| {
                let _ = &capture;
                assert_eq!(received, tile);
                seen.fetch_add(1, Ordering::SeqCst);
                panic!("contained callback panic");
            });
            let mut arena = InputArena::default();
            let raw: sys::mln_custom_geometry_source_options =
                to_native(&options, &mut arena).unwrap();
            drop(options);
            if accepted {
                arena.accept_registrations();
            }
            drop(arena);
            if accepted {
                assert_eq!(drops.load(Ordering::SeqCst), 1);
                let native = to_native(&tile, &mut InputArena::default()).unwrap();
                // SAFETY: accepted registration retains its generated root until release.
                unsafe {
                    raw.fetch_tile.unwrap()(raw.user_data, native);
                    raw.release_user_data.unwrap()(raw.user_data);
                }
            }
        }
        assert_eq!(calls.load(Ordering::SeqCst), 1);
        assert_eq!(drops.load(Ordering::SeqCst), 2);
    }
    #[test]
    fn a_strided_batch_and_an_unknown_union_arm_decode_without_losing_data() {
        // A batch from a newer library: each event record is longer than this
        // binding's, and the second event carries a payload this binding predates.
        let stride = std::mem::size_of::<sys::mln_runtime_event>() + 24;
        let messages = b"first\0second\0";
        let events = [
            (
                sys::MLN_RUNTIME_EVENT_MAP_IDLE,
                sys::MLN_RUNTIME_EVENT_PAYLOAD_NONE,
                0,
                5,
            ),
            (sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED, 0xfff0, 6, 6),
        ];
        let mut storage = vec![0_u8; stride * events.len()];
        for (index, (event_type, payload_type, offset, length)) in events.into_iter().enumerate() {
            // SAFETY: the record is plain data, and all zeroes is a valid value.
            let mut event: sys::mln_runtime_event = unsafe { std::mem::zeroed() };
            event.type_ = event_type;
            event.source_type = sys::MLN_RUNTIME_EVENT_SOURCE_MAP;
            event.source = 0xabcd_0000_0000_0000 | index as u64;
            event.generation = 0x1_0000_0000 + index as u64;
            event.payload_type = payload_type;
            event.message = sys::mln_buffer_view {
                data: messages.as_ptr().wrapping_add(offset).cast(),
                size: length,
            };
            // SAFETY: the storage holds a full stride for every event, and the
            // write does not rely on its alignment.
            unsafe {
                storage
                    .as_mut_ptr()
                    .add(index * stride)
                    .cast::<sys::mln_runtime_event>()
                    .write_unaligned(event);
            }
        }
        // SAFETY: every pointer in the events addresses the storage above.
        let events: Vec<crate::RuntimeEvent> = unsafe {
            copy_strided(
                storage.as_ptr().cast::<sys::mln_runtime_event>(),
                events.len(),
                stride,
            )
        }
        .unwrap();
        assert_eq!(events.len(), 2);
        assert_eq!(events[0].r#type, crate::RuntimeEventType::MapIdle);
        assert_eq!(events[0].payload, crate::RuntimeEventPayload::Empty);
        assert_eq!(events[0].message, "first");
        assert_eq!(events[1].r#type, crate::RuntimeEventType::MapStyleLoaded);
        assert_eq!(events[1].source, 0xabcd_0000_0000_0001);
        assert_eq!(events[1].generation, 0x1_0000_0001);
        assert_eq!(
            events[1].payload,
            crate::RuntimeEventPayload::Unknown(0xfff0)
        );
        assert_eq!(events[1].message, "second");
    }
}
