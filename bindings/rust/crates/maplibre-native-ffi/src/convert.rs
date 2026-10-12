//! Conversions between this crate's values and their C representations.
//!
//! Generated records and operations convert every field and argument through
//! [`ToNative`] and [`FromNative`], and through the helpers here for the shapes
//! that a Rust type alone does not select: presence fields, counted and
//! strided arrays, and borrowed references.

use std::any::Any;
use std::ffi::{c_char, c_void};

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use crate::{Error, Result};

/// Converts a value into the C representation `N` for one native call.
///
/// Pointers in the result borrow `self` or `arena`, so the result stays valid
/// while both live.
pub trait ToNative<N> {
    fn to_native(&self, arena: &mut InputArena) -> Result<N>;
}

/// Copies a C value of type `N`, and everything it points to, into an owned
/// value.
pub trait FromNative<N>: Sized {
    /// # Safety
    ///
    /// Every pointer in `native` must be readable for the lengths its C
    /// declaration states, for the duration of this call.
    unsafe fn from_native(native: N) -> Result<Self>;
}

/// Converts `value` for one call. A function rather than a method, since enum
/// types also have an inherent `to_native`.
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

/// Copies a value that a C default constructor returned.
pub(crate) fn native_default<N, T: FromNative<N>>(native: N) -> T {
    // SAFETY: C default constructors return values that hold no pointers.
    unsafe { from_native(native) }.expect("native default must be valid")
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
pub trait Nullable {
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
        unsafe { maplibre_core::string::copy_string_view(native) }
    }
}

impl FromNative<sys::mln_buffer_view> for Vec<u8> {
    unsafe fn from_native(native: sys::mln_buffer_view) -> Result<Self> {
        // SAFETY: the caller guarantees the view is readable.
        unsafe { maplibre_core::string::copy_string_view_bytes(native) }
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
        unsafe { maplibre_core::string::copy_c_string(native) }
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

/// Copies a view whose empty value means absent.
///
/// # Safety
///
/// The view must be readable.
pub(crate) unsafe fn nonempty<T: FromNative<sys::mln_buffer_view>>(
    native: sys::mln_buffer_view,
) -> Result<Option<T>> {
    if native.size == 0 {
        return Ok(None);
    }
    // SAFETY: the caller guarantees the view is readable.
    unsafe { T::from_native(native) }.map(Some)
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

/// Converts a slice into a C array of fixed length.
#[allow(
    dead_code,
    reason = "a generator shape that the current headers do not use"
)]
pub(crate) fn fixed<N, T: ToNative<N>, const LENGTH: usize>(
    values: &[T],
    arena: &mut InputArena,
) -> Result<[N; LENGTH]> {
    values
        .iter()
        .map(|value| value.to_native(arena))
        .collect::<Result<Vec<_>>>()?
        .try_into()
        .map_err(|_| Error::invalid_argument("incorrect fixed array length"))
}

/// Converts one value into a C pointer that `arena` owns.
pub(crate) fn reference<N: 'static, T: ToNative<N> + ?Sized>(
    value: &T,
    arena: &mut InputArena,
) -> Result<*const N> {
    let value = value.to_native(arena)?;
    Ok(arena.store(value))
}

/// Converts an optional slice into a C array, or a null pointer.
#[allow(
    dead_code,
    reason = "a generator shape that the current headers do not use"
)]
pub(crate) fn optional_array<N: 'static, T: ToNative<N>>(
    values: Option<&[T]>,
    arena: &mut InputArena,
) -> Result<*const N> {
    values.map_or(Ok(std::ptr::null()), |values| array(values, arena))
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

/// Copies a C array, or `None` for a null pointer.
///
/// # Safety
///
/// As for [`copy_array`].
#[allow(
    dead_code,
    reason = "a generator shape that the current headers do not use"
)]
pub(crate) unsafe fn copy_optional_array<N: Copy, T: FromNative<N>>(
    pointer: *const N,
    count: impl TryInto<usize>,
) -> Result<Option<Vec<T>>> {
    if pointer.is_null() {
        return Ok(None);
    }
    // SAFETY: the caller upholds the array contract.
    unsafe { copy_array(pointer, count) }.map(Some)
}

