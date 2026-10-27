use std::any::Any;
use std::ffi::c_char;

/// Owns temporary C input allocations until a native call returns.
#[derive(Default)]
pub struct InputStorage {
    roots: Vec<Box<dyn Any>>,
}
impl InputStorage {
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
    pub fn c_string(&mut self, value: &str) -> super::Result<*const c_char> {
        let value = super::string::c_string(value)?;
        let pointer = value.as_ptr();
        self.roots.push(Box::new(value));
        Ok(pointer)
    }
}
