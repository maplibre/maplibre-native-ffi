use std::any::Any;

/// Owns temporary C input allocations until a native call returns.
#[derive(Default)]
pub struct InputStorage {
    roots: Vec<Box<dyn Any>>,
}
impl InputStorage {
    pub fn array<T: 'static>(&mut self, values: Vec<T>) -> *const T {
        let pointer = values.as_ptr();
        self.roots.push(Box::new(values));
        pointer
    }
}
