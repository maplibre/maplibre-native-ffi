"""Compile notification stubs added by a new descriptor in the input headers."""

import subprocess
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters import dart
from tools.bindgen.frontend import parse_headers
from tools.bindgen.native_ports import generate

HEADER = """
#define BIND(x) __attribute__((annotate("mln:" x)))
typedef unsigned long long mln_map;
typedef int mln_status;
typedef struct mln_completion { void *state; } mln_completion;
typedef struct mln_sample_point { unsigned int x; unsigned int y; } mln_sample_point;
typedef void (*mln_sample_notification)(void *context BIND("kind=context;lifetime=owner"), mln_sample_point point) BIND("thread=native;failure=contain");
typedef void (*mln_sample_release)(void *context BIND("kind=context;lifetime=owner")) BIND("thread=native;failure=contain");
typedef struct mln_sample_options {
  unsigned int size BIND("kind=size;default=sizeof");
  mln_sample_notification changed;
  void *context BIND("kind=context;lifetime=owner");
  mln_sample_release release;
} mln_sample_options BIND("kind=callback_registration;user_data=context;release=release");
BIND("receiver=map;execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_observe_sample(mln_map map, const mln_sample_options *options BIND("length=1"), const mln_completion *completion);
"""


class NativePortTests(unittest.TestCase):
    def test_new_record_callback_generates_a_native_port_and_dart_registration(self):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "sample.h").write_text(HEADER)
            bound = compile_api(parse_headers(root))
            header, implementation = generate(bound)
            source = (
                """#include <cstddef>
#include <cstdint>
#include <cassert>
#include "sample.h"
"""
                + "\n".join(header)
                + """
static unsigned deliveries = 0;
template <std::size_t Count>
void dart_port_notify(void *context, const std::int64_t (&values)[Count]) {
  assert(context == reinterpret_cast<void*>(17));
  assert(Count == 3 && values[1] == 7 && values[2] == 9);
  ++deliveries;
}
"""
                + implementation
                + """
int main() {
  const auto callback = reinterpret_cast<mln_sample_notification>(dart_port_function(MLN_ADAPTER_DART_PORT_SAMPLE_OPTIONS_CHANGED));
  assert(callback);
  callback(reinterpret_cast<void*>(17), {7, 9});
  assert(deliveries == 1);
}
"""
            )
            (root / "test.cpp").write_text(source)
            subprocess.run(
                [
                    "clang++",
                    "-std=c++20",
                    str(root / "test.cpp"),
                    "-o",
                    str(root / "test"),
                ],
                check=True,
            )
            subprocess.run([str(root / "test")], check=True)
            emitted = dart.generate(bound)
            self.assertIn(
                "Future<CommandCompletion> observeSample(SampleOptions options)",
                emitted,
            )
            self.assertIn(
                "SamplePoint(x: message[1] as int, y: message[2] as int)", emitted
            )
            self.assertIn("registration.releaseMemory?.call()", emitted)
            self.assertEqual(dart.coverage(bound)["unsupported"], {})
