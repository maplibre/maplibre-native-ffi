"""Direct callback registration with native release or generational callback tokens."""

from ..semantic import FieldPlan, RegistrationPlan, ValuePlan
from .zig import camel, identifier, pascal
from .zig_callbacks import parts


def operation(plan, values):
    registration = plan.direct_registrations[0]
    parameter = next(p for p in plan.inputs if p.name == registration.callback)
    callback_plan = values.bound.callbacks[parameter.value.native]
    for item in callback_plan.parameters:
        if item.name != callback_plan.context:
            values.add(item.value)
    if callback_plan.result.ctype.kind != "void":
        values.add(callback_plan.result)
    name = pascal(parameter.value.native.removeprefix("mln_"))
    descriptor = ValuePlan(
        kind="record",
        native=parameter.value.native,
        ctype=parameter.value.ctype,
        fields=(FieldPlan(name="call", value=parameter.value),),
        registration=RegistrationPlan(
            parameter="callback",
            descriptor=parameter.value.native,
            callbacks=("call",),
            user_data="user_data",
            release="release_user_data",
        ),
    )
    fields, _, _, trampolines = parts(values, descriptor)
    token = registration.owner_release is not None
    if token:
        fields.append("    owner: u64 = 0,")
        trampolines = [
            line.replace(
                f"callback.Registration({name}).get(callback_arg_0)",
                f"callback.Token({name}).get(callback_arg_0) orelse return",
            ).replace(
                "const host =",
                f"defer callback.Registration({name}).releaseErased(state); const host =",
            )
            for line in trampolines
        ]
    declaration = (
        f"pub const {name} = struct {{\n"
        + "\n".join([*fields, *trampolines])
        + "\n};\n"
    )
    input_names = {
        p.name: f"binding_arg_{i}" for i, p in enumerate(plan.function.parameters)
    }
    args, signature, setup = [], [], []
    public_callback = input_names[registration.callback]
    receiver_name = input_names.get(plan.receiver)
    if receiver_name:
        receiver = next(p.value for p in plan.inputs if p.name == plan.receiver)
        signature.append(f"{receiver_name}: {values.public(receiver)}")
        setup.extend(
            [f"const lease = try {receiver_name}.lease();", "defer lease.release();"]
        )
    signature.append(f"{public_callback}: ?{name}")
    setup.insert(
        0,
        f'try callback.check("{plan.name}", {receiver_name + ".raw" if receiver_name else "0"});',
    )
    setup.extend(
        [
            "var roots: callback.Roots = .{};",
            "defer roots.deinit();",
            "var context: ?*anyopaque = null;",
        ]
    )
    if token:
        setup.append(
            f"if ({public_callback}) |value| {{ var retained = value; retained.owner = {receiver_name}.raw; if (retained.call != null) context = try callback.Token({name}).create(&roots, retained); }}"
        )
    else:
        setup.append(
            f"if ({public_callback}) |value| {{ if (value.call != null) context = try roots.retain({name}, value); }}"
        )
    for p in plan.function.parameters:
        if p.name == plan.receiver:
            args.append("lease.native")
        elif p.name == registration.callback:
            args.append(
                f"if (context != null) {name}.{identifier('callTrampoline')} else null"
            )
        elif p.name == registration.user_data:
            args.append("context")
        elif p.name == registration.release_callback:
            args.append(
                f"if (context != null) callback.Registration({name}).releaseNative else null"
            )
        elif p.name == registration.accepted_unless:
            setup.append("var already_cancelled: bool = false;")
            args.append("&already_cancelled")
    if token:
        setup.extend(
            [
                "var keep_registration = false;",
                "if (context != null) try lease.attachCallback(roots.items.items[0]);",
                "defer if (context != null and !keep_registration) lease.detachCallback();",
            ]
        )
    setup.append(
        f"try status.checkStatus(c.{plan.name}({', '.join(args)}), {'lease.diagnostic_store' if receiver_name else 'null'});"
    )
    if token:
        setup.append("if (already_cancelled) return true;")
        setup.append("keep_registration = true;")
    setup.append("roots.accept();")
    if token:
        setup.append("return false;")
    return_type = "bool" if token else "void"
    return (
        declaration
        + f"pub fn {camel(plan.name.removeprefix('mln_'))}({', '.join(signature)}) status.Error!{return_type} {{\n    "
        + "\n    ".join(setup)
        + "\n}\n",
        "global",
        "",
        None,
    )
