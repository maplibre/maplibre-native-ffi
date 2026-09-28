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
typedef struct mln_completion { void *state; } mln_completion;
typedef struct mln_new_point { double type; double self; double str; } mln_new_point;
BIND("execution=query;result=mln_new_point;shape=value;ownership=borrowed")
mln_status mln_map_match(mln_map map, const mln_completion *completion);
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_move(mln_map map, mln_new_point native, mln_new_point arena, mln_new_point binding_arg_1, const mln_completion *completion);
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
            (root / "operations.rs").write_text(
                files["crates/maplibre-native-ffi/src/map/generated.rs"]
            )
            (root / "lib.rs").write_text("""
#![allow(dead_code, non_camel_case_types)]
extern crate self as maplibre_native_ffi_sys;
extern crate self as maplibre_core;
#[derive(Clone, Copy)]
pub struct mln_new_point { pub type_: f64, pub self_: f64, pub str_: f64 }
pub mod values {
    pub trait NativeValue: Sized {
        type Raw;
        fn to_native(self) -> Self::Raw;
        fn from_native(value: Self::Raw) -> Self;
    }
}
mod generated { include!("values.rs"); }
pub use generated::*;
type Result<T> = std::result::Result<T, ()>;
struct NativeFuture<T>(std::marker::PhantomData<T>);
struct CommandCompletion;
mod completion {
    pub fn copy_value<T>(_: ()) -> super::Result<T> { unimplemented!() }
    pub fn submit<T>(_: impl Fn(*const ()) -> i32, _: impl Fn(()) -> super::Result<T>) -> super::Result<super::NativeFuture<T>> { unimplemented!() }
    pub fn submit_command(_: impl Fn(*const ()) -> i32) -> super::Result<super::NativeFuture<super::CommandCompletion>> { unimplemented!() }
}
mod callback { pub fn check(_: &str, _: u64) -> super::Result<()> { Ok(()) } }
mod sys {
    pub use crate::mln_new_point;
    #[derive(Clone, Copy)] pub struct mln_map(pub u64);
    pub unsafe fn mln_map_release(_: mln_map) {}
    pub unsafe fn mln_map_match(_: mln_map, _: *const ()) -> i32 { 0 }
    pub unsafe fn mln_map_move(_: mln_map, _: mln_new_point, _: mln_new_point, _: mln_new_point, _: *const ()) -> i32 { 0 }
}
mod map {
    use super::*;
    struct TestHandle;
    impl TestHandle { fn close_with<R>(&self, f: impl FnOnce(sys::mln_map) -> Result<R>) -> Result<Option<R>> { f(sys::mln_map(1)).map(Some) } }
    struct Inner { handle: TestHandle }
    impl Inner { fn native(&self) -> Result<sys::mln_map> { Ok(sys::mln_map(1)) } }
    struct MapHandle { inner: Inner }
    mod operations { include!("operations.rs"); }
}
fn main() {
    let point = NewPoint::new(1.0, 2.0, 3.0);
    let raw = values::NativeValue::to_native(point);
    assert_eq!((raw.type_, raw.self_, raw.str_), (1.0, 2.0, 3.0));
    assert_eq!(NewPoint::from_native(raw), point);
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
