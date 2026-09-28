"""Compile new C value shapes and execute their generated Go round trips."""

import subprocess
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from tools.bindgen.emitters import go
from tools.bindgen.frontend import parse_headers
from tools.bindgen.schema import validate


class GoEmitterTests(unittest.TestCase):
    def test_header_mutation_compiles_counted_arrays_and_optional_keyword_fields(self):
        for scalar, literal in (("double", "3.25"), ("_Bool", "true")):
            with self.subTest(scalar=scalar), TemporaryDirectory() as directory:
                root = Path(directory)
                include = root / "include"
                include.mkdir()
                header = """
#ifndef PROBE_H
#define PROBE_H
#include <stdint.h>
#include <stdlib.h>
#define BIND(x) __attribute__((annotate("mln:" x)))
typedef int mln_status;
typedef struct mln_buffer_view { const void *data; size_t size; } mln_buffer_view;
typedef struct mln_probe_point { double type; SCALAR gain; } mln_probe_point;
typedef struct mln_probe_options {
  mln_buffer_view title BIND("encoding=utf8;nullable=true");
  _Bool has_point BIND("kind=presence_mask");
  mln_probe_point point BIND("mask=has_point");
  const mln_probe_point *left BIND("length=left_count;ownership=borrowed;nullable=true");
  uint16_t left_count BIND("kind=count");
  const mln_probe_point *right BIND("length=right_count;ownership=borrowed");
  uint32_t right_count BIND("kind=count");
} mln_probe_options;
BIND("execution=immediate")
static inline mln_status mln_probe_roundtrip(mln_probe_options input, mln_probe_options *out BIND("direction=out")) { *out = input; return 0; }
typedef struct mln_probe_text_result { mln_buffer_view text BIND("encoding=utf8;nullable=true"); } mln_probe_text_result;
BIND("execution=immediate")
static inline mln_status mln_probe_nullable_text(const char *text BIND("length=text_size;encoding=utf8;nullable=true;ownership=borrowed"), uint16_t text_size BIND("kind=count"), mln_probe_text_result *out BIND("direction=out")) { out->text = (mln_buffer_view){text, text_size}; return 0; }
#endif
""".replace("SCALAR", scalar)
                (include / "api.h").write_text(header)
                api = parse_headers(include)
                validate(api)
                self.assertEqual(
                    go.coverage(api),
                    {
                        "generated": ["mln_probe_nullable_text", "mln_probe_roundtrip"],
                        "unsupported": {},
                    },
                )
                files = go.generate(api)
                source = files["generated_api.go"]
                source = source.replace(
                    "#cgo pkg-config: maplibre-native-c",
                    "#cgo CFLAGS: -I${SRCDIR}/include",
                )
                source = source.replace(
                    '#include "maplibre_native_c/plugin.h"', '#include "api.h"'
                )
                source = source.replace(
                    '#include "maplibre_native_c/callback_adapter.h"', ""
                )
                (root / "generated.go").write_text(source)
                (root / "generated_callbacks.h").write_text(
                    "enum { binding_operation_mln_probe_roundtrip = 1, binding_operation_mln_probe_nullable_text = 2 };\n"
                )
                (root / "go.mod").write_text("module fixture\n\ngo 1.24\n")
                (root / "runtime.go").write_text("""package maplibre
/*
#include <stdlib.h>
*/
import "C"
import "unsafe"
import "fmt"
type bindingArena struct { pointers []unsafe.Pointer }
func (a *bindingArena) allocate(size uintptr) unsafe.Pointer { if size==0 { return nil }; p := C.calloc(1, C.size_t(size)); a.pointers = append(a.pointers,p); return p }
func (a *bindingArena) bytes(value []byte) unsafe.Pointer { if len(value)==0 { return nil }; p:=a.allocate(uintptr(len(value))); copy(unsafe.Slice((*byte)(p),len(value)),value); return p }
func bindingString(p unsafe.Pointer,n uint64) string { return string(unsafe.Slice((*byte)(p),int(n))) }
func (a *bindingArena) array(count int,size uintptr) unsafe.Pointer { return a.allocate(uintptr(count)*size) }
func (a *bindingArena) close() { for _, p := range a.pointers { C.free(p) } }
func (a *bindingArena) fail(message string) { panic(message) }
func bindingCall[T any](f func() T) (T,error) { return f(),nil }
func bindingCheck(f func() int32) { if f()!=0 { panic("native failure") } }
func bindingAdmission(_ uint32,_ uint64) func() { return func(){} }
func bindingCount[T ~uint16 | ~uint32](n int) T { if uint64(T(n)) != uint64(n) { panic("overflow") }; return T(n) }
func bindingCountLike[T ~uint16 | ~uint32](_ T,n int) T { if uint64(T(n)) != uint64(n) { panic("overflow") }; return T(n) }
func bindingLength(n uint64) int { return int(n) }
func bindingElement(p unsafe.Pointer,i int,stride uint64,size,align uintptr) unsafe.Pointer { if stride < uint64(size) || stride%uint64(align)!=0 { panic(fmt.Sprint("invalid stride",stride)) }; return unsafe.Add(p,uintptr(i)*uintptr(stride)) }
""")
                (root / "values_test.go").write_text(
                    """package maplibre
import "testing"
func TestRoundtrip(t *testing.T) {
    point := ProbePoint{Type: 9.5, Gain: LITERAL}
    empty := ""
    input := ProbeOptions{Title: &empty, Point: &point, Left: []ProbePoint{point}, Right: []ProbePoint{point,point}}
    output, err := ProbeRoundtrip(input)
    if err != nil || output.Title == nil || *output.Title != "" || output.Point == nil || *output.Point != point || len(output.Left)!=1 || len(output.Right)!=2 || output.Right[1]!=point { t.Fatalf("round trip: %+v, %v",output,err) }
    emptyArray, err := ProbeRoundtrip(ProbeOptions{Left: []ProbePoint{}})
    if err != nil || emptyArray.Left == nil || len(emptyArray.Left)!=0 { t.Fatalf("present empty array: %+v, %v",emptyArray,err) }
    for _, text := range []*string{nil, &empty, func() *string { value:="text";return &value }()} {
        result, err := ProbeNullableText(text)
        copied := result.Text
        if err != nil || (text==nil)!=(copied==nil) || (text!=nil && *text!=*copied) { t.Fatalf("nullable text: %v, %v",copied,err) }
    }
    input.Left[0].Type = 0
    if output.Left[0].Type != 9.5 { t.Fatal("result aliases input storage") }
    absent, err := ProbeRoundtrip(ProbeOptions{})
    if err != nil || absent.Title != nil || absent.Point != nil || len(absent.Left)!=0 || len(absent.Right)!=0 { t.Fatalf("absence: %+v, %v",absent,err) }
}
""".replace("LITERAL", literal)
                )
                result = subprocess.run(
                    [
                        "mise",
                        "exec",
                        "--no-deps",
                        "--",
                        "go",
                        "-C",
                        str(root),
                        "test",
                        "./...",
                    ],
                    cwd=Path(__file__).resolve().parents[2] / "bindings/go",
                    capture_output=True,
                    text=True,
                    check=False,
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
