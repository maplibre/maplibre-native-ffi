use std::ptr::{self, NonNull};

use crate::error::{Error, Result};

#[derive(Debug)]
pub struct OutPtr<T> {
    value: *mut T,
}

impl<T> OutPtr<T> {
    pub fn new() -> Self {
        Self {
            value: ptr::null_mut(),
        }
    }

    pub fn as_mut_ptr(&mut self) -> *mut *mut T {
        &mut self.value
    }

    pub fn get(&self) -> *mut T {
        self.value
    }

    pub fn into_non_null(self, name: &'static str) -> Result<NonNull<T>> {
        non_null_mut(self.value, name)
    }

    pub fn into_option(self) -> Option<NonNull<T>> {
        NonNull::new(self.value)
    }
}

impl<T> Default for OutPtr<T> {
    fn default() -> Self {
        Self::new()
    }
}

pub fn non_null_mut<T>(ptr: *mut T, name: &'static str) -> Result<NonNull<T>> {
    NonNull::new(ptr).ok_or_else(|| null_pointer_error(name))
}

pub fn null_pointer_error(name: &'static str) -> Error {
    Error::invalid_argument(format!("{name} must not be null"))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn out_pointer_starts_null_and_wraps_non_null_values() {
        let mut out = OutPtr::<u8>::new();
        assert!(out.get().is_null());

        let mut value = 7u8;
        // Simulate a native function writing a handle through a T** out-pointer.
        unsafe {
            *out.as_mut_ptr() = &mut value;
        }

        assert_eq!(
            out.into_non_null("value").unwrap().as_ptr().cast_const(),
            ptr::addr_of!(value)
        );
    }

    #[test]
    fn out_pointer_into_option_wraps_non_null_values() {
        let mut out = OutPtr::<u8>::new();
        let mut value = 7u8;

        // Simulate a native function writing an optional handle through a T**
        // out-pointer.
        unsafe {
            *out.as_mut_ptr() = &mut value;
        }

        assert_eq!(
            out.into_option().unwrap().as_ptr().cast_const(),
            ptr::addr_of!(value)
        );
    }

    #[test]
    fn non_null_wrappers_reject_null() {
        let error = non_null_mut::<u8>(ptr::null_mut(), "value").unwrap_err();

        assert_eq!(error.kind(), crate::error::ErrorKind::InvalidArgument);
        assert!(error.diagnostic().contains("value must not be null"));
    }
}
