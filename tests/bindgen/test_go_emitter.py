"""Check the Go binding plan."""

import unittest

from support import real_api

from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters.go_callbacks import accepts_native
from tools.bindgen.emitters.go_operations import lower


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


if __name__ == "__main__":
    unittest.main()
