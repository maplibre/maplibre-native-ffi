"""Semantic mutation tests shared by every static language backend."""

import unittest

from support import parse

from tools.bindgen import default_cases
from tools.bindgen.model import ModelError
from tools.bindgen.semantic import DefaultSupport, DisposeSupport, FieldInitial, bind


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

    def test_public_enums_and_retired_handle_access(self):
        source = """
typedef enum event_code : unsigned { CAMERA_CHANGED = 3 } event_code;
typedef unsigned long owner BIND("kind=handle;release=release_owner");
void release_owner(owner value);
mln_status await_owner(owner value BIND("handle_access=issued"), mln_diagnostic *out_diagnostic);
"""
        model = bind(self.parse(source), require_complete=True)
        self.assertEqual(
            model.values["event_code"].enum_values, (("CAMERA_CHANGED", 3),)
        )
        self.assertEqual(
            model.operations_by_name["await_owner"].receiver_access, "issued"
        )
        with self.assertRaisesRegex(ModelError, "nonconsuming input handle"):
            bind(
                self.parse(
                    source.replace(
                        "await_owner(owner value", "await_owner(unsigned value"
                    )
                ),
                require_complete=True,
            )

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
        with self.assertRaisesRegex(ModelError, "distinct values"):
            bind(
                self.parse(source.replace("variant=NUMBER", "variant=TEXT")),
                require_complete=True,
            )

    def test_field_defaults_resolve_to_typed_initials(self):
        bound = bind(parse(groups=("defaults",)), require_complete=True)
        settings = bound.values["mln_probe_settings"]
        initials = {
            field.name: field.initial
            for plan in (settings, bound.values["mln_probe_extent"])
            for field in plan.fields
        }
        self.assertEqual(
            initials,
            {
                "size": None,
                "extent": None,
                "width": FieldInitial("256", 256),
                "scale": FieldInitial("1.5", 1.5),
                "mode": FieldInitial(
                    "MLN_PROBE_MODE_SECOND", 2, "MLN_PROBE_MODE_SECOND", "second"
                ),
                "flags": FieldInitial(
                    "MLN_PROBE_FLAG_ALL", 3, "MLN_PROBE_FLAG_ALL", "all"
                ),
                "heading": FieldInitial(
                    "MLN_PROBE_FLAG_SOUTH", 2, "MLN_PROBE_FLAG_SOUTH", "south"
                ),
                "ratio": FieldInitial("0.25", 0.25),
                "offset": FieldInitial("-3", -3),
                "enabled": FieldInitial("true", True),
                "count": None,
            },
        )
        # The generated C case checks every field against the same values.
        self.assertEqual(
            [
                (item.path, item.form, item.expected)
                for item in default_cases.record_expectations(
                    settings, "value", "settings"
                )
            ],
            [
                ("settings.size", "unsigned", "sizeof(mln_probe_settings)"),
                ("settings.extent.width", "unsigned", "256"),
                ("settings.extent.scale", "float", "1.5"),
                ("settings.mode", "unsigned", "MLN_PROBE_MODE_SECOND"),
                ("settings.flags", "unsigned", "MLN_PROBE_FLAG_ALL"),
                ("settings.heading", "unsigned", "MLN_PROBE_FLAG_SOUTH"),
                ("settings.ratio", "float", "0.25"),
                ("settings.offset", "signed", "-3"),
                ("settings.enabled", "unsigned", "true"),
                ("settings.count", "signed", "0"),
            ],
        )

    def test_field_defaults_state_only_nonzero_values_that_a_default_returns(self):
        source = """
typedef enum mode : unsigned { MODE_OFF = 0, MODE_ON = 1 } mode;
typedef struct extent { unsigned width; } extent;
typedef struct turn { double w; } turn;
typedef struct settings {
  unsigned size;
  bool has_zoom;
  bool has_turn;
  extent area;
  turn orientation BIND("mask=has_turn");
  unsigned level;
  unsigned mode BIND("enum=mode");
  double zoom BIND("mask=has_zoom");
  unsigned reserved BIND("kind=reserved");
} settings;
settings settings_default(void);
typedef struct loose { unsigned width; } loose;
mln_status write_loose(const loose *value, mln_diagnostic *out_diagnostic);
"""
        self.assertEqual(
            bind(self.parse(source), require_complete=True)
            .values["settings"]
            .fields[5]
            .initial,
            None,
        )
        for before, after, message in (
            ("unsigned level;", 'unsigned level BIND("default=0");', "restates zero"),
            (
                'BIND("enum=mode")',
                'BIND("enum=mode;default=MODE_OFF")',
                "restates zero",
            ),
            ('BIND("enum=mode")', 'BIND("enum=mode;default=ON")', "names no mode"),
            ("unsigned level;", 'unsigned level BIND("default=1.0");', "decimal"),
            (
                'BIND("mask=has_zoom")',
                'BIND("mask=has_zoom;default=1.0")',
                "plain value",
            ),
            ("extent area;", 'extent area BIND("default=1");', "plain value"),
            (
                'BIND("kind=reserved")',
                'BIND("kind=reserved;default=1")',
                "is fixed",
            ),
            (
                "typedef struct loose { unsigned width; }",
                'typedef struct loose { unsigned width BIND("default=1"); }',
                "default function returns",
            ),
            # The default leaves an optional member absent, so no case checks
            # the defaults of a record reached only through one.
            (
                "typedef struct turn { double w; }",
                'typedef struct turn { double w BIND("default=1.0"); }',
                "default function returns",
            ),
        ):
            with (
                self.subTest(after=after),
                self.assertRaisesRegex(ModelError, message),
            ):
                bind(self.parse(source.replace(before, after)), require_complete=True)
        # A record nested by value in a default may state its own defaults.
        nested = source.replace(
            "typedef struct extent { unsigned width; }",
            'typedef struct extent { unsigned width BIND("default=256"); }',
        )
        self.assertEqual(
            bind(self.parse(nested), require_complete=True)
            .values["extent"]
            .fields[0]
            .initial,
            FieldInitial("256", 256),
        )

    def test_mask_group_preserves_joint_presence(self):
        source = """
typedef enum BIND("kind=bitmask") fields : unsigned { CENTER = 1, ZOOM = 2 } fields;
typedef struct options {
  unsigned fields BIND("enum=fields");
  double latitude BIND("mask=fields;bit=CENTER");
  double longitude BIND("mask=fields;bit=CENTER");
  double zoom BIND("mask=fields;bit=ZOOM");
} options;
mln_status write_options(const options *value, mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        value = api.operations[0].inputs[0].value.element
        self.assertEqual(value.presence_groups[0].fields, ("latitude", "longitude"))
        self.assertEqual(value.fields[0].role, "presence_mask")
        self.assertEqual(value.fields[0].value.enum_kind, "bitmask")
        with self.assertRaisesRegex(ModelError, "both mask and bit"):
            bind(
                self.parse(source.replace("mask=fields;bit=ZOOM", "mask=fields")),
                require_complete=True,
            )

    def test_boolean_presence_preserves_optional_groups(self):
        source = """
