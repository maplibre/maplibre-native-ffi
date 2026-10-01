"""Compile header-driven Rust value types and methods against an isolated ABI stub."""

import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import ROOT, parse, real_api, require_tool, run

from tools.bindgen.emitters import rust, rust_sys
from tools.bindgen.emitters.rust import native_identifier
from tools.bindgen.schema import validate

SYS = ROOT / "bindings/rust/crates/maplibre-native-ffi-sys/src"


class RustEmitterTests(unittest.TestCase):
    def test_new_record_keywords_and_runtime_local_collisions_compile(self):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            api = parse(
                """
typedef struct mln_new_point { double type; double self; double str; } mln_new_point;
BIND("execution=query;result=mln_new_point;shape=value;ownership=borrowed")
mln_status mln_map_match(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_move(mln_map map, mln_new_point native, mln_new_point arena, mln_new_point binding_arg_1, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""",
                defines=("MLN_PROTOCOL_MAP_RELEASE",),
            )
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
            require_tool(self, "rustc").run(
                self,
                "--edition=2024",
                str(root / "lib.rs"),
                "-o",
                str(binary),
                cwd=root,
            )
            run(self, [str(binary)], root)


class RustSysTests(unittest.TestCase):
    def test_declarations_match_the_c_layout(self):
        """Every emitted record, handle, and enum has the size, alignment, and
        field offsets that the C compiler gives the real headers."""
        api = real_api()
        layout = {
            "mln_diagnostic": ("size", "message"),
            **rust_sys.declarations(api).layout(),
        }
        records = {record.name: record.kind for record in api.records}
        with TemporaryDirectory() as directory:
            root = Path(directory)
            crate = root / "sys"
            crate.mkdir()
            (crate / "lib.rs").write_text((SYS / "lib.rs").read_text())
            (crate / "generated.rs").write_text(rust.generate(api)[rust_sys.PATH])
            rust_lines, c_lines = [], []
            for name, fields in sorted(layout.items()):
                c_type = (
                    f"{records.get(name, 'struct')} {name}"
                    if name in records or name == "mln_diagnostic"
                    else name
                )
                rust_lines.append(
                    f'writeln!(out, "{name} {{}} {{}}", size_of::<sys::{name}>(), align_of::<sys::{name}>()).unwrap();'
                )
                c_lines.append(
                    f'fprintf(out, "{name} %zu %zu\\n", sizeof({c_type}), _Alignof({c_type}));'
                )
                for field in fields:
                    rust_lines.append(
                        f'writeln!(out, "{name}.{field} {{}}", std::mem::offset_of!(sys::{name}, {native_identifier(field)})).unwrap();'
                    )
                    c_lines.append(
                        f'fprintf(out, "{name}.{field} %zu\\n", offsetof({c_type}, {field}));'
                    )
            (root / "main.rs").write_text(
                "use std::io::Write;\nuse std::mem::{align_of, size_of};\n"
                "use maplibre_native_ffi_sys as sys;\nfn main() {\n"
                "    let mut out = std::fs::File::create(std::env::args().nth(1).unwrap()).unwrap();\n    "
                + "\n    ".join(rust_lines)
                + "\n}\n"
            )
            (root / "main.c").write_text(
                "#include <stddef.h>\n#include <stdio.h>\n"
                "#include <maplibre_native_c.h>\n"
                "#include <maplibre_native_c/callback_adapter.h>\n"
                "#include <maplibre_native_c/plugin.h>\n"
                "int main(int argc, char** argv) {\n  (void)argc;\n"
                '  FILE* out = fopen(argv[1], "w");\n  '
                + "\n  ".join(c_lines)
                + "\n  return fclose(out);\n}\n"
            )
            rustc = require_tool(self, "rustc")
            rustc.run(
                self,
                "--edition=2024",
                "--crate-type=lib",
                "--crate-name=maplibre_native_ffi_sys",
                str(crate / "lib.rs"),
                "--out-dir",
                str(root),
                cwd=root,
            )
            rustc.run(
                self,
                "--edition=2024",
                "--extern",
                f"maplibre_native_ffi_sys={root / 'libmaplibre_native_ffi_sys.rlib'}",
                str(root / "main.rs"),
                "-o",
                str(root / "rust-layout"),
                cwd=root,
            )
            require_tool(self, "clang").run(
                self,
                "-std=c2x",
                f"-I{ROOT / 'include'}",
                f"-I{ROOT / 'third_party/maplibre-native/include'}",
                str(root / "main.c"),
                "-o",
                str(root / "c-layout"),
                cwd=root,
            )
            run(self, [str(root / "rust-layout"), str(root / "rust.txt")], root)
            run(self, [str(root / "c-layout"), str(root / "c.txt")], root)
            rust_layout = (root / "rust.txt").read_text().splitlines()
            c_layout = (root / "c.txt").read_text().splitlines()
            self.assertGreater(len(c_layout), len(layout))
            self.assertEqual(rust_layout, c_layout)
