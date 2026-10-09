"""Direct callback registration whose native release frees the binding root."""

from ..model import ModelError
from ..semantic import FieldPlan, RegistrationPlan, ValuePlan
from .zig import (
    DIAGNOSTIC_PARAMETER,
    DIAGNOSTIC_PREAMBLE,
    camel,
    identifier,
    pascal,
    status_call,
)
from .zig_callbacks import parts


def operation(plan, values):
    registration = plan.direct_registrations[0]
    if not registration.release_callback:
        raise ModelError([f"{plan.name}: direct callback requires a native release"])
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
    policy = callback_plan.reentry_policy
    # A callback restricted to its registration owner records that owner.
    owned = bool(policy and policy.registration_owner)
    if owned:
        if not plan.receiver:
            raise ModelError([f"{plan.name}: registration owner requires a receiver"])
        fields.append("    owner: u64 = 0,")
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
    if owned:
        setup.append(
            f"if ({public_callback}) |value| {{ var retained = value; retained.owner = {receiver_name}.raw; if (retained.call != null) context = try roots.retain({name}, retained); }}"
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
                f"if (context != null) &{name}.{identifier('callTrampoline')} else null"
            )
        elif p.name == registration.user_data:
            args.append("context")
        elif p.name == registration.release_callback:
            args.append(
                f"if (context != null) &callback.Registration({name}).releaseNative else null"
            )
        elif p.name == registration.accepted_unless:
            setup.append("var rejected: bool = false;")
            args.append("&rejected")
        else:
            raise ModelError([f"{plan.name}: unsupported direct callback parameter"])
    signature.append(DIAGNOSTIC_PARAMETER)
    setup[:0] = DIAGNOSTIC_PREAMBLE
    setup.append(status_call(plan.function, args))
    if registration.accepted_unless:
        # A rejected registration stores nothing, so the caller keeps its
        # context and the roots release without its release_context.
        setup.append("if (rejected) return true;")
    setup.append("roots.accept();")
    if registration.accepted_unless:
        setup.append("return false;")
    return_type = "bool" if registration.accepted_unless else "void"
    return (
        declaration
        + f"pub fn {camel(plan.name.removeprefix('mln_'))}({', '.join(signature)}) status.Error!{return_type} {{\n    "
        + "\n    ".join(setup)
        + "\n}\n"
    )