/// Copies a C array of fixed length.
///
/// # Safety
///
/// As for [`FromNative::from_native`].
#[allow(
    dead_code,
    reason = "a generator shape that the current headers do not use"
)]
pub(crate) unsafe fn copy_fixed<N: Copy, T: FromNative<N>, const LENGTH: usize>(
    values: [N; LENGTH],
) -> Result<Vec<T>> {
    values
        .into_iter()
        // SAFETY: the caller upholds the pointer contract.
        .map(|value| unsafe { T::from_native(value) })
        .collect()
}

/// Copies `count` records whose stride may exceed this binding's record size.
///
/// # Safety
///
/// The pointer must address `count` records at the given stride.
#[allow(
    dead_code,
    reason = "a generator shape that the current headers do not use"
)]
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

/// Copies `count` C records without converting them.
///
/// # Safety
///
/// As for [`copy_array`].
#[allow(
    dead_code,
    reason = "a generator shape that the current headers do not use"
)]
pub(crate) unsafe fn items<N: Copy>(
    pointer: *const N,
    count: impl TryInto<usize>,
) -> Result<Vec<N>> {
    // SAFETY: the caller guarantees count initialized values.
    Ok(unsafe { slice(pointer, length(count)?) }?.to_vec())
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

/// Owns temporary C input allocations through native submission.
#[derive(Default)]
pub struct InputArena {
    roots: Vec<Box<dyn Any>>,
    registrations: Vec<PendingRegistration>,
}

struct PendingRegistration {
    pointer: *mut c_void,
    release: unsafe extern "C" fn(*mut c_void),
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
    pub(crate) unsafe fn registration<T: 'static>(
        &mut self,
        state: T,
        release: unsafe extern "C" fn(*mut c_void),
    ) -> *mut c_void {
        let pointer = Box::into_raw(Box::new(state)).cast();
        self.registrations
            .push(PendingRegistration { pointer, release });
        pointer
    }

    /// Transfers pending callback roots after native accepts their registration.
    pub(crate) fn accept_registrations(&mut self) {
        self.registrations.clear();
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
        let value = maplibre_core::string::c_string(value)?;
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
    if count == 0 {
        return Ok(&[]);
    }
    if pointer.is_null() || count > isize::MAX as usize / std::mem::size_of::<T>().max(1) {
        return Err(Error::invalid_argument("invalid native array"));
    }
    // SAFETY: the caller guarantees count initialized values.
    Ok(unsafe { std::slice::from_raw_parts(pointer, count) })
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
    Ok(unsafe { maplibre_core::ptr::strided_records(pointer, count, stride) }?.collect())
}

/// Copies a UTF-8 range from a native batch's shared message storage.
///
/// # Safety
/// The data pointer must address `size` readable bytes.
pub(crate) unsafe fn arena_string<P>(
    data: *const P,
    size: impl TryInto<usize>,
    offset: impl TryInto<usize>,
    count: impl TryInto<usize>,
) -> Result<String> {
    let (offset, count) = (length(offset)?, length(count)?);
    let end = offset
        .checked_add(count)
        .ok_or_else(|| Error::invalid_argument("native message range overflow"))?;
    // SAFETY: the caller guarantees size readable bytes.
    let bytes = unsafe { slice(data.cast::<u8>(), length(size)?) }?;
    let value = bytes
        .get(offset..end)
        .ok_or_else(|| Error::invalid_argument("native message range is outside its batch"))?;
    String::from_utf8(value.to_vec())
        .map_err(|_| Error::invalid_argument("native message is invalid UTF-8"))
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
        impl Default for $name {
            fn default() -> Self {
                Self::from_raw(0)
            }
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
            pub fn from_native(raw: $raw) -> Self {
                Self::from_raw(raw)
            }
            pub fn to_native(self) -> $raw {
                self.as_raw()
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
        impl $name {
            pub fn from_native(raw: $raw) -> Self {
                Self::from_bits_retain(raw)
            }
            pub fn to_native(self) -> $raw {
                self.bits()
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
}
