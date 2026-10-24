"""Check the Go binding plan."""

import re
import unittest

from support import parse, real_api

from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters import go
from tools.bindgen.emitters.go_callbacks import accepts_native
from tools.bindgen.emitters.go_operations import lower
from tools.bindgen.schema import validate


class GoEmitterTests(unittest.TestCase):
    def test_wakes_and_synchronous_registrations_take_a_native_registration(self):
        # Every other registration takes only Go funcs, whose calls enter the
        # Go runtime.
        values, _, _, _ = lower(compile_api(real_api()))
        native = {
            plan.native
            for plan in values.records.values()
            if plan.registration and accepts_native(values, plan)
        }
        self.assertEqual(native, {"mln_queue_lock", "mln_wake"})

    def test_a_dispose_that_is_the_release_closes_with_a_completion(self):
        api = parse("""
typedef unsigned long long mln_lease BIND("kind=handle;release=mln_lease_dispose;dispose=mln_lease_dispose");
BIND("execution=lifecycle")
mln_status mln_lease_dispose(mln_lease lease, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        validate(api)
        self.assertEqual(go.coverage(api)["generated"], ["mln_lease_dispose"])
        source = go.generate(api)["generated_api.go"]
        # Close returns a future; the owner's disposer passes a null completion.
        self.assertIn(
            "func (receiver *LeaseHandle) Close() (*Future[struct{}], error)", source
        )
        self.assertIn(
            "C.mln_lease_dispose(C.mln_lease(raw), completion, diagnostic)", source
        )
        self.assertIn("C.mln_lease_dispose(C.mln_lease(raw), nil, nil)", source)
        # Dispose is the synchronous form, which passes a null completion.
        self.assertIn("func (receiver *LeaseHandle) Dispose() error", source)
        self.assertIn("C.mln_lease_dispose(C.mln_lease(raw), nil, diagnostic)", source)

    def test_dispose_is_public_only_where_c_has_one_besides_a_void_release(self):
        source = go.generate(real_api())["generated_api.go"]
        disposes = {
            line.split("*", 1)[1].split(")", 1)[0]
            for line in source.splitlines()
            if line.startswith("func (receiver *") and ") Dispose() error" in line
        }
        self.assertEqual(
            disposes, {"MapHandle", "RenderSessionHandle", "RuntimeHandle"}
        )

    def test_every_owner_declares_its_state_members(self):
        source = go.generate(real_api())["generated_api.go"]
        owners = [
            line.split()[1]
            for line in source.splitlines()
            if line.endswith("struct { *bindingOwner }")
        ]
        self.assertEqual(len(owners), 10)
        for owner in owners:
            for member in ("Id() uint64", "IsClosed() bool", "String() string"):
                self.assertIn(f"func (handle *{owner}) {member}", source)
        # Only the runtime and the map are event sources.
        sources = re.findall(
            r"// Id returns the native handle value, which runtime events report"
            r" as their\n(?://.*\n)*func \(handle \*(\w+)\) Id\(\)",
            source,
        )
        self.assertEqual(set(sources), {"MapHandle", "RuntimeHandle"})

    def test_operations_cannot_take_owner_member_names(self):
        api = parse("""
typedef unsigned long long mln_host BIND("kind=handle;release=mln_host_destroy");
void mln_host_destroy(mln_host host);
mln_status mln_host_is_closed(mln_host host, bool *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
""")
        result = go.coverage(api)
        self.assertEqual(result["generated"], ["mln_host_destroy"])
        self.assertIn(
            "reserved for owner state", result["unsupported"]["mln_host_is_closed"]
        )


if __name__ == "__main__":
    unittest.main()
