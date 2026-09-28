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
typedef unsigned long long mln_map BIND("kind=handle;release=mln_map_close;dispose=mln_map_close;parent=none");
typedef int mln_status;
BIND("execution=immediate") void mln_map_close(mln_map map);
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


DIRECT_HEADER = """
#include <stdbool.h>
#define BIND(x) __attribute__((annotate("mln:" x)))
typedef unsigned long long mln_ticket BIND("kind=handle;release=mln_ticket_release;dispose=mln_ticket_release;parent=none");
typedef int mln_status;
BIND("execution=immediate") void mln_ticket_release(mln_ticket ticket);
typedef void (*mln_ticket_cancel)(void *context BIND("kind=context;lifetime=owner")) BIND("thread=native;failure=contain");
typedef void (*mln_runtime_callback_release)(void *context BIND("kind=context;lifetime=owner")) BIND("thread=native;failure=contain");
BIND("execution=immediate;registration=callback;user_data=context;release_callback=release;accepted_unless=declined")
mln_status mln_ticket_on_cancel(mln_ticket ticket, mln_ticket_cancel callback, void *context BIND("kind=context;ownership=borrowed"), mln_runtime_callback_release release, bool *declined BIND("direction=out"));
"""


def run_port(root, bound, notify, main):
    """Compile generated port callbacks against a notify stub and run main."""
    header, implementation = generate(bound)
    source = (
        """#include <cstddef>
#include <cstdint>
#include <cassert>
#include "sample.h"
"""
        + "\n".join(header)
        + notify
        + implementation
        + main
    )
    (root / "test.cpp").write_text(source)
    subprocess.run(
        ["clang++", "-std=c++20", str(root / "test.cpp"), "-o", str(root / "test")],
        check=True,
    )
    subprocess.run([str(root / "test")], check=True)


class NativePortTests(unittest.TestCase):
    def test_new_record_callback_generates_a_native_port_and_dart_registration(self):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "sample.h").write_text(HEADER)
            bound = compile_api(parse_headers(root))
            run_port(
                root,
                bound,
                """
static unsigned deliveries = 0;
template <std::size_t Count>
void dart_port_notify(void *context, const std::int64_t (&values)[Count]) {
  assert(context == reinterpret_cast<void*>(17));
  assert(Count == 3 && values[1] == 7 && values[2] == 9);
  ++deliveries;
}
""",
                """
int main() {
  const auto callback = reinterpret_cast<mln_sample_notification>(dart_port_function(MLN_ADAPTER_DART_PORT_SAMPLE_OPTIONS_CHANGED));
  assert(callback);
  callback(reinterpret_cast<void*>(17), {7, 9});
  assert(deliveries == 1);
}
""",
            )
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

    def test_direct_registration_generates_a_native_port_and_dart_method(self):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "sample.h").write_text(DIRECT_HEADER)
            bound = compile_api(parse_headers(root))
            run_port(
                root,
                bound,
                """
static unsigned deliveries = 0;
template <std::size_t Count>
void dart_port_notify(void *context, const std::int64_t (&values)[Count]) {
  assert(context == reinterpret_cast<void*>(17));
  assert(Count == 1 && values[0] == MLN_ADAPTER_DART_PORT_TICKET_ON_CANCEL_CALLBACK);
  ++deliveries;
}
""",
                """
int main() {
  const auto callback = reinterpret_cast<mln_ticket_cancel>(dart_port_function(MLN_ADAPTER_DART_PORT_TICKET_ON_CANCEL_CALLBACK));
  assert(callback);
  callback(reinterpret_cast<void*>(17));
  assert(deliveries == 1);
}
""",
            )
            emitted = dart.generate(bound)
            self.assertIn(
                "bool onCancel(TicketCancel callback) => withNativeArena(", emitted
            )
            self.assertIn("if (!isClosed) { callback(); }", emitted)
            self.assertIn("accepted = !declined.value;", emitted)
            self.assertIn("if (!accepted) { port.reject(); }", emitted)
            self.assertIn("raw.mln_adapter_dart_port_release", emitted)
            coverage = dart.coverage(bound)
            self.assertEqual(coverage["unsupported"], {})
            self.assertIn("mln_ticket_on_cancel", coverage["generated"])
