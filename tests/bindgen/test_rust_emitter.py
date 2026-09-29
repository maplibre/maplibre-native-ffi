"""Compile header-driven Rust value types and methods against an isolated ABI stub."""

import subprocess
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from tools.bindgen.emitters import rust
from tools.bindgen.frontend import parse_headers
from tools.bindgen.schema import validate


class RustEmitterTests(unittest.TestCase):
    def test_new_record_keywords_and_runtime_local_collisions_compile(self):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            headers = root / "include"
            headers.mkdir()
            (headers / "api.h").write_text("""
#define BIND(x) __attribute__((annotate("mln:" x)))
typedef unsigned long long mln_map BIND("kind=handle;release=mln_map_release;parent=none");
BIND("execution=immediate") void mln_map_release(mln_map map BIND("consumes=always"));
typedef int mln_status;
typedef struct mln_diagnostic { unsigned int size; char message[4096]; } mln_diagnostic;
typedef struct mln_completion { void *state; } mln_completion;
typedef struct mln_new_point { double type; double self; double str; } mln_new_point;
BIND("execution=query;result=mln_new_point;shape=value;ownership=borrowed")
mln_status mln_map_match(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_move(mln_map map, mln_new_point native, mln_new_point arena, mln_new_point binding_arg_1, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
            api = parse_headers(headers)
            validate(api)
            self.assertEqual(
                set(rust.coverage(api)["generated"]),
                {"mln_map_match", "mln_map_move", "mln_map_release"},
            )
            files = rust.generate(api)
            (root / "values.rs").write_text(
                files["crates/maplibre-native-ffi-core/src/generated.rs"]
            )
            owners = root / "generated"
            owners.mkdir()
            prefix = "crates/maplibre-native-ffi/src/generated/"
            self.assertEqual(
                {
                    path.removeprefix(prefix)
                    for path in files
                    if path.startswith(prefix)
                },
                {"map.rs", "mod.rs"},
            )
            for path, source in files.items():
                if path.startswith(prefix):
                    (owners / path.removeprefix(prefix)).write_text(source)
            (root / "lib.rs").write_text("""
#![allow(dead_code, non_camel_case_types, unused_imports)]
extern crate self as maplibre_native_ffi_sys;
extern crate self as maplibre_native_ffi_core;
#[derive(Clone, Copy)]
pub struct mln_new_point { pub type_: f64, pub self_: f64, pub str_: f64 }
#[derive(Clone, Copy, Debug)]
pub struct mln_map(pub u64);
pub unsafe fn mln_map_release(_: mln_map) {}
pub unsafe fn mln_map_match(_: mln_map, _: *const (), _: *mut ()) -> i32 { 0 }
pub unsafe fn mln_map_move(_: mln_map, _: mln_new_point, _: mln_new_point, _: mln_new_point, _: *const (), _: *mut ()) -> i32 { 0 }
pub mod values {
    pub trait NativeValue: Sized {
        type Raw;
        fn to_native(self) -> Self::Raw;
        fn from_native(value: Self::Raw) -> Self;
    }
}
pub mod generated { include!("values.rs"); }
pub use generated::*;
type Result<T> = std::result::Result<T, ()>;
struct NativeFuture<T>(std::marker::PhantomData<T>);
struct CommandCompletion;
mod completion {
    pub fn copy_value<T>(_: ()) -> super::Result<T> { unimplemented!() }
    pub fn ready<T>(_: T) -> super::NativeFuture<T> { unimplemented!() }
    pub fn submit<T>(_: impl Fn(*const (), *mut ()) -> i32, _: impl Fn(()) -> super::Result<T>) -> super::Result<super::NativeFuture<T>> { unimplemented!() }
    pub fn submit_command(_: impl Fn(*const (), *mut ()) -> i32) -> super::Result<super::NativeFuture<super::CommandCompletion>> { unimplemented!() }
}
mod callback { pub fn check(_: &str, _: u64) -> super::Result<()> { Ok(()) } }
mod handle {
    #[derive(Debug)]
    pub struct ConcurrentNativeHandle<T: Copy>(std::cell::Cell<Option<T>>);
    impl<T: Copy> ConcurrentNativeHandle<T> {
        pub unsafe fn from_handle(raw: T, _: &str) -> super::Result<Self> { Ok(Self(std::cell::Cell::new(Some(raw)))) }
        pub fn live_handle(&self) -> Option<T> { self.0.get() }
        pub fn is_closed(&self) -> bool { self.0.get().is_none() }
        pub fn close_with<R>(&self, close: impl FnOnce(T) -> super::Result<R>) -> super::Result<Option<R>> {
            let Some(raw) = self.0.get() else { return Ok(None) };
            let result = close(raw)?;
            self.0.set(None);
            Ok(Some(result))
        }
        pub fn finalize_with(&mut self, dispose: impl FnOnce(T) -> super::Result<()>) {
            if let Some(raw) = self.0.take() { let _ = dispose(raw); }
        }
    }
    pub fn closed_handle_error(_: &str) {}
}
#[path = "generated/mod.rs"]
mod owners;
fn main() {
    let point = NewPoint::new(1.0, 2.0, 3.0);
    let raw = values::NativeValue::to_native(point);
    assert_eq!((raw.type_, raw.self_, raw.str_), (1.0, 2.0, 3.0));
    assert_eq!(NewPoint::from_native(raw), point);
    let map = owners::MapHandle::from_native(mln_map(7)).unwrap();
    assert_eq!(map.id(), 7);
    map.release().unwrap();
    assert!(map.is_closed());
}
""")
            binary = root / "probe"
            subprocess.run(
                ["rustc", "--edition=2024", str(root / "lib.rs"), "-o", str(binary)],
                check=True,
                capture_output=True,
                text=True,
            )
            subprocess.run([str(binary)], check=True)
