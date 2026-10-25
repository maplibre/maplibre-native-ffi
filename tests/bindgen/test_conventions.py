"""Header conventions supply the metadata that a declaration's C shape implies."""

import unittest

from support import parse

from tools.bindgen.compiler import compile_api
from tools.bindgen.model import ModelError
from tools.bindgen.schema import validate
from tools.bindgen.semantic import DefaultSupport, bind

# A handle, a callback registration, and an operation that opens the handle,
# each annotated only where it departs from convention.
REGISTRATION = """
typedef unsigned long long session BIND("kind=handle;release=session_close");
void session_close(session value);
typedef void (*notify)(void *context, mln_buffer_view message);
typedef void (*release)(void *context);
typedef struct listener {
  notify callback;
  void *context BIND("kind=context");
  release release_context;
} listener BIND("kind=callback_registration;release=release_context");
mln_status session_open(const listener *events, void *native_window,
  session *out_session BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""


class ConventionTests(unittest.TestCase):
    def test_completion_nullability_follows_execution(self):
        api = parse("""BIND("execution=command")
mln_status submit(const mln_completion *completion BIND("nullable=true"), mln_diagnostic *out_diagnostic);
""")
        with self.assertRaisesRegex(
            ModelError, "nullable is not allowed on a completion"
        ):
            validate(api)

    def test_versioned_values_and_commands_need_no_annotation(self):
        api = parse(groups=("conventions",))
        validate(api)
        bound = compile_api(api)
        label = bound.values["mln_label"]
        fields = {field.name: field for field in label.fields}
        self.assertEqual(fields["size"].role, "size")
        self.assertEqual(fields["reserved"].role, "reserved")
        self.assertEqual(fields["fields"].role, "presence_mask")
        self.assertEqual(
            (fields["text"].value.kind, fields["text"].value.encoding),
            ("buffer", "utf8"),
        )
        self.assertEqual(label.default, "mln_label_default")
        constructor = bound.operations_by_name["mln_label_default"]
        self.assertEqual(
            (constructor.execution, constructor.support),
            ("immediate", DefaultSupport("mln_label")),
        )
        command = bound.operations_by_name["mln_map_set_label"]
        self.assertIsNone(command.result)
        self.assertEqual(
            {
                key: command.function.metadata[key]
                for key in ("execution", "result", "shape", "ownership")
            },
            {
                "execution": "command",
                "result": "void",
                "shape": "none",
                "ownership": "value",
            },
        )
        (options,) = command.inputs[1:]
        self.assertEqual(
            (options.value.kind, options.value.length, options.value.ownership),
            ("reference", "1", "borrowed"),
        )

    def test_pointers_callbacks_and_registrations_need_no_annotation(self):
        bound = compile_api(parse(REGISTRATION))
        notify = bound.callbacks["notify"]
        self.assertEqual(
            (notify.failure, notify.context),
            ("contain", "context"),
        )
        message = notify.parameters[1].value
        self.assertEqual(
            (message.kind, message.buffer_form, message.encoding),
            ("buffer", "view", "utf8"),
        )
        self.assertEqual(notify.parameters[0].value.lifetime, "owner")
        operation = bound.operations_by_name["session_open"]
        (registration,) = operation.registrations
        self.assertEqual(
            (registration.user_data, registration.release),
            ("context", "release_context"),
        )
        window = next(p for p in operation.inputs if p.name == "native_window")
        self.assertEqual(window.value.kind, "native_pointer")
        (owner,) = operation.owned_outputs
        self.assertEqual((owner.parameter, owner.handle.parent), ("out_session", None))

    def test_terminated_text_is_rejected(self):
        # Text is an mln_buffer_view; a character pointer must name its length.
        with self.assertRaisesRegex(ModelError, "text requires an mln_buffer_view"):
            validate(
                parse(
                    REGISTRATION.replace(
                        "mln_buffer_view message", "const char *message"
                    )
                )
            )
        with self.assertRaisesRegex(ModelError, "text requires an mln_buffer_view"):
            validate(
                parse(
                    "mln_status set_label(const char *label, mln_diagnostic *out_diagnostic);"
                )
            )

    def test_an_annotation_that_restates_a_default_is_rejected(self):
        with self.assertRaisesRegex(
            ModelError, "execution=immediate restates the default"
        ):
            parse(
                REGISTRATION.replace(
                    "void session_close(",
                    'BIND("execution=immediate") void session_close(',
                    1,
                )
            )
        with self.assertRaisesRegex(ModelError, "kind=size restates the default"):
            parse(
                groups=("conventions",),
                source='typedef struct sized { uint32_t size BIND("kind=size"); } sized;',
            )

    def test_an_annotation_writes_only_the_vocabulary_headers_use(self):
        source = (
            REGISTRATION
            + """