typedef struct range_value {
  bool has_range;
  unsigned start BIND("mask=has_range");
  unsigned end BIND("mask=has_range");
} range_value;
mln_status write_range(const range_value *value, mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        value = api.operations[0].inputs[0].value.element
        group = value.presence_groups[0]
        self.assertEqual(
            (group.mask, group.bit, group.fields), ("has_range", None, ("start", "end"))
        )
        self.assertEqual(value.fields[0].role, "presence_mask")
        with self.assertRaisesRegex(ModelError, "boolean mask"):
            bind(
                self.parse(source.replace("bool has_range", "unsigned has_range")),
                require_complete=True,
            )

    def test_copied_values_hold_registrations_only_in_defaults(self):
        source = """
typedef void (*notify)(void *state);
typedef void (*release)(void *state);
typedef struct signals { notify signal; void *state BIND("kind=context"); release retire; } signals BIND("kind=callback_registration;release=retire");
typedef struct hook { notify fire; void *state BIND("kind=context"); release retire; double scale; } hook BIND("kind=callback_registration;release=retire");
typedef struct extent { double width; } extent;
typedef struct settings { extent extent; signals wake; } settings;
typedef struct request { double zoom; } request;
typedef struct reading { double zoom; } reading;
settings settings_default(void);
hook hook_default(void);
mln_status configure(const settings *options, const hook *events, const request *input, reading *out_reading BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""
        model = bind(self.parse(source), require_complete=True)
        self.assertEqual(
            model.defaults.keys() & {"settings", "hook"}, {"settings", "hook"}
        )
        # A default's registration field is left unset rather than copied, and
        # a registration's own default copies its other fields.
        records = {"signals", "hook", "extent", "settings", "request", "reading"}
        self.assertEqual(
            model.returned & records, {"hook", "extent", "settings", "reading"}
        )
        for rejected in (
            'mln_status inspect(settings *out_settings BIND("direction=out"), mln_diagnostic *out_diagnostic);',
            'mln_status refresh(settings *options BIND("direction=inout"), mln_diagnostic *out_diagnostic);',
            (
                'BIND("execution=query;result=settings")\n'
                "mln_status mln_map_settings(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);"
            ),
            (
                "typedef void (*observe)(void *state, settings current);\n"
                'mln_status watch(observe callback, void *state BIND("kind=context"), mln_diagnostic *out_diagnostic);'
            ),
        ):
            with (
                self.subTest(rejected=rejected),
                self.assertRaisesRegex(
                    ModelError, "native cannot return a callback registration"
                ),
            ):
                bind(self.parse(source + rejected), require_complete=True)

    def test_defaults_hold_registrations_only_in_fields_without_presence(self):
        # A copy would have to keep an optional registration's presence, so a
        # default holds a registration only as a field without presence.
        source = """
typedef void (*notify)(void *state);
typedef void (*release)(void *state);
typedef struct signals { notify signal; void *state BIND("kind=context"); release retire; } signals BIND("kind=callback_registration;release=retire");
typedef struct settings { bool has_wake; FIELD } settings;
settings settings_default(void);
"""
        for field in (
            'signals wake BIND("mask=has_wake");',
            'const signals *wake BIND("nullable=true");',
        ):
            with (
                self.subTest(field=field),
                self.assertRaisesRegex(
                    ModelError,
                    "settings_default result.wake: a record default holds a "
                    "callback registration only in a field without presence",
                ),
            ):
                bind(self.parse(source.replace("FIELD", field)), require_complete=True)

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

    def test_native_adapter_has_verified_callback_signature_and_protocol(self):
        source = """
typedef struct reply { void *context BIND("kind=context"); } reply BIND("kind=callback_response");
typedef struct rules { int value; } rules;
typedef void (*answer)(void *state, reply *out BIND("direction=out")) BIND("reentry=protocol;reentry_owner=out;reentry_calls=reply_set");
mln_status reply_set(reply *response, int value, mln_diagnostic *out_diagnostic);
BIND("callback_adapter=answer;context_type=rules") void adapter(void *state BIND("kind=context"), reply *out BIND("direction=out"));
"""
        api = bind(self.parse(source), require_complete=True)
        policy = api.callbacks["answer"].reentry_policy
        self.assertEqual(
            (policy.owner_parameter, policy.owner_type, policy.operations),
            ("out", "reply", ("reply_set",)),
        )
        with self.assertRaisesRegex(
            ModelError, "immediate operation on the declared owner"
        ):
            bind(
                self.parse(
                    source.replace("reentry_calls=reply_set", "reentry_calls=adapter")
                ),
                require_complete=True,
            )
        adapter = api.callback_adapters[0]
        self.assertEqual((adapter.callback, adapter.context), ("answer", "rules"))
        # The adapter answers through the response its callback writes.
        self.assertEqual(
            [plan.name for plan in api.adapter_operations(adapter)], ["reply_set"]
        )
        self.assertIn("rules", api.values)
        with self.assertRaisesRegex(ModelError, "signature differs"):
            bind(
                self.parse(source.replace("void adapter(", "int adapter(")),
                require_complete=True,
            )

    def test_strided_items_resolve_their_message_arena(self):
        source = """
typedef struct item { unsigned int offset; unsigned int count; } item;
typedef struct batch {
 unsigned int stride;
 const item *items BIND("length=count;stride=stride;item_name=message;item_buffer=bytes;item_buffer_size=byte_count;item_offset=offset;item_length=count;item_encoding=utf8");
 unsigned int count;
 const char *bytes BIND("length=byte_count;encoding=bytes");
 unsigned int byte_count;
} batch;
mln_status capture(batch *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        value = next(f.value for f in api.values["batch"].fields if f.name == "items")
        self.assertEqual(value.stride, "stride")
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
        self.assertEqual(roles["bytes"], "arena")
        self.assertEqual(roles["byte_count"], "count")
        self.assertEqual(
            (value.item_buffer.field, value.item_buffer.data, value.item_buffer.offset),
            ("message", "bytes", "offset"),
        )
        with self.assertRaisesRegex(ModelError, "stride requires"):
            bind(
                self.parse(source.replace("stride=stride", "stride=missing")),
                require_complete=True,
            )
        with self.assertRaisesRegex(ModelError, "absent storage"):
            bind(
                self.parse(source.replace("item_offset=offset", "item_offset=missing")),
                require_complete=True,
            )

    def test_callback_reentry_policy_is_explicit(self):
        source = """
typedef void (*logger)(void *state) BIND("reentry=forbid");
"""
        api = bind(self.parse(source), require_complete=True)
        self.assertEqual(api.callbacks["logger"].reentry, "forbid")
        with self.assertRaisesRegex(ModelError, "reentry"):
            bind(
                self.parse(source.replace("reentry=forbid", "reentry=unknown")),
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
mln_status reply_set(reply *response BIND("direction=inout"), int value, mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        response = api.values["reply"].response
        self.assertEqual(response.context, "context")
        self.assertEqual(response.callbacks, ("answer",))
        self.assertEqual(response.methods, ("reply_set",))
        self.assertEqual(response.lifetime, "callback")
        self.assertEqual(
            api.operations_by_name["reply_set"].scoped_receiver, "response"
        )
        with self.assertRaisesRegex(ModelError, "one context"):
            bind(
                self.parse(source.replace(' BIND("kind=context")', "")),
                require_complete=True,
            )

    def test_borrowed_output_view_tracks_owner_and_parent_invalidation(self):
        source = """
typedef unsigned long long parent BIND("kind=handle;release=parent_close;dispose=parent_close;abandon=parent_abandon");
typedef unsigned long long child BIND("kind=handle;release=child_close;dispose=child_close;parent=parent;view_begin=child_begin;view_end=child_end");
typedef struct view { void *texture; } view;
mln_status parent_close(parent value, mln_diagnostic *out_diagnostic);
mln_status parent_abandon(parent value, mln_diagnostic *out_diagnostic);
mln_status child_close(child value, mln_diagnostic *out_diagnostic);
mln_status child_begin(child value, void **out_token BIND("direction=out;kind=context"), mln_diagnostic *out_diagnostic);
void child_end(void *token BIND("kind=context"));
BIND("view_owner=owner") mln_status child_get_view(child owner, view *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""
        api = bind(self.parse(source), require_complete=True)
        view = api.operations_by_name["child_get_view"].view
        self.assertEqual(view.owner_parameter, "owner")
        self.assertEqual(view.owner.native, "child")
        self.assertEqual(
            view.invalidated_by, ("child_close", "parent_close", "parent_abandon")
        )
        self.assertEqual(
            (view.output.name, view.begin, view.end, view.stem),
            ("out", "child_begin", "child_end", "view"),
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
  unsigned *out BIND("direction=out;enum=flags"), mln_diagnostic *out_diagnostic);
"""),
            require_complete=True,
        )
        output = model.operations[0].outputs[0].value
        self.assertEqual(output.kind, "reference")
        self.assertEqual(output.element.kind, "enum")

    def test_counted_output_preserves_array_storage(self):
        model = bind(
            self.parse("""
mln_status copy_values(
  double *out BIND("direction=out;length=count"), unsigned count, mln_diagnostic *out_diagnostic);
"""),
            require_complete=True,
        )
        output = model.operations[0].outputs[0].value
        self.assertEqual((output.kind, output.length), ("array", "count"))

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
        with self.assertRaisesRegex(ModelError, "void context pointer"):
            bind(
                self.parse(
                    source.replace(
                        '  void *context BIND("kind=context");', "  unsigned context;"
                    ).replace("release=release", "release=release;user_data=context")
                ),
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

    def test_default_support_is_derived_from_a_checked_consumer(self):
        source = """
typedef struct options { double zoom; } options;
options make_options(void);
mln_status set_options(const options *value, mln_diagnostic *out_diagnostic);
"""
        model = bind(self.parse(source), require_complete=True)
        constructor = model.operations_by_name["make_options"]
        self.assertEqual(constructor.support, DefaultSupport("options"))
        self.assertEqual(model.defaults, {"options": constructor})
        self.assertEqual(model.operations_by_name["set_options"].role, "public")
        # A constructor that takes arguments is no default, and naming one fails.
        with_argument = source.replace(
            "make_options(void)", "make_options(int ignored)"
        )
        model = bind(self.parse(with_argument), require_complete=True)
        self.assertEqual(model.operations_by_name["make_options"].role, "public")
        with self.assertRaisesRegex(ModelError, "no-argument constructor"):
            bind(
                self.parse(
                    with_argument.replace(
                        "} options;", '} options BIND("default=make_options");'
                    )
                ),
                require_complete=True,
            )
        # The relation is derived, never declared.
        with self.assertRaisesRegex(ModelError, "unknown metadata key 'support'"):
            bind(
                self.parse(
                    source.replace(
                        "mln_status set_options",
                        'BIND("support=default:options") mln_status set_options',
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
        with self.assertRaisesRegex(ModelError, "receiver must be this handle type"):
            bind(
                self.parse(
                    source.replace(
                        "abandon_child(child value,", "abandon_child(root value,"
                    )
                ),
                require_complete=True,
            )

    def test_consuming_release_preserves_required_synchronization(self):
        model = bind(
            self.parse("""
typedef unsigned long frame BIND("kind=handle;release=release_frame");
typedef struct sync { unsigned kind; unsigned long object; } sync;
mln_status release_frame(
  frame *value BIND("direction=inout;consumes=success"), const sync *consumer, mln_diagnostic *out_diagnostic);
"""),
            require_complete=True,
        )
        operation = model.operations[0]
        self.assertEqual((operation.receiver, operation.consumes), ("value", "success"))
        self.assertEqual(model.handles["frame"].release_inputs, ("consumer",))
        self.assertIsNone(model.handles["frame"].dispose)

    def test_resource_decision_protocol_uses_verified_relationships(self):
        source = """
typedef enum decision : unsigned { DELEGATE = 0, CLAIM = 1 } decision;
typedef unsigned long request BIND("kind=handle;release=release_request");
typedef void (*cancel)(void *context);
typedef void (*release_cancel)(void *context);
typedef unsigned (*provider)(request ticket) BIND("enum=decision;failure=DELEGATE;decision_handle=ticket;decision_accept=CLAIM;decision_pass=DELEGATE;complete=answer;cancelled=is_cancelled;cancel_registration=on_cancel;wait_retired=await_retirement");
mln_status answer(request value, unsigned response, mln_diagnostic *out_diagnostic);
mln_status is_cancelled(request value, bool *result BIND("direction=out"), mln_diagnostic *out_diagnostic);
BIND("registration=callback;release_callback=release;accepted_unless=cancelled") mln_status on_cancel(
  request value, cancel callback, void *context BIND("kind=context"), release_cancel release, bool *cancelled BIND("direction=out"), mln_diagnostic *out_diagnostic);
void release_request(request value);
mln_status await_retirement(request value BIND("handle_access=issued"), mln_diagnostic *out_diagnostic);
"""
        model = bind(self.parse(source), require_complete=True)
        decision = model.callbacks["provider"].decision
        assert decision is not None
        self.assertEqual(
            (decision.parameter, decision.accept, decision.complete),
            ("ticket", "CLAIM", "answer"),
        )
        self.assertEqual(decision.wait_retired, "await_retirement")
        self.assertEqual(decision.handle.release_consumes, "always")
        registration = model.operations_by_name["on_cancel"].direct_registrations[0]
        self.assertEqual(
            (registration.release_callback, registration.accepted_unless),
            ("release", "cancelled"),
        )
        for before, after, error in (
            (
                "wait_retired=await_retirement",
                "wait_retired=answer",
                "issued decision handle",
            ),
            (' BIND("handle_access=issued")', "", "issued decision handle"),
            ("decision_accept=CLAIM", "decision_accept=DELEGATE", "distinct values"),
            ("complete=answer", "complete=is_missing", "immediate status operation"),
            (
                "decision_handle=ticket",
                "decision_handle=absent",
                "callback handle parameter",
            ),
            ("accepted_unless=cancelled", "accepted_unless=value", "boolean output"),
            (
                "release_callback=release",
                "release_callback=value",
                "void callback taking its context",
            ),
        ):
            with self.subTest(after=after), self.assertRaisesRegex(ModelError, error):
                bind(self.parse(source.replace(before, after)), require_complete=True)

    def test_an_absence_status_names_the_one_output_it_reports_absent(self):
        model = bind(parse(groups=("absence",)), require_complete=True)
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
            ("absent_on=MLN_STATUS_MISSING", one, "failure enumerator"),
            ("absent_on=MLN_STATUS_OK", one, "failure enumerator"),
        ):
            source = f'BIND("{annotation}") mln_status read({parameters});'
            with self.subTest(source=source), self.assertRaisesRegex(ModelError, error):
                bind(self.parse(source), require_complete=True)

    def test_deferred_callbacks_answer_early_and_copy_their_inputs(self):
        source = """
typedef enum decision : unsigned { DELEGATE = 0, CLAIM = 1 } decision;
typedef unsigned long request BIND("kind=handle;release=release_request");
typedef void (*cancel)(void *context);
typedef void (*release_cancel)(void *context);
typedef unsigned (*provider)(void *context, const char *url, request ticket) BIND("enum=decision;failure=DELEGATE;deferred=CLAIM;decision_handle=ticket;decision_accept=CLAIM;decision_pass=DELEGATE;complete=answer;cancelled=is_cancelled;cancel_registration=on_cancel;wait_retired=await_retirement");
typedef unsigned (*logger)(void *context, int code) BIND("failure=0;deferred=1");
mln_status answer(request value, unsigned response, mln_diagnostic *out_diagnostic);
mln_status is_cancelled(request value, bool *result BIND("direction=out"), mln_diagnostic *out_diagnostic);
BIND("registration=callback;release_callback=release;accepted_unless=cancelled") mln_status on_cancel(
  request value, cancel callback, void *context BIND("kind=context"), release_cancel release, bool *cancelled BIND("direction=out"), mln_diagnostic *out_diagnostic);
void release_request(request value);
mln_status await_retirement(request value BIND("handle_access=issued"), mln_diagnostic *out_diagnostic);
"""
        model = bind(self.parse(source), require_complete=True)
        self.assertEqual(model.callbacks["provider"].deferred, "CLAIM")
        self.assertEqual(model.callbacks["logger"].deferred, "1")
        for before, after, error in (
            ("deferred=CLAIM", "deferred=OTHER", "value of decision"),
            ("deferred=CLAIM", "deferred=DELEGATE", "requires the accept value"),
            ("deferred=1", "deferred=-1", "integer the callback result"),
            ("deferred=1", "deferred=yes", "integer the callback result"),
            ("void *context, int code", "int code", "one context parameter"),
            (
                "int code) BIND",
                'int *code BIND("direction=out")) BIND',
                "copyable input",
            ),
            (
                "typedef void (*cancel)(void *context);",
                'typedef void (*cancel)(void *context) BIND("deferred=0");',
                "callback with a result",
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

    def test_owned_output_retains_its_parent_input_rather_than_the_receiver(self):
        model = bind(parse(groups=("owned_output",)), require_complete=True)
        operation = model.operations_by_name["mln_seed_plant"]
        self.assertEqual(operation.receiver, "seed")
        (output,) = operation.owned_outputs
        self.assertEqual(
            (output.parameter, output.parent_parameter, output.handle.parent),
            ("out_tree", "forest", "mln_forest"),
        )

    def test_receiver_registration_transfers_its_root_unless_declined(self):
        model = bind(parse(groups=("direct_registration",)), require_complete=True)
        operation = model.operations_by_name["mln_ticket_on_cancel"]
        self.assertEqual(operation.receiver, "ticket")
        (registration,) = operation.direct_registrations
        self.assertEqual(
            (registration.release_callback, registration.accepted_unless),
            ("release", "cancelled"),
        )

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
        with self.assertRaisesRegex(ModelError, "receiver must be this handle type"):
            bind(
                self.parse(
                    source.replace(
                        "discard_owner(owner value,", "discard_owner(unsigned value,"
                    )
                ),
                require_complete=True,
            )


if __name__ == "__main__":
    unittest.main()
