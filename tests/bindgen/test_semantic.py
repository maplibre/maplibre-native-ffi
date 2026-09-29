"""Semantic mutation tests shared by every static language backend."""

import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from tools.bindgen.frontend import parse_headers
from tools.bindgen.model import ModelError
from tools.bindgen.semantic import bind


class SemanticTests(unittest.TestCase):
    def parse(self, source):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "api.h").write_text(
                '#define BIND(x) __attribute__((annotate("mln:" x)))\n'
                "typedef int mln_status;\n" + source
            )
            return parse_headers(root)

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
BIND("execution=immediate") mln_status read_values(values *out BIND("direction=out"));
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

    def test_adapter_projection_preserves_public_value_contract(self):
        source = """
typedef enum category : unsigned { CATEGORY_A = 1 } category;
typedef struct request { unsigned size BIND("kind=size;default=sizeof"); unsigned kind BIND("enum=category"); const char *url BIND("length=nul;encoding=utf8;ownership=borrowed;nullable=true"); } request;
typedef struct queued { void *context BIND("kind=context"); unsigned kind; const char *url BIND("length=nul;encoding=utf8;ownership=borrowed;nullable=true"); } queued BIND("projection=request");
BIND("execution=immediate") mln_status capture(queued *out BIND("direction=out"));
"""
        model = bind(self.parse(source), require_complete=True)
        projection = model.values["queued"].projection
        self.assertEqual(projection.native, "request")
        self.assertEqual(projection.fields[1].value.kind, "enum")
        self.assertTrue(projection.fields[2].value.nullable)
        with self.assertRaisesRegex(ModelError, "must preserve its source C type"):
            bind(
                self.parse(source.replace("unsigned kind;", "double kind;")),
                require_complete=True,
            )
        with self.assertRaisesRegex(ModelError, "must preserve its source C type"):
            bind(
                self.parse(source.replace("unsigned kind;", "")), require_complete=True
            )

    def test_public_enums_and_retired_handle_access(self):
        source = """
typedef enum event_code : unsigned { CAMERA_CHANGED = 3 } event_code;
typedef unsigned long owner BIND("kind=handle;release=release_owner;parent=none");
BIND("execution=immediate") void release_owner(owner value);
BIND("execution=immediate") mln_status await_owner(owner value BIND("handle_access=issued"));
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
typedef unsigned long owner BIND("kind=handle;release=release_owner;parent=none;view_begin=begin_view;view_end=end_view");
BIND("execution=immediate") void release_owner(owner value);
BIND("execution=immediate") mln_status begin_view(owner value, void **scope BIND("direction=out;kind=context"));
BIND("execution=immediate") void end_view(void *scope BIND("kind=context"));
typedef enum event_tag : int { NONE = 0, NUMBER = -1 } event_tag;
typedef struct event { event_tag tag; union { double number BIND("variant=NUMBER"); } payload BIND("tag=tag;empty_variant=NONE"); } event;
BIND("execution=immediate") mln_status read_event(event *value BIND("direction=out"));
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
typedef struct text { const char *data BIND("length=count;encoding=utf8"); unsigned count; } text;
typedef struct value {
  choice tag;
  union {
    text text BIND("variant=TEXT");
    double number BIND("variant=NUMBER");
  } data BIND("tag=tag");
} value;
BIND("execution=immediate") mln_status read_value(value *out BIND("direction=out"));
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

    def test_mask_group_preserves_joint_presence(self):
        source = """
