"""Header conventions supply the metadata that a declaration's C shape implies."""

import unittest

from support import parse

from tools.bindgen.compiler import compile_api
from tools.bindgen.model import ModelError
from tools.bindgen.schema import validate
from tools.bindgen.semantic import DefaultSupport

# A handle, a callback registration, and an operation that opens the handle,
# each annotated only where it departs from convention.
REGISTRATION = """
typedef unsigned long long session BIND("kind=handle;release=session_close");
void session_close(session value);
typedef void (*notify)(void *context, const char *message);
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
    def test_versioned_values_and_commands_need_no_annotation(self):
        api = parse(groups=("conventions",))
        validate(api)
        bound = compile_api(api)
        label = bound.values["mln_label"]
        fields = {field.name: field for field in label.fields}
        self.assertEqual(
            (fields["size"].role, fields["size"].default), ("size", "sizeof")
        )
        self.assertEqual(
            (fields["reserved"].role, fields["reserved"].default), ("reserved", "0")
        )
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
            (notify.thread, notify.failure, notify.reentry, notify.context),
            ("native", "contain", "allow", "context"),
        )
        message = notify.parameters[1].value
        self.assertEqual(
            (message.kind, message.length, message.encoding, message.ownership),
            ("buffer", "nul", "utf8", "borrowed"),
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

    def test_an_annotation_that_restates_a_default_is_rejected(self):
        cases = (
            (
                "mln_status session_open(const listener *events,",
                'mln_status session_open(const listener *events BIND("length=1"),',
                "events: length=1",
            ),
            (
                "void session_close(",
                'BIND("execution=immediate") void session_close(',
                "execution=immediate",
            ),
            (
                "release=session_close",
                "release=session_close;parent=none",
                "parent=none",
            ),
            (
                "const char *message",
                'const char *message BIND("encoding=utf8;length=nul")',
                "length=nul",
            ),
            (
                "(*release)(void *context)",
                '(*release)(void *context BIND("kind=context"))',
                "kind=context",
            ),
            (
                "(*release)(void *context);",
                '(*release)(void *context) BIND("failure=contain");',
                "failure=contain",
            ),
            (
                "release=release_context",
                "release=release_context;user_data=context",
                "user_data=context",
            ),
            (
                'BIND("direction=out")',
                'BIND("direction=out;ownership=owned")',
                "ownership=owned",
            ),
            (
                "void *native_window",
                'void *native_window BIND("kind=native_pointer")',
                "kind=native_pointer",
            ),
        )
        for before, after, item in cases:
            with (
                self.subTest(item=item),
                self.assertRaisesRegex(ModelError, f"{item} restates the default"),
            ):
                parse(REGISTRATION.replace(before, after, 1))
        with self.assertRaisesRegex(ModelError, "kind=size restates the default"):
            parse(
                groups=("conventions",),
                source='typedef struct sized { uint32_t size BIND("kind=size"); } sized;',
            )
        with self.assertRaisesRegex(ModelError, "result=void restates the default"):
            parse(
                groups=("conventions",),
                source='BIND("execution=operation;result=void") mln_status mln_map_flush('
                "mln_map map, const mln_completion *completion, "
                "mln_diagnostic *out_diagnostic);",
            )

    def test_a_default_constructor_is_inferred_only_when_it_is_unambiguous(self):
        source = """
typedef struct options { double zoom; } options;
options make_options(void);
options other_options(void);
"""
        bound = compile_api(parse(source))
        self.assertIsNone(bound.values["options"].default)
        explicit = compile_api(
            parse(
                source.replace("} options;", '} options BIND("default=make_options");')
            )
        )
        self.assertEqual(explicit.values["options"].default, "make_options")

    def test_an_annotation_that_names_an_absent_function_is_rejected(self):
        source = """
typedef enum decision : unsigned { PASS = 0, TAKE = 1 } decision;
typedef unsigned long long request BIND("kind=handle;release=request_close");
void request_close(request value);
typedef unsigned (*provider)(void *context, request handle) BIND("enum=decision;failure=PASS;ANNOTATION");
"""
        for annotation in (
            "reentry=protocol;reentry_owner=handle;reentry_calls=request_close,missing",
            "complete=missing",
            "cancel_registration=missing",
            "wait_retired=missing",
        ):
            key = annotation.rsplit(";", 1)[-1].split("=")[0]
            with (
                self.subTest(key=key),
                self.assertRaisesRegex(
                    ModelError, f"{key} names an absent function 'missing'"
                ),
            ):
                validate(parse(source.replace("ANNOTATION", annotation)))
        with self.assertRaisesRegex(
            ModelError, "release names an absent function 'missing'"
        ):
            validate(
                parse(
                    source.replace(";ANNOTATION", "").replace(
                        "release=request_close", "release=missing"
                    )
                )
            )

    def test_a_callback_failure_must_be_a_result_it_can_return(self):
        source = """
typedef enum decision : unsigned { PASS = 0, TAKE = 1 } decision;
typedef unsigned (*provider)(void *context) BIND("enum=decision;failure=PASS");
"""
        validate(parse(source))
        for before, after, error in (
            ("failure=PASS", "failure=SKIP", "failure must name a value of decision"),
            ("failure=PASS", "failure=contain", "declares the failure it returns"),
            (";failure=PASS", "", "declares the failure it returns"),
        ):
            with self.subTest(after=after), self.assertRaisesRegex(ModelError, error):
                validate(parse(source.replace(before, after)))

    def test_an_unknown_kind_is_rejected(self):
        with self.assertRaisesRegex(ModelError, "unsupported kind='contxt'"):
            validate(parse('void attach(void *state BIND("kind=contxt"));'))


if __name__ == "__main__":
    unittest.main()