BIND("execution=query;result=double") mln_status session_scale(
  session value, const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("execution=command") mln_status session_flush(
  session value, const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""
        )
        validate(parse(source))
        command = 'BIND("execution=command'
        message = "mln_buffer_view message"
        # A key that no header writes.
        with self.assertRaisesRegex(ModelError, "unknown metadata key 'receiver'"):
            validate(parse(source.replace(command, command + ";receiver=value", 1)))
        # A value that no header writes, where it departs from the default.
        with self.assertRaisesRegex(ModelError, "unsupported lifetime='completion'"):
            parse(source.replace(message, message + ' BIND("lifetime=completion")', 1))

    def test_a_default_constructor_is_inferred_only_when_it_is_unambiguous(self):
        source = """
typedef struct options { double zoom; } options;
options make_options(void);
options other_options(void);
void use_options(const options *value);
"""
        bound = compile_api(parse(source))
        self.assertIsNone(bound.values["options"].default)

    def test_an_annotation_that_names_an_absent_function_is_rejected(self):
        source = """
typedef enum decision : unsigned { PASS = 0, TAKE = 1 } decision;
typedef unsigned long long request BIND("kind=handle;release=request_close");
void request_close(request value);
typedef decision (*provider)(void *context, request handle) BIND("failure=PASS");
"""
        with self.assertRaisesRegex(
            ModelError, "release names an absent function 'missing'"
        ):
            validate(parse(source.replace("release=request_close", "release=missing")))

    def test_a_callback_borrows_its_registration_receiver(
        self,
    ):
        source = """
typedef unsigned long long request BIND("kind=handle;release=request_close");
typedef unsigned long long other BIND("kind=handle;release=other_close");
void request_close(request value);
void other_close(other value);
typedef void (*cancel)(void *context, request owner) BIND("borrowed_owner=owner");
typedef void (*release)(void *context);
typedef struct watcher { unsigned size; cancel callback; void *context BIND("kind=context"); release retire; } watcher BIND("kind=callback_registration;release=retire");
mln_status watch(request value, const watcher *handler, mln_diagnostic *out_diagnostic);
"""
        compile_api(parse(source))
        for before, after, error in (
            (
                "borrowed_owner=owner",
                "borrowed_owner=missing",
                "borrowed_owner names an absent parameter 'missing'",
            ),
        ):
            with self.subTest(after=after), self.assertRaisesRegex(ModelError, error):
                bind(parse(source.replace(before, after)), require_complete=True)

    def test_a_callback_failure_must_be_a_result_it_can_return(self):
        source = """
typedef enum decision : unsigned { PASS = 0, TAKE = 1 } decision;
typedef decision (*provider)(void *context) BIND("failure=PASS");
"""
        validate(parse(source))
        for before, after, error in (
            ("failure=PASS", "failure=SKIP", "failure must name a value of decision"),
            ("failure=PASS", "failure=contain", "declares the failure it returns"),
            (' BIND("failure=PASS")', "", "declares the failure it returns"),
        ):
            with self.subTest(after=after), self.assertRaisesRegex(ModelError, error):
                validate(parse(source.replace(before, after)))

    def test_an_unknown_kind_is_rejected(self):
        with self.assertRaisesRegex(ModelError, "unsupported kind='contxt'"):
            validate(parse('void attach(void *state BIND("kind=contxt"));'))


if __name__ == "__main__":
    unittest.main()