typedef enum BIND("kind=bitmask") fields : unsigned { CENTER = 1, ZOOM = 2 } fields;
typedef struct options {
  unsigned fields BIND("kind=presence_mask;enum=fields");
  double latitude BIND("mask=fields;bit=CENTER");
  double longitude BIND("mask=fields;bit=CENTER");
  double zoom BIND("mask=fields;bit=ZOOM");
} options;
BIND("execution=immediate") mln_status write_options(const options *value BIND("length=1"));
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
  bool has_range BIND("kind=presence_mask");
  unsigned start BIND("mask=has_range");
  unsigned end BIND("mask=has_range");
} range_value;
BIND("execution=immediate") mln_status write_range(const range_value *value BIND("length=1"));
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

    def test_nested_registration_and_callback_context_are_resolved(self):
        api = bind(
            self.parse("""
typedef void (*notify)(void *state BIND("kind=context;lifetime=owner")) BIND("thread=native;failure=contain");
typedef void (*release)(void *state BIND("kind=context;lifetime=owner")) BIND("thread=native;failure=contain");
typedef struct signals { notify signal; void *state BIND("kind=context"); release retire; } signals BIND("kind=callback_registration;user_data=state;release=retire");
typedef struct settings { signals wake; } settings;
BIND("execution=immediate") mln_status create(const settings *options BIND("length=1"));
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
typedef struct reply { void *context BIND("kind=context;ownership=borrowed"); } reply BIND("kind=callback_response");
typedef struct rules { int value; } rules;
typedef void (*answer)(void *state BIND("kind=context"), reply *out BIND("direction=out;length=1")) BIND("failure=contain;thread=native;reentry=protocol;reentry_owner=out;reentry_calls=reply_set");
BIND("execution=immediate") mln_status reply_set(reply *response BIND("length=1"), int value);
BIND("execution=immediate;callback_adapter=answer;context_type=rules;invokes=reply_set") void adapter(void *state BIND("kind=context"), reply *out BIND("direction=out"));
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
        self.assertEqual(
            (adapter.callback, adapter.context, adapter.invokes),
            ("answer", "rules", ("reply_set",)),
        )
        self.assertIn("rules", api.values)
        with self.assertRaisesRegex(ModelError, "outside its callback protocol"):
            bind(
                self.parse(source.replace("invokes=reply_set", "invokes=adapter")),
                require_complete=True,
            )
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
 const item *items BIND("length=count;ownership=borrowed;stride=stride;item_name=message;item_buffer=bytes;item_buffer_size=byte_count;item_offset=offset;item_length=count;item_encoding=utf8");
 unsigned int count BIND("kind=count");
 const char *bytes BIND("length=byte_count;encoding=bytes;ownership=borrowed");
 unsigned int byte_count BIND("kind=count");
} batch;
BIND("execution=immediate") mln_status capture(batch *out BIND("direction=out"));
"""
        api = bind(self.parse(source), require_complete=True)
        value = next(f.value for f in api.values["batch"].fields if f.name == "items")
        self.assertEqual(value.stride, "stride")
        with self.assertRaisesRegex(ModelError, "input reconstruction contract"):
            bind(
                self.parse(
                    source.replace(
                        'batch *out BIND("direction=out")',
                        'const batch *input BIND("length=1;ownership=borrowed")',
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
typedef void (*logger)(void *state BIND("kind=context")) BIND("failure=contain;thread=native;reentry=forbid");
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
BIND("execution=immediate") mln_status set_text(const char *value BIND("length=count;encoding=utf8;ownership=borrowed"), unsigned int count);
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
typedef struct payload { const byte *bytes BIND("length=count;encoding=bytes;ownership=borrowed"); unsigned int count BIND("kind=count"); } payload;
BIND("execution=immediate") mln_status capture(payload *out BIND("direction=out"));
"""),
            require_complete=True,
        )
        byte_value = byte_api.values["payload"].fields[0].value
        self.assertEqual((byte_value.kind, byte_value.encoding), ("buffer", "bytes"))

    def test_callback_response_preserves_scope_and_native_receiver(self):
        source = """
typedef struct reply { void *context BIND("kind=context;ownership=borrowed"); } reply BIND("kind=callback_response");
typedef void (*answer)(reply *out BIND("direction=out;length=1")) BIND("failure=ignore;thread=native");
BIND("execution=immediate") mln_status reply_set(reply *response BIND("length=1;direction=inout"), int value);
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
                self.parse(source.replace("kind=context", "kind=native_pointer")),
                require_complete=True,
            )

    def test_borrowed_output_view_tracks_owner_and_parent_invalidation(self):
        source = """
typedef unsigned long long parent BIND("kind=handle;release=parent_close;dispose=parent_close;abandon=parent_abandon;parent=none");
typedef unsigned long long child BIND("kind=handle;release=child_close;dispose=child_close;parent=parent");
typedef struct view { void *texture BIND("kind=native_pointer;ownership=borrowed"); } view;
BIND("execution=immediate") mln_status parent_close(parent value);
BIND("execution=immediate") mln_status parent_abandon(parent value);
BIND("execution=immediate") mln_status child_close(child value);
BIND("execution=immediate;view_owner=owner") mln_status child_view(child owner, view *out BIND("direction=out"));
"""
        api = bind(self.parse(source), require_complete=True)
        view = api.operations_by_name["child_view"].view
        self.assertEqual(view.owner_parameter, "owner")
        self.assertEqual(view.owner.native, "child")
        self.assertEqual(
            view.invalidated_by, ("child_close", "parent_close", "parent_abandon")
        )
        with self.assertRaisesRegex(ModelError, "input handle"):
            bind(
                self.parse(source.replace("view_owner=owner", "view_owner=out")),
                require_complete=True,
            )

    def test_ambiguous_pointer_cannot_become_a_generated_operation(self):
        source = """
BIND("execution=immediate") mln_status write_data(const double *values);
"""
        model = bind(self.parse(source))
        self.assertFalse(model.operations)
        self.assertIn("write_data", model.unsupported)
        with self.assertRaisesRegex(ModelError, "pointer requires length"):
            bind(self.parse(source), require_complete=True)

    def test_record_pointer_cardinality_is_explicit(self):
        source = """
typedef struct coordinate { double latitude; double longitude; } coordinate;
BIND("execution=immediate") mln_status project(
  const coordinate *coordinates BIND("length=count"), unsigned count);
"""
        model = bind(self.parse(source), require_complete=True)
        array = model.operations[0].inputs[0].value
        self.assertEqual(
            (array.kind, array.length, array.element.kind), ("array", "count", "record")
        )
        single = bind(
            self.parse(source.replace("length=count", "length=1")),
            require_complete=True,
        )
        self.assertEqual(single.operations[0].inputs[0].value.kind, "reference")
        with self.assertRaisesRegex(ModelError, "pointer requires length"):
            bind(
                self.parse(source.replace(' BIND("length=count")', "")),
                require_complete=True,
            )

    def test_enum_output_keeps_output_storage_indirection(self):
        model = bind(
            self.parse("""
typedef enum flags : unsigned { FIRST = 1, SECOND = 2 } flags;
BIND("execution=immediate") mln_status read_flags(
  unsigned *out BIND("direction=out;enum=flags"));
"""),
            require_complete=True,
        )
        output = model.operations[0].outputs[0].value
        self.assertEqual(output.kind, "reference")
        self.assertEqual(output.element.kind, "enum")

    def test_counted_output_preserves_array_storage(self):
        model = bind(
            self.parse("""
BIND("execution=immediate") mln_status copy_values(
  double *out BIND("direction=out;length=count"), unsigned count);
"""),
            require_complete=True,
        )
        output = model.operations[0].outputs[0].value
        self.assertEqual((output.kind, output.length), ("array", "count"))

    def test_callback_signature_and_retirement_form_one_registration(self):
        source = """
typedef void (*notify)(void *context BIND("kind=context")) BIND("thread=native;failure=contain");
typedef void (*release)(void *context BIND("kind=context")) BIND("thread=native;failure=contain");
typedef struct registration {
  notify callback;
  void *context BIND("kind=context");
  release release;
} registration BIND("kind=callback_registration;user_data=context;release=release");
BIND("execution=immediate") mln_status install(const registration *value BIND("length=1"));
"""
        model = bind(self.parse(source), require_complete=True)
        plan = model.operations[0].registrations[0]
        self.assertEqual(
            (plan.callbacks, plan.user_data, plan.release),
            (("callback",), "context", "release"),
        )
        self.assertEqual(model.callbacks["notify"].thread, "native")
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
                    )
                ),
                require_complete=True,
            )
        with self.assertRaisesRegex(ModelError, "callback requires failure and thread"):
            bind(
                self.parse(
                    source.replace("thread=native;failure=contain", "thread=native", 1)
                ),
                require_complete=True,
            )

    def test_default_support_is_derived_from_a_checked_consumer(self):
        source = """
typedef struct options { double zoom; } options BIND("default=make_options");
BIND("execution=immediate") options make_options(void);
BIND("execution=immediate") mln_status set_options(const options *value BIND("length=1"));
"""
        model = bind(self.parse(source), require_complete=True)
        constructor = model.operations_by_name["make_options"]
        self.assertEqual(
            (constructor.role, constructor.support_for), ("support", "default:options")
        )
        self.assertEqual(model.operations_by_name["set_options"].role, "public")
        with self.assertRaisesRegex(ModelError, "no-argument constructor"):
            bind(
                self.parse(
                    source.replace("make_options(void)", "make_options(int ignored)")
                ),
                require_complete=True,
            )
        with self.assertRaisesRegex(ModelError, "checked default-constructor consumer"):
            bind(
                self.parse(
                    source.replace(
                        'BIND("execution=immediate") mln_status set_options',
                        'BIND("execution=immediate;support=runtime") mln_status set_options',
                    )
                ),
                require_complete=True,
            )

    def test_attachment_keeps_immediate_owner_separate_from_completion(self):
        source = """
typedef struct mln_completion { unsigned size; } mln_completion;
typedef unsigned long root BIND("kind=handle;release=close_root;parent=none");
typedef unsigned long child BIND("kind=handle;release=close_child;parent=root;abandon=abandon_child");
BIND("execution=immediate") mln_status close_root(root value);
BIND("execution=immediate") mln_status close_child(child value);
BIND("execution=immediate") mln_status abandon_child(child value);
BIND("execution=lifecycle;result=void;shape=none;ownership=value") mln_status attach(
  root parent, child *owner BIND("direction=out;ownership=owned"),
  const mln_completion *done BIND("length=1"));
"""
        model = bind(self.parse(source), require_complete=True)
        operation = model.operations_by_name["attach"]
        assert operation.completion is not None
        owner = operation.completion.immediate_owners[0]
        self.assertEqual((owner.parameter, owner.parent_parameter), ("owner", "parent"))
        self.assertEqual(owner.handle.finalize, ("abandon_child", "close_child"))
        self.assertEqual(operation.owned_outputs, (owner,))
        self.assertTrue(operation.completion.inline)
        self.assertIn(
            ("completion_failure", "retain_immediate_owner"),
            [(step.phase, step.action) for step in operation.completion.transitions],
        )
        self.assertEqual(operation.completion.native_release, "quiescence")
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
                        "abandon_child(child value)", "abandon_child(root value)"
                    )
                ),
                require_complete=True,
            )

    def test_consuming_release_preserves_required_synchronization(self):
        model = bind(
            self.parse("""
typedef unsigned long frame BIND("kind=handle;release=release_frame;parent=none");
typedef struct sync { unsigned kind; unsigned long object; } sync;
BIND("execution=immediate") mln_status release_frame(
  frame *value BIND("direction=inout;consumes=success"), const sync *consumer BIND("length=1"));
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
typedef unsigned long request BIND("kind=handle;release=release_request;parent=none");
typedef void (*cancel)(void *context BIND("kind=context")) BIND("thread=native;failure=contain");
typedef void (*release_cancel)(void *context BIND("kind=context")) BIND("thread=native;failure=contain");
typedef unsigned (*provider)(request ticket) BIND("thread=native;failure=DELEGATE;decision_handle=ticket;decision_accept=CLAIM;decision_pass=DELEGATE;complete=answer;cancelled=is_cancelled;cancel_registration=on_cancel;wait_retired=await_retirement");
BIND("execution=immediate") mln_status answer(request value, unsigned response);
BIND("execution=immediate") mln_status is_cancelled(request value, bool *result BIND("direction=out"));
BIND("execution=immediate;registration=callback;user_data=context;release_callback=release;accepted_unless=cancelled") mln_status on_cancel(
  request value, cancel callback, void *context BIND("kind=context"), release_cancel release, bool *cancelled BIND("direction=out"));
BIND("execution=immediate") void release_request(request value);
BIND("execution=immediate") mln_status await_retirement(request value BIND("handle_access=issued"));
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
        self.assertIn(
            ("complete_enter", "force_accept_decision"),
            [(step.phase, step.action) for step in decision.transitions],
        )
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
            ("handle_access=issued", "handle_access=live", "issued decision handle"),
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

    def test_deferred_callbacks_answer_early_and_copy_their_inputs(self):
        source = """
typedef enum decision : unsigned { DELEGATE = 0, CLAIM = 1 } decision;
typedef unsigned long request BIND("kind=handle;release=release_request;parent=none");
typedef void (*cancel)(void *context BIND("kind=context")) BIND("thread=native;failure=contain");
typedef void (*release_cancel)(void *context BIND("kind=context")) BIND("thread=native;failure=contain");
typedef unsigned (*provider)(void *context BIND("kind=context"), const char *url BIND("length=nul;encoding=utf8;lifetime=call"), request ticket) BIND("thread=native;enum=decision;failure=DELEGATE;deferred=CLAIM;decision_handle=ticket;decision_accept=CLAIM;decision_pass=DELEGATE;complete=answer;cancelled=is_cancelled;cancel_registration=on_cancel;wait_retired=await_retirement");
typedef unsigned (*logger)(void *context BIND("kind=context"), int code) BIND("thread=native;failure=0;deferred=1");
BIND("execution=immediate") mln_status answer(request value, unsigned response);
BIND("execution=immediate") mln_status is_cancelled(request value, bool *result BIND("direction=out"));
BIND("execution=immediate;registration=callback;user_data=context;release_callback=release;accepted_unless=cancelled") mln_status on_cancel(
  request value, cancel callback, void *context BIND("kind=context"), release_cancel release, bool *cancelled BIND("direction=out"));
BIND("execution=immediate") void release_request(request value);
BIND("execution=immediate") mln_status await_retirement(request value BIND("handle_access=issued"));
"""
        model = bind(self.parse(source), require_complete=True)
        self.assertEqual(model.callbacks["provider"].deferred, "CLAIM")
        self.assertEqual(model.callbacks["logger"].deferred, "1")
        for before, after, error in (
            ("deferred=CLAIM", "deferred=OTHER", "value of decision"),
            ("deferred=CLAIM", "deferred=DELEGATE", "requires the accept value"),
            ("deferred=1", "deferred=-1", "integer the callback result"),
            ("deferred=1", "deferred=yes", "integer the callback result"),
            (
                'void *context BIND("kind=context"), int code',
                "int code",
                "one context parameter",
            ),
            (
                "int code) BIND",
                'int *code BIND("direction=out")) BIND',
                "copyable input",
            ),
            (
                'typedef void (*cancel)(void *context BIND("kind=context")) BIND("thread=native;failure=contain");',
                'typedef void (*cancel)(void *context BIND("kind=context")) BIND("thread=native;failure=contain;deferred=0");',
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
typedef struct mln_completion { unsigned size; } mln_completion;
typedef struct entry { double value; } entry;
BIND("execution=query;result=entry;shape=array;ownership=borrowed;nullable=true")
mln_status query(const mln_completion *completion BIND("length=1"));
"""),
            require_complete=True,
        )
        result = model.operations[0].result
        assert result is not None and result.element is not None
        self.assertTrue(result.nullable)
        self.assertFalse(result.element.nullable)

    def test_disposal_support_requires_a_handle_consumer(self):
        source = """
typedef unsigned long owner BIND("kind=handle;release=close_owner;dispose=discard_owner;parent=none");
BIND("execution=immediate") mln_status close_owner(owner value);
BIND("execution=immediate") mln_status discard_owner(owner value);
"""
        model = bind(self.parse(source), require_complete=True)
        self.assertEqual(
            model.operations_by_name["discard_owner"].support_for, "dispose:owner"
        )
        self.assertEqual(model.operations_by_name["close_owner"].role, "public")
        with self.assertRaisesRegex(ModelError, "receiver must be this handle type"):
            bind(
                self.parse(
                    source.replace(
                        "discard_owner(owner value)", "discard_owner(unsigned value)"
                    )
                ),
                require_complete=True,
            )


if __name__ == "__main__":
    unittest.main()
