"""Semantic mutation tests shared by every static language backend."""

import re
import unittest
from typing import get_args

from support import parse, parse_sources, protocol_header

from tools.bindgen.emitters import dart
from tools.bindgen.model import ModelError
from tools.bindgen.semantic import (
    DeliveryPlan,
    DisposeSupport,
    OperationShape,
    bind,
    enum_member_prefix,
)


class SemanticTests(unittest.TestCase):
    def parse(self, source):
        return parse(source)

    def test_scalar_carriers_preserve_portable_typedefs_across_host_abis(self):
        for underlying in ("long", "long long"):
            with self.subTest(underlying=underlying):
                model = bind(
                    self.parse(f"""
typedef unsigned {underlying} uint64_t;
typedef {underlying} int64_t;
typedef unsigned {underlying} size_t;
typedef uint64_t custom_unsigned;
typedef custom_unsigned nested_unsigned;
typedef int64_t custom_signed;
typedef size_t custom_count;
typedef enum flags : uint64_t {{ FLAG_HIGH = 0x100000000ULL }} flags;
typedef struct values {{ nested_unsigned id; custom_signed offset; custom_count count; flags mask; }} values;
mln_status read_values(values *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""),
                    require_complete=True,
                )
                self.assertEqual(
                    {
                        field.name: field.value.scalar_carrier
                        for field in model.values["values"].fields
                    },
                    {
                        "id": "uint64_t",
                        "offset": "int64_t",
                        "count": "size_t",
                        "mask": "uint64_t",
                    },
                )

    def test_enum_members_drop_the_prefix_their_type_name_shares(self):
        for native, constants, prefix in (
            (
                "mln_status",
                ["MLN_STATUS_OK", "MLN_STATUS_INVALID_STATE"],
                "MLN_STATUS_",
            ),
            (
                "mln_rendered_feature_field",
                ["MLN_RENDERED_FEATURE_SOURCE_ID", "MLN_RENDERED_FEATURE_STATE"],
                "MLN_RENDERED_FEATURE_",
            ),
            (
                "mln_source_feature_field",
                ["MLN_SOURCE_FEATURE_SOURCE_LAYER_ID"],
                "MLN_SOURCE_FEATURE_",
            ),
        ):
            with self.subTest(native=native):
                self.assertEqual(prefix, enum_member_prefix(constants, native))

    def test_view_scope_and_empty_union_have_verified_relationships(self):
        source = """
typedef unsigned long owner BIND("kind=handle;release=release_owner;view_begin=begin_view;view_end=end_view");
void release_owner(owner value);
mln_status begin_view(owner value, void **scope BIND("direction=out;kind=context"), mln_diagnostic *out_diagnostic);
void end_view(void *scope BIND("kind=context"));
typedef enum event_tag : int { NONE = 0, NUMBER = -1 } event_tag;
typedef struct event { event_tag tag; union { double number BIND("variant=NUMBER"); } payload BIND("tag=tag;empty_variant=NONE"); } event;
mln_status read_event(event *value BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""
        model = bind(self.parse(source), require_complete=True)
        self.assertEqual(model.handles["owner"].view_begin, "begin_view")
        self.assertEqual(model.handles["owner"].view_end, "end_view")
        self.assertEqual(model.values["event_tag"].enum_underlying.canonical, "int")
        payload = next(
            field.value
            for field in model.values["event"].fields
            if field.name == "payload"
        )
        self.assertEqual(payload.empty_variant, ("NONE", 0))
        for before, after, message in (
            ("empty_variant=NONE", "empty_variant=NUMBER", "unused value"),
            (
                "void end_view(void *scope",
                "void end_view(unsigned scope",
                "view scope requires",
            ),
        ):
            with self.subTest(after=after), self.assertRaisesRegex(ModelError, message):
                bind(self.parse(source.replace(before, after)), require_complete=True)

    def test_a_root_constructor_is_a_factory_and_a_global_names_its_verb_first(
        self,
    ):
        source = """
typedef unsigned long long mln_engine BIND("kind=handle;release=mln_engine_release;dispose=mln_engine_release");
void mln_engine_release(mln_engine engine);
mln_status mln_engine_create(mln_engine *out_engine BIND("direction=out"), mln_diagnostic *out_diagnostic);
mln_status mln_level_set_volume(unsigned volume, mln_diagnostic *out_diagnostic);
mln_status mln_level_peak(unsigned *out_peak BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        operations = {op.name: op for op in api.operations}
        create = operations["mln_engine_create"]
        self.assertEqual((create.factory, create.member), ("mln_engine", "create"))
        self.assertEqual(operations["mln_level_set_volume"].member, "set_level_volume")
        # A global without a leading verb keeps its order.
        self.assertEqual(operations["mln_level_peak"].member, "level_peak")
        self.assertIsNone(operations["mln_level_peak"].factory)

    def test_a_record_of_only_a_tag_and_its_union_presents_the_union(self):
        source = """
typedef enum choice : unsigned { TEXT = 1, NUMBER = 2 } choice;
typedef struct value {
  unsigned size;
  choice tag;
  union {
    double text BIND("variant=TEXT");
    double number BIND("variant=NUMBER");
  } data BIND("tag=tag");
} value;
mln_status write_value(const value *input, mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        member = api.values["value"].union_member
        self.assertEqual(member.name, "data")
        # Another public member keeps the record a record.
        widened = bind(
            self.parse(
                source.replace("  choice tag;", "  choice tag;\n  double scale;")
            ),
            require_complete=True,
        )
        self.assertIsNone(widened.values["value"].union_member)

    def test_nested_count_and_union_are_resolved_once(self):
        source = """
typedef enum choice : unsigned { TEXT = 1, NUMBER = 2 } choice;
typedef struct text { const char *data BIND("length=count"); unsigned count; } text;
typedef struct value {
  choice tag;
  union {
    text text BIND("variant=TEXT");
    double number BIND("variant=NUMBER");
  } data BIND("tag=tag");
} value;
mln_status read_value(value *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        result = api.operations[0].outputs[0].value.element
        union = result.fields[1]
        self.assertEqual(union.presence.tag, "tag")
        self.assertEqual(union.value.fields[0].presence.variant, "TEXT")
        counted = union.value.fields[0].value.fields[0].value
        self.assertEqual(
            (counted.kind, counted.length, counted.encoding),
            ("buffer", "count", "utf8"),
        )
        with self.assertRaisesRegex(ModelError, "length names an absent field"):
            bind(
                self.parse(source.replace("length=count", "length=missing")),
                require_complete=True,
            )
        with self.assertRaisesRegex(
            ModelError, "variant requires a declared enum value"
        ):
            bind(
                self.parse(source.replace("variant=TEXT", "variant=ABSENT")),
                require_complete=True,
            )

    def test_each_struct_is_versioned_by_one_thing(self):
        header = protocol_header(groups=("versioning", "strided_records"))
        bind(parse_sources({"api.h": header}), require_complete=True)
        message = "struct versioned by its container or stride must not carry size"
        for name in (
            "mln_probe_span",
            "mln_probe_survey",
            "mln_probe_tally",
            "mln_probe_reading",
        ):
            with (
                self.subTest(record=name),
                self.assertRaisesRegex(ModelError, rf"{name}: {message}"),
            ):
                bind(
                    parse_sources(
                        {
                            "api.h": header.replace(
                                f"typedef struct {name} {{\n",
                                f"typedef struct {name} {{\n  uint32_t size;\n",
                            )
                        }
                    )
                )

    def test_enums_that_only_masks_and_tags_use_stay_private(self):
        source = """
typedef enum BIND("kind=bitmask") field : unsigned { CENTER = 1 } field;
typedef unsigned fields;
typedef enum tag : unsigned { NUMBER = 1 } tag;
typedef enum shared : unsigned { SHARED_A = 1 } shared;
typedef struct options {
  fields fields;
  double latitude BIND("mask=fields;bit=CENTER");
  tag kind;
  union { double number BIND("variant=NUMBER"); } payload BIND("tag=kind");
  shared shared_tag;
  union { double other BIND("variant=SHARED_A"); } other BIND("tag=shared_tag");
  shared mode;
} options;
mln_status write_options(const options *value, mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        self.assertEqual(api.control_enums, {"field", "tag"})
        self.assertNotIn("field", api.public_values)
        self.assertNotIn("tag", api.public_values)
        # A value field that also uses the enum keeps it public.
        self.assertIn("shared", api.public_values)

    def test_enums_that_only_completion_results_use_stay_private(self):
        api = bind(
            parse(
                'BIND("execution=command") mln_status submit('
                "const mln_completion *completion, mln_diagnostic *out_diagnostic);",
                defines=("MLN_PROTOCOL_COMPLETION_RUNTIME",),
            ),
            require_complete=True,
        )
        self.assertIn("mln_command_disposition", api.control_enums)
        self.assertNotIn("mln_command_disposition", api.public_values)

    def test_a_presence_bit_guards_one_member_of_a_record(self):
        source = """
typedef enum BIND("kind=bitmask") field : unsigned { CENTER = 1, ZOOM = 2 } field;
typedef unsigned fields;
typedef struct options {
  fields fields;
  double latitude BIND("mask=fields;bit=CENTER");
  double longitude BIND("mask=fields;bit=ZOOM");
} options;
mln_status write_options(const options *value, mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        value = api.operations[0].inputs[0].value.element
        self.assertEqual(value.fields[0].role, "presence_mask")
        self.assertEqual(value.fields[0].value.enum_kind, "bitmask")
        with self.assertRaisesRegex(
            ModelError,
            r"options\.longitude: presence bit CENTER guards more than one "
            "member; embed a record",
        ):
            bind(
                self.parse(source.replace("bit=ZOOM", "bit=CENTER")),
                require_complete=True,
            )
        with self.assertRaisesRegex(ModelError, "both mask and bit") as raised:
            bind(
                self.parse(re.sub(r";bit=\w+", "", source)),
                require_complete=True,
            )
        self.assertNotIn("guards more than one member", str(raised.exception))
        with self.assertRaisesRegex(
            ModelError,
            r"options\.item_count: a count takes its array's presence; "
            "drop mask and bit",
        ):
            bind(
                self.parse(
                    source.replace(
                        'double longitude BIND("mask=fields;bit=ZOOM");',
                        "const double *items "
                        'BIND("length=item_count;mask=fields;bit=ZOOM");\n'
                        'unsigned long item_count BIND("mask=fields;bit=ZOOM");',
                    )
                ),
                require_complete=True,
            )
        with self.assertRaisesRegex(ModelError, "mask must name a sibling field"):
            bind(
                self.parse(
                    source.replace("mask=fields;bit=ZOOM", "mask=a.fields;bit=ZOOM")
                ),
                require_complete=True,
            )

    def test_a_required_member_is_an_unmasked_struct_value(self):
        source = """
typedef enum BIND("kind=bitmask") field : unsigned { ZOOM = 1 } field;
typedef unsigned fields;
typedef enum choice : unsigned { TEXT = 1, NUMBER = 2 } choice;
typedef struct options {
  unsigned size;
  fields fields;
  double width BIND("required=true");
  double zoom BIND("mask=fields;bit=ZOOM");
  const double *items BIND("length=item_count");
  unsigned long item_count;
  choice tag;
  union {
    double text BIND("variant=TEXT");
    double number BIND("variant=NUMBER");
  } data BIND("tag=tag");
} options;
mln_status write_options(const options *value, mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        value = api.operations[0].inputs[0].value.element
        required = {field.name: field.required for field in value.fields}
        self.assertTrue(required["width"])
        self.assertFalse(required["zoom"])
        self.assertTrue(
            bind(
                self.parse(
                    source.replace(
                        'BIND("length=item_count")',
                        'BIND("length=item_count;required=true")',
                    )
                ),
                require_complete=True,
            )
            .operations[0]
            .inputs[0]
            .value.element.fields[4]
            .required
        )
        for before, after in (
            (
                'double zoom BIND("mask=fields;bit=ZOOM")',
                'double zoom BIND("mask=fields;bit=ZOOM;required=true")',
            ),
            ("unsigned size;", 'unsigned size BIND("required=true");'),
            (
                "unsigned long item_count;",
                'unsigned long item_count BIND("required=true");',
            ),
            ("choice tag;", 'choice tag BIND("required=true");'),
            ("fields fields;", 'fields fields BIND("required=true");'),
            (
                'double text BIND("variant=TEXT")',
                'double text BIND("variant=TEXT;required=true")',
            ),
        ):
            with (
                self.subTest(after=after),
                self.assertRaisesRegex(
                    ModelError, "required applies only to an unmasked value member"
                ),
            ):
                bind(self.parse(source.replace(before, after)), require_complete=True)
        with self.assertRaisesRegex(ModelError, "unsupported required='false'"):
            bind(
                self.parse(source.replace("required=true", "required=false")),
                require_complete=True,
            )

    def test_nested_registration_and_callback_context_are_resolved(self):
        api = bind(
            self.parse("""
typedef void (*notify)(void *state);
typedef void (*release)(void *state);
typedef struct signals { notify signal; void *state BIND("kind=context"); release retire; } signals BIND("kind=callback_registration;release=retire");
typedef struct settings { signals wake; } settings;
mln_status create(const settings *options, mln_diagnostic *out_diagnostic);
"""),
            require_complete=True,
        )
        registration = api.operations[0].registrations[0]
        self.assertEqual(
            (registration.parameter, registration.path, registration.callbacks),
            ("options", ("wake",), ("signal",)),
        )
        self.assertEqual(api.values["signals"].registration.user_data, "state")
        self.assertEqual(api.values["signals"].registration.release, "retire")
        self.assertEqual(api.callbacks["notify"].context, "state")

    def test_strided_items_resolve_borrowed_message_views(self):
        source = """
typedef struct item { mln_buffer_view message; } item;
typedef struct batch {
 unsigned int stride;
 const item *items BIND("length=count;stride=stride");
 unsigned int count;
} batch;
mln_status capture(batch *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        value = next(f.value for f in api.values["batch"].fields if f.name == "items")
        self.assertEqual(value.stride, "stride")
        callback_source = (
            source
            + """
typedef void (*on_batch)(void *user_data, const batch *events);
typedef void (*release_events)(void *user_data);
typedef struct handler {
 on_batch callback;
 void *user_data BIND("kind=context");
 release_events release_user_data;
} handler BIND("kind=callback_registration;release=release_user_data");
mln_status subscribe(const handler *value, mln_diagnostic *out_diagnostic);
"""
        )
        callbacks = bind(self.parse(callback_source), require_complete=True)
        callback_events = callbacks.callbacks["on_batch"].parameters[-1].value
        self.assertEqual(callback_events.unwrapped.native, "batch")
        self.assertEqual(
            next(
                f.value for f in callbacks.values["batch"].fields if f.name == "items"
            ).element,
            value.element,
        )
        with self.assertRaisesRegex(ModelError, "input reconstruction contract"):
            bind(
                self.parse(
                    source.replace(
                        'batch *out BIND("direction=out")',
                        "const batch *input",
                    )
                ),
                require_complete=True,
            )
        roles = {field.name: field.role for field in api.values["batch"].fields}
        self.assertEqual(roles["stride"], "stride")
        self.assertEqual(value.element.fields[0].value.kind, "buffer")
        with self.assertRaisesRegex(ModelError, "stride requires"):
            bind(
                self.parse(source.replace("stride=stride", "stride=missing")),
                require_complete=True,
            )

    def test_counted_character_encoding_resolves_as_buffer(self):
        api = bind(
            self.parse("""
mln_status set_text(const char *value BIND("length=count"), unsigned int count, mln_diagnostic *out_diagnostic);
"""),
            require_complete=True,
        )
        value = api.operations[0].inputs[0].value
        self.assertEqual(
            (value.kind, value.encoding, value.length), ("buffer", "utf8", "count")
        )
        byte_api = bind(
            self.parse("""
typedef unsigned char byte;
typedef struct payload { const byte *bytes BIND("length=count;encoding=bytes"); unsigned int count; } payload;
mln_status capture(payload *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""),
            require_complete=True,
        )
        byte_value = byte_api.values["payload"].fields[0].value
        self.assertEqual((byte_value.kind, byte_value.encoding), ("buffer", "bytes"))

    def test_callback_response_preserves_scope_and_native_receiver(self):
        source = """
typedef struct reply { void *context BIND("kind=context"); } reply BIND("kind=callback_response");
typedef void (*answer)(reply *out BIND("direction=out"));
mln_status reply_set(reply *response, int value, mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        response = api.values["reply"].response
        self.assertEqual(response.methods, ("reply_set",))
        self.assertEqual(
            api.operations_by_name["reply_set"].scoped_receiver, "response"
        )

    def test_borrowed_output_view_tracks_owner_and_parent_invalidation(self):
        source = """
typedef unsigned long long parent BIND("kind=handle;release=parent_close;dispose=parent_close;abandon=parent_abandon");
typedef unsigned long long child BIND("kind=handle;release=child_close;dispose=child_close;parent=parent;view_begin=child_begin;view_end=child_end");
typedef struct view { void *texture BIND("lifetime=owner"); int width; } view;
mln_status parent_close(parent value, mln_diagnostic *out_diagnostic);
mln_status parent_abandon(parent value, mln_diagnostic *out_diagnostic);
mln_status child_close(child value, mln_diagnostic *out_diagnostic);
mln_status child_begin(child value, void **out_token BIND("direction=out;kind=context"), mln_diagnostic *out_diagnostic);
void child_end(void *token BIND("kind=context"));
BIND("view_owner=owner") mln_status child_get_view(child owner, view *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        view = api.operations_by_name["child_get_view"].view
        # The scope operations back the view and never become public members.
        self.assertTrue({"child_begin", "child_end"}.isdisjoint(api.operations_by_name))
        self.assertEqual(view.owner_parameter, "owner")
        self.assertEqual(view.owner.native, "child")
        self.assertEqual(
            (view.output.name, view.begin, view.end, view.stem),
            ("out", "child_begin", "child_end", "view"),
        )
        # The scope guards only the fields that the owner lends.
        self.assertEqual(
            {f.name: f.scoped for f in view.output.value.element.fields},
            {"texture": True, "width": False},
        )
        # A view lends at least one field, and only a lent record has one.
        with self.assertRaisesRegex(ModelError, "a lifetime=owner field"):
            bind(
                self.parse(source.replace(' BIND("lifetime=owner")', "")),
                require_complete=True,
            )
        with self.assertRaisesRegex(ModelError, "view owner lends"):
            bind(
                self.parse(
                    source
                    + 'typedef struct loose { int id BIND("lifetime=owner"); } loose;\n'
                    + 'mln_status child_read(child owner, loose *out BIND("direction=out"), mln_diagnostic *out_diagnostic);\n'
                ),
                require_complete=True,
            )
        with self.assertRaisesRegex(ModelError, "input handle"):
            bind(
                self.parse(source.replace("view_owner=owner", "view_owner=out")),
                require_complete=True,
            )
        # A view needs its owner's scope operations.
        with self.assertRaisesRegex(ModelError, "declares view_begin and view_end"):
            bind(
                self.parse(
                    source.replace(";view_begin=child_begin;view_end=child_end", "")
                ),
                require_complete=True,
            )

    def test_ambiguous_pointer_cannot_become_a_generated_operation(self):
        source = """
mln_status write_data(const double *values, mln_diagnostic *out_diagnostic);
"""
        model = bind(self.parse(source))
        self.assertFalse(model.operations)
        self.assertIn("write_data", model.unsupported)
        with self.assertRaisesRegex(ModelError, "pointer requires length"):
            bind(self.parse(source), require_complete=True)

    def test_record_pointer_addresses_one_record_unless_counted(self):
        source = """
typedef struct coordinate { double latitude; double longitude; } coordinate;
mln_status project(
  const coordinate *coordinates BIND("length=count"), unsigned count, mln_diagnostic *out_diagnostic);
"""
        model = bind(self.parse(source), require_complete=True)
        array = model.operations[0].inputs[0].value
        self.assertEqual(
            (array.kind, array.length, array.element.kind), ("array", "count", "record")
        )
        single = bind(
            self.parse(source.replace(' BIND("length=count")', "")),
            require_complete=True,
        )
        reference = single.operations[0].inputs[0].value
        self.assertEqual(
            (reference.kind, reference.length, reference.ownership),
            ("reference", "1", "borrowed"),
        )
        with self.assertRaisesRegex(ModelError, "length=1 restates the default"):
            self.parse(source.replace("length=count", "length=1"))

    def test_enum_output_keeps_output_storage_indirection(self):
        model = bind(
            self.parse("""
typedef enum flags : unsigned { FIRST = 1, SECOND = 2 } flags;
mln_status read_flags(
  flags *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""),
            require_complete=True,
        )
        output = model.operations[0].outputs[0].value
        self.assertEqual(output.kind, "reference")
        self.assertEqual(output.element.kind, "enum")

    def test_callback_signature_and_retirement_form_one_registration(self):
        source = """
typedef void (*notify)(void *context);
typedef void (*release)(void *context);
typedef struct registration {
  notify callback;
  void *context BIND("kind=context");
  release release;
} registration BIND("kind=callback_registration;release=release");
mln_status install(const registration *value, mln_diagnostic *out_diagnostic);
"""
        model = bind(self.parse(source), require_complete=True)
        plan = model.operations[0].registrations[0]
        self.assertEqual(
            (plan.callbacks, plan.user_data, plan.release),
            (("callback",), "context", "release"),
        )
        with self.assertRaisesRegex(
            ModelError, "registration release requires a descriptor field"
        ):
            bind(
                self.parse(source.replace("release=release", "release=missing")),
                require_complete=True,
            )
        with self.assertRaisesRegex(ModelError, "release requires a callback typedef"):
            bind(
                self.parse(source.replace("  release release;", "  unsigned release;")),
                require_complete=True,
            )
        with self.assertRaisesRegex(ModelError, "void callback contains its failure"):
            bind(
                self.parse(
                    source.replace(
                        "(*notify)(void *context);",
                        '(*notify)(void *context) BIND("failure=0");',
                    )
                ),
                require_complete=True,
            )

    def test_attachment_keeps_immediate_owner_separate_from_completion(self):
        source = """
typedef unsigned long root BIND("kind=handle;release=close_root");
typedef unsigned long child BIND("kind=handle;release=close_child;parent=root;abandon=abandon_child");
mln_status close_root(root value, mln_diagnostic *out_diagnostic);
mln_status close_child(child value, mln_diagnostic *out_diagnostic);
mln_status abandon_child(child value, mln_diagnostic *out_diagnostic);
BIND("execution=lifecycle") mln_status attach(
  root parent, child *owner BIND("direction=out"),
  const mln_completion *done, mln_diagnostic *out_diagnostic);
"""
        model = bind(self.parse(source), require_complete=True)
        operation = model.operations_by_name["attach"]
        assert operation.completion is not None
        owner = operation.completion.immediate_owners[0]
        self.assertEqual((owner.parameter, owner.parent_parameter), ("owner", "parent"))
        self.assertEqual(owner.handle.finalize, ("abandon_child", "close_child"))
        self.assertEqual(operation.owned_outputs, (owner,))
        with self.assertRaisesRegex(ModelError, "requires its parent handle input"):
            bind(
                self.parse(
                    source.replace(
                        "root parent, child *owner", "unsigned parent, child *owner"
                    )
                ),
                require_complete=True,
            )
        # A handle that only the completion carried would leak if the host
        # dropped the completion, so it comes back through the out parameter.
        with self.assertRaisesRegex(ModelError, "a completion cannot deliver a handle"):
            bind(
                self.parse(
                    source.replace(
                        'BIND("execution=lifecycle")',
                        'BIND("execution=lifecycle;result=child")',
                    ).replace('child *owner BIND("direction=out"),', "")
                ),
                require_complete=True,
            )

    def test_consuming_release_preserves_required_synchronization(self):
        model = bind(
            self.parse("""
typedef unsigned long frame BIND("kind=handle;release=release_frame");
typedef struct sync { unsigned kind; unsigned long object; } sync;
mln_status release_frame(
  frame value, const sync *consumer, mln_diagnostic *out_diagnostic);
"""),
            require_complete=True,
        )
        operation = model.operations[0]
        self.assertEqual((operation.receiver, operation.consumes), ("value", "success"))
        self.assertEqual(model.handles["frame"].release_inputs, ("consumer",))
        self.assertIsNone(model.handles["frame"].dispose)

    def test_a_discard_cleans_up_a_dropped_owner_but_no_scope(self):
        source = """
typedef unsigned long frame BIND("kind=handle;release=release_frame;discard=discard_frame");
typedef struct sync { unsigned kind; unsigned long object; } sync;
mln_status release_frame(
  frame value, const sync *consumer BIND("nullable=true"),
  mln_diagnostic *out_diagnostic);
mln_status discard_frame(frame value, mln_diagnostic *out_diagnostic);
"""
        model = bind(self.parse(source), require_complete=True)
        handle = model.handles["frame"]
        self.assertEqual(handle.cleanup, "discard_frame")
        self.assertEqual(handle.finalize, ("discard_frame",))
        # A scope cannot supply the consumer sync that a release needs.
        self.assertIsNone(handle.scoped_release)
        discard = model.operations_by_name["discard_frame"]
        self.assertEqual((discard.role, discard.consumes), ("public", "success"))
        with self.assertRaisesRegex(
            ModelError, "a handle takes a dispose or a discard, not both"
        ):
            bind(
                self.parse(source.replace("discard=", "dispose=discard_frame;discard="))
            )

    def test_resource_request_protocol_uses_verified_relationships(self):
        source = """
typedef unsigned long request BIND("kind=handle;release=release_request");
typedef void (*cancel)(void *context);
typedef void (*release_cancel)(void *context);
typedef struct cancel_handler { unsigned size; cancel callback; void *context BIND("kind=context"); release_cancel release; } cancel_handler BIND("kind=callback_registration;release=release");
typedef void (*provider)(void *context, request ticket BIND("ownership=owned"));
BIND("consumes=success") mln_status answer(request value, unsigned response, mln_diagnostic *out_diagnostic);
mln_status is_cancelled(request value, bool *result BIND("direction=out"), mln_diagnostic *out_diagnostic);
BIND("accepted_unless=cancelled") mln_status on_cancel(
  request value, const cancel_handler *handler, bool *cancelled BIND("direction=out"), mln_diagnostic *out_diagnostic);
void release_request(request value);
"""
        model = bind(self.parse(source), require_complete=True)
        (ticket,) = model.callbacks["provider"].arguments
        self.assertEqual(
            (ticket.value.kind, ticket.value.ownership), ("handle", "owned")
        )
        # An answer consumes the request it answers, as its release does, but
        # is no release: it keeps its name, and a repeated one reaches native.
        answer = model.operations_by_name["answer"]
        self.assertEqual((answer.consumes, answer.release), ("success", False))
        self.assertTrue(model.operations_by_name["release_request"].release)
        (registration,) = model.operations_by_name["on_cancel"].registrations
        self.assertEqual(
            (registration.release, registration.accepted_unless),
            ("release", "cancelled"),
        )
        for before, after, error in (
            ("consumes=success", "consumes=always", "consumes"),
            ("accepted_unless=cancelled", "accepted_unless=value", "boolean output"),
            (
                "const cancel_handler *handler",
                "cancel callback",
                "one registration parameter",
            ),
        ):
            with self.subTest(after=after), self.assertRaisesRegex(ModelError, error):
                bind(self.parse(source.replace(before, after)), require_complete=True)

    def test_each_operation_takes_the_first_shape_that_it_matches(self):
        source = """
typedef unsigned long root BIND("kind=handle;release=release_root");
typedef unsigned long child BIND("kind=handle;release=close_child;parent=root;abandon=abandon_child");
typedef unsigned long request BIND("kind=handle;release=release_request");
BIND("execution=lifecycle") mln_status release_root(
  root value, const mln_completion *done, mln_diagnostic *out_diagnostic);
mln_status close_child(child value, mln_diagnostic *out_diagnostic);
mln_status abandon_child(child value, mln_diagnostic *out_diagnostic);
void release_request(request value);
BIND("consumes=success") mln_status answer(request value, unsigned response, mln_diagnostic *out_diagnostic);
BIND("blocks=thread_start") mln_status start(root *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
BIND("execution=lifecycle") mln_status attach(
  root parent, child *owner BIND("direction=out"),
  const mln_completion *done, mln_diagnostic *out_diagnostic);
BIND("execution=command") mln_status refresh(root value, const mln_completion *done, mln_diagnostic *out_diagnostic);
BIND("execution=query;result=double") mln_status zoom(root value, const mln_completion *done, mln_diagnostic *out_diagnostic);
BIND("execution=operation") mln_status reload(root value, const mln_completion *done, mln_diagnostic *out_diagnostic);
mln_status jump(root value, double zoom, mln_diagnostic *out_diagnostic);
"""
        model = bind(
            parse(source, groups=("borrowed_view", "strided_records")),
            require_complete=True,
        )
        operations = (*model.operations, *model.runtime_operations)
        shapes = {operation.name: operation.shape for operation in operations}
        # A drain and a thread start also create owners, a lifecycle release
        # also completes, and a discard is also its handle's cleanup.
        self.assertEqual(
            shapes,
            {
                "mln_frame_discard": "discard",
                "mln_frame_release": "release",
                "close_child": "release",
                "release_request": "release",
                "mln_probe_ledger_release": "release",
                "mln_probe_reading_batch_release": "release",
                "release_root": "release_async",
                "answer": "answer",
                "start": "thread_start",
                "attach": "attach",
                "refresh": "command",
                "zoom": "query",
                "reload": "run",
                "mln_frame_get_texture": "view",
                "mln_probe_ledger_drain": "drain",
                "mln_frame_acquire": "owned",
                "mln_probe_ledger_open": "owned",
                "abandon_child": "call",
                "jump": "call",
            },
        )
        self.assertEqual(set(shapes.values()), set(get_args(OperationShape)))
        self.assertEqual(
            [operation.name for operation in operations if operation.answers],
            ["answer"],
        )

    def test_counted_output_preserves_array_storage(self):
        model = bind(
            self.parse("""
mln_status copy_values(
  const double *values BIND("length=count"), unsigned count,
  double *out BIND("direction=out;length=count"), mln_diagnostic *out_diagnostic);
"""),
            require_complete=True,
        )
        output = model.operations[0].outputs[0]
        self.assertEqual((output.value.kind, output.value.length), ("array", "count"))
        self.assertEqual(output.sized_by, "values")

    # A binding decodes caller-sized elements after the call from zeroed
    # storage, so an element that owns or borrows is refused.
    def test_counted_output_elements_must_be_plain_values(self):
        for element, declarations in (
            (
                "owner",
                (
                    'typedef unsigned long owner BIND("kind=handle;release=release_owner");\n'
                    "void release_owner(owner value);"
                ),
            ),
            (
                "item",
                "typedef struct item { mln_buffer_view message; } item;",
            ),
        ):
            source = f"""
{declarations}
mln_status copy_values(
  const double *values BIND("length=count"), unsigned count,
  {element} *out BIND("direction=out;length=count"), mln_diagnostic *out_diagnostic);
"""
            with (
                self.subTest(declarations=declarations),
                self.assertRaisesRegex(ModelError, "caller-sized output element"),
            ):
                bind(self.parse(source), require_complete=True)

    def test_an_absence_status_names_the_one_output_it_reports_absent(self):
        model = bind(
            parse(groups=("absent_handle", "absent_value")), require_complete=True
        )
        self.assertEqual(
            {
                name: (
                    plan.absence.status,
                    plan.absence.value,
                    plan.absence.output.name,
                )
                for name, plan in model.operations_by_name.items()
                if plan.absence
            },
            {
                "mln_probe_take_parcel": ("MLN_STATUS_NOT_READY", -9, "out_parcel"),
                "mln_probe_read_level": ("MLN_STATUS_NOT_READY", -9, "out_level"),
            },
        )
        one = 'double *value BIND("direction=out"), mln_diagnostic *out_diagnostic'
        two = 'double *first BIND("direction=out"), ' + one
        later = "mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic"
        for annotation, parameters, error in (
            ("absent_on=MLN_STATUS_NOT_READY", two, "exactly one output"),
            (
                "execution=query;result=double;absent_on=MLN_STATUS_NOT_READY",
                later,
                "without a completion",
            ),
            ("absent_on=MLN_STATUS_OK", one, "failure enumerator"),
            # A drain is absent on MLN_STATUS_NOT_READY by convention.
            (
                "execution=event_batch;absent_on=MLN_STATUS_NOT_READY",
                one,
                "absent_on=MLN_STATUS_NOT_READY restates the default",
            ),
        ):
            source = f'BIND("{annotation}") mln_status read({parameters});'
            with self.subTest(source=source), self.assertRaisesRegex(ModelError, error):
                bind(self.parse(source), require_complete=True)
        # An absent call publishes nothing, so no binding may root a
        # registration that it passed.
        callbacks = """
typedef void (*notify)(void *state);
typedef void (*release)(void *state);
typedef struct signals { notify signal; void *state BIND("kind=context"); release retire; } signals BIND("kind=callback_registration;release=retire");
"""
        source = (
            callbacks
            + 'BIND("absent_on=MLN_STATUS_NOT_READY") mln_status read(const signals *events, '
            + one
            + ");"
        )
        with self.assertRaisesRegex(ModelError, "without registrations"):
            bind(self.parse(source), require_complete=True)

    def test_delivered_callbacks_name_the_payload_that_copies_them(self):
        text = protocol_header(groups=("delivery", "owned_argument"))
        model = bind(parse_sources({"api.h": text}), require_complete=True)
        self.assertEqual(
            model.callbacks["mln_notice_callback"].delivery,
            DeliveryPlan("MLN_DELIVERY_TYPE_NOTICE", "notice", "mln_notice_delivery"),
        )
        self.assertEqual(
            model.callbacks["mln_query_callback"].delivery.record,
            "mln_query_delivery",
        )
        # What queue mode answers in place of a callback with a result; a void
        # callback needs no answer.
        self.assertEqual(model.callbacks["mln_notice_callback"].queue_result, "1")
        self.assertIsNone(model.callbacks["mln_query_callback"].queue_result)
        registration = model.operations_by_name[
            "mln_notice_set_callback"
        ].registrations[0]
        self.assertEqual(registration.queue, "queue")
        queue = next(
            field
            for field in model.values["mln_notice_handler"].fields
            if field.name == "queue"
        )
        self.assertFalse(queue.public)
        # A runtime drives the protocol, so it stays out of every public API.
        self.assertIn(
            "mln_delivery_queue_drain",
            {plan.name for plan in model.runtime_operations},
        )
        self.assertNotIn("mln_delivery_queue_drain", model.operations_by_name)
        self.assertNotIn("mln_delivery_type", model.public_values)
        self.assertNotIn("mln_delivery_queue", model.public_handles)
        stray = (
            "typedef struct stray { mln_delivery_queue queue "
            'BIND("kind=delivery_queue"); } stray;\n'
        )
        queue_member = (
            '  mln_delivery_queue queue BIND("kind=delivery_queue");\n'
            "} mln_notice_handler"
        )
        for before, after, error in (
            (
                "delivery=MLN_DELIVERY_TYPE_NOTICE",
                "delivery=MLN_DELIVERY_TYPE_UNKNOWN",
                "unknown constant",
            ),
            (
                ";queue_result=1",
                "",
                "declares its queue_result",
            ),
            (
                "queue_result=1",
                "queue_result=4294967296",
                "queue_result must be an integer",
            ),
            (
                "  int code;\n  const char* text",
                '  const char* text BIND("lifetime=owner");\n  int code',
                "must mirror",
            ),
            (
                queue_member,
                queue_member.replace(
                    "mln_delivery_queue queue", "unsigned long long queue"
                ),
                "requires mln_delivery_queue",
            ),
            (
                queue_member,
                '  mln_delivery_queue spare BIND("kind=delivery_queue");\n'
                + queue_member,
                "at most one delivery queue",
            ),
            (
                "typedef struct mln_notice_delivery {",
                stray + "typedef struct mln_notice_delivery {",
                "requires a callback registration",
            ),
        ):
            with self.subTest(after=after), self.assertRaisesRegex(ModelError, error):
                self.assertIn(before, text)
                bind(
                    parse_sources({"api.h": text.replace(before, after, 1)}),
                    require_complete=True,
                )

    def test_a_queue_registration_without_deliveries_only_dispatches(self):
        # A callback without a delivery has no record to drain, so its
        # registration names a queue only for dispatch. A registration whose
        # other callback drains must give every callback a delivery.
        source = """
typedef void (*mln_ping_callback)(void* context);
typedef struct mln_ping_handler {
  unsigned int size;
  mln_ping_callback callback;
  void* context BIND("kind=context");
  mln_notice_release release;
  mln_delivery_queue queue BIND("kind=delivery_queue");
} mln_ping_handler BIND("kind=callback_registration;release=release");
mln_status mln_ping_set_callback(
  const mln_ping_handler* handler, mln_diagnostic* out_diagnostic
);
"""
        text = protocol_header(source, groups=("delivery",))
        model = bind(parse_sources({"api.h": text}), require_complete=True)
        self.assertEqual(
            model.operations_by_name["mln_ping_set_callback"].registrations[0].queue,
            "queue",
        )
        mixed = text.replace(
            "  mln_ping_callback callback;\n",
            "  mln_ping_callback callback;\n  mln_notice_callback notice;\n",
        )
        with self.assertRaisesRegex(ModelError, "callback callback requires delivery="):
            bind(parse_sources({"api.h": mixed}), require_complete=True)

    def test_synchronous_callbacks_run_on_their_thread(self):
        source = """
BIND("synchronous=true") typedef void (*gate)(void *context);
typedef void (*notify)(void *context);
typedef void (*release)(void *context);
typedef unsigned long long mln_delivery_queue BIND("kind=handle;release=mln_delivery_queue_release;dispose=mln_delivery_queue_release");
void mln_delivery_queue_release(mln_delivery_queue queue);
typedef struct gates { gate enter BIND("nullable=true"); gate leave BIND("nullable=true"); void *context BIND("kind=context"); release retire; } gates BIND("kind=callback_registration;release=retire");
typedef struct signals { notify signal BIND("nullable=true"); void *context BIND("kind=context"); release retire; mln_delivery_queue queue BIND("kind=delivery_queue"); } signals BIND("kind=callback_registration;release=retire");
typedef struct settings { unsigned size; signals wake; gates guard; } settings;
mln_status attach(const settings *options, mln_diagnostic *out_diagnostic);
"""
        api = self.parse(source)
        model = bind(api, require_complete=True)
        self.assertTrue(model.callbacks["gate"].synchronous)
        self.assertFalse(model.callbacks["notify"].synchronous)
        # Dart takes the synchronous registration only as a native
        # registration, so it still lowers the call.
        self.assertIn("attach", dart.lower(api).generated)
        for before, after, error in (
            (
                "signals wake; gates guard; } settings;",
                'signals wake; gates guard; } settings BIND("synchronous=true");',
                "synchronous requires a callback",
            ),
            (
                'BIND("synchronous=true") typedef void (*gate)(void *context);',
                'BIND("synchronous=true") typedef unsigned (*gate)(void *context) BIND("failure=0;delivery=MLN_DELIVERY_TYPE_NOTICE");',
                "cannot be delivered through a queue",
            ),
        ):
            with self.subTest(after=after), self.assertRaisesRegex(ModelError, error):
                bind(
                    self.parse(source.replace(before, after, 1)), require_complete=True
                )

    def test_nullable_completion_array_has_optional_container(self):
        model = bind(
            self.parse("""
typedef struct entry { double value; } entry;
BIND("execution=query;result=entry;shape=array;nullable=true")
mln_status query(const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""),
            require_complete=True,
        )
        result = model.operations[0].result
        assert result is not None and result.element is not None
        self.assertTrue(result.nullable)
        self.assertFalse(result.element.nullable)

    def test_receiver_registration_transfers_its_root_unless_declined(self):
        model = bind(parse(groups=("declinable_registration",)), require_complete=True)
        operation = model.operations_by_name["mln_ticket_on_cancel"]
        self.assertEqual(operation.receiver, "ticket")
        (registration,) = operation.registrations
        self.assertEqual(
            (registration.release, registration.accepted_unless),
            ("release", "cancelled"),
        )
        # The cancel callback borrows its owner, which the host never
        # receives because the registration already retains that owner.
        callback = model.callbacks["mln_ticket_cancel"]
        self.assertEqual(
            (callback.borrowed_owner,),
            ("ticket",),
        )
        self.assertEqual(callback.arguments, ())

    def test_disposal_support_requires_a_handle_consumer(self):
        source = """
typedef unsigned long owner BIND("kind=handle;release=close_owner;dispose=discard_owner");
mln_status close_owner(owner value, mln_diagnostic *out_diagnostic);
mln_status discard_owner(owner value, mln_diagnostic *out_diagnostic);
"""
        model = bind(self.parse(source), require_complete=True)
        self.assertEqual(
            model.operations_by_name["discard_owner"].support,
            DisposeSupport(model.handles["owner"]),
        )
        self.assertEqual(model.operations_by_name["close_owner"].role, "public")


if __name__ == "__main__":
    unittest.main()
