"""Check the Go binding plan."""

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


if __name__ == "__main__":
    unittest.main()
