"""Compile notification stubs added by a new descriptor in the input headers."""

import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import parse_sources, protocol_header, require_tool, run

from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters import dart
from tools.bindgen.native_ports import generate


def run_port(test, root, bound, notify, main):
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
    require_tool(test, "clang++").run(
        test, "-std=c++20", str(root / "test.cpp"), "-o", str(root / "test"), cwd=root
    )
    run(test, [str(root / "test")], root)


class NativePortTests(unittest.TestCase):
    def test_new_record_callback_generates_a_native_port_and_dart_registration(self):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            header = protocol_header(groups=("retained_registration",))
            (root / "sample.h").write_text(header)
            bound = compile_api(parse_sources({"sample.h": header}))
            run_port(
                self,
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
            self.assertEqual(dart.coverage(bound)["unsupported"], {})

    def test_declinable_registration_generates_a_native_port_and_dart_method(self):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            header = protocol_header(groups=("declinable_registration",))
            (root / "sample.h").write_text(header)
            bound = compile_api(parse_sources({"sample.h": header}))
            run_port(
                self,
                root,
                bound,
                """
static unsigned deliveries = 0;
template <std::size_t Count>
void dart_port_notify(void *context, const std::int64_t (&values)[Count]) {
  assert(context == reinterpret_cast<void*>(17));
  assert(Count == 1 && values[0] == MLN_ADAPTER_DART_PORT_TICKET_CANCEL_HANDLER_CALLBACK);
  ++deliveries;
}
""",
                """
int main() {
  const auto callback = reinterpret_cast<mln_ticket_cancel>(dart_port_function(MLN_ADAPTER_DART_PORT_TICKET_CANCEL_HANDLER_CALLBACK));
  assert(callback);
  callback(reinterpret_cast<void*>(17));
  assert(deliveries == 1);
}
""",
            )
            coverage = dart.coverage(bound)
            self.assertEqual(coverage["unsupported"], {})
            self.assertIn("mln_ticket_on_cancel", coverage["generated"])
