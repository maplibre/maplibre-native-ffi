"""Check the Go binding plan."""

import re
import unittest

from support import parse, real_api

from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters import go
from tools.bindgen.emitters.go_callbacks import accepts_native
from tools.bindgen.emitters.go_operations import lower
from tools.bindgen.schema import validate

# A drain that publishes an owned batch of strided readings with a slice view
# of it, as the render session's frame-result drain does.
SLICE_DRAIN = """
typedef struct mln_reading {
  double value;
} mln_reading;
typedef struct mln_reading_view {
  uint32_t size;
  uint32_t reading_size;
  const mln_reading* readings BIND("length=reading_count;stride=reading_size");
  size_t reading_count;
} mln_reading_view BIND("kind=slice");
typedef unsigned long long mln_reading_batch BIND(
  "kind=handle;release=mln_reading_batch_release;"
  "dispose=mln_reading_batch_release"
);
void mln_reading_batch_release(mln_reading_batch batch);
BIND("execution=event_batch")
mln_status mln_drain_readings(
  mln_reading_batch* out_batch BIND("direction=out"),
  mln_reading_view* out_view BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
"""


class GoEmitterTests(unittest.TestCase):
    def test_a_drain_returns_its_slice_and_releases_its_batch(self):
        api = parse(SLICE_DRAIN, defines=("MLN_PROTOCOL_STANDARD_TYPES",))
        validate(api)
        source = go.generate(api)["generated_api.go"]
        self.assertIn("func DrainReadings() ([]Reading, error) {", source)
        # The copy runs only when native published a batch, and a deferred
        # release frees the batch even if the copy panics.
        self.assertIn(
            "bindingGetUnless(bindingGlobal(), int32(C.MLN_STATUS_NOT_READY)", source
        )
        self.assertIn(
            "}, func(arena *bindingArena) []Reading {\n"
            "defer C.mln_reading_batch_release(outBatch)\n",
            source,
        )
        # Neither the batch nor its view is public.
        self.assertNotIn("ReadingBatchHandle", source)
        self.assertNotIn("ReadingView", source)

    def test_an_input_only_union_rejects_a_value_its_switch_does_not_name(self):
        source = go.generate(real_api())["generated_api.go"]
        # A pointer to a variant also satisfies the sealed interface, so even a
        # union with no UnknownVariant rejects whatever its switch doesn't name.
        encode = re.search(
            r"case RenderedQueryGeometryLineStringVariant:[^\n]*", source
        )
        self.assertIsNotNone(encode)
        self.assertIn('default: arena.fail("unknown input union variant")', encode[0])
        self.assertNotIn("func (UnknownVariant) isRenderedQueryGeometry()", source)

    def test_a_callback_receives_its_slice_argument_as_a_slice(self):
        source = go.generate(real_api())["generated_api.go"]
        self.assertIn(
            "Callback func(events []RuntimeEvent)",
            source,
        )
        self.assertNotIn("EventBatchView", source)

    def test_a_func_names_its_arguments_and_returns_a_bool_result(self):
        source = go.generate(real_api())["generated_api.go"]
        self.assertIn(
            "Callback func(severity LogSeverity, event LogEvent, code int64, "
            "message string) bool",
            source,
        )
        # The log callback's failure is false, which a panic answers too.
        self.assertIn("(result C.bool) { result = false; ", source)

    def test_a_status_callback_returns_an_error_that_the_trampoline_maps(self):
        source = go.generate(real_api())["generated_api.go"]
        self.assertIn(
            "Callback func(kind ResourceKind, url string, "
            "outResponse *HttpHeaderTransformResponseScope) error",
            source,
        )
        self.assertIn(
            'result = C.mln_status(bindingCallbackStatus("mln_http_header_transform_callback", ',
            source,
        )
        self.assertIn("), C.MLN_STATUS_NATIVE_ERROR))", source)
        self.assertIn(
            "// answers native that status. Any other error, or a panic, is logged and\n"
            "// answers MLN_STATUS_NATIVE_ERROR.\n"
            "Callback func(kind ResourceKind",
            source,
        )

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
        self.assertEqual(len(owners), 9)
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
