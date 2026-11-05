"""The forms that Java calls in place of Kotlin members that it can't call.

Kotlin compiles an unsigned type to a value class, so a member that takes or
returns one has a mangled JVM name, and a constructor that takes one has a
synthetic marker parameter. Java instead passes `int` for a u32 and `long` for
a u64, with the same bits.

- `@JvmName` keeps a member's JVM name where its JVM types are already signed
  primitives: a non-null unsigned parameter or result.
- A nullable or generic unsigned type compiles to a boxed `kotlin.UInt`, which
  Java can't create or read, so the Kotlin member is `@JvmSynthetic` and a
  `@PublishedApi internal` twin takes or returns the signed type. Other Kotlin
  modules can't see the twin.
- A record with a member that has a default gets a static `of` factory with the
  members that have none, and a `withX` copy for each member that has one, so
  Java never restates a default or passes null for an unset option, and a
  member that C appends to a record doesn't break Java's calls. A view's copy
  keeps the view's scope. Another record
  whose constructor takes an unsigned member gets an `of` factory that takes
  every member. Both are hidden from Kotlin, which uses the constructor's named
  and default arguments.
"""

from __future__ import annotations

import re

from ..names import pascal

UNSIGNED = {"UByte": "Byte", "UShort": "Short", "UInt": "Int", "ULong": "Long"}
NESTED = re.compile(r"[<(,]\s*U(Byte|Short|Int|Long)\b")


def unsigned(typ: str) -> bool:
    """Whether a Kotlin type is an unsigned scalar, nullable or not, which mangles
    the JVM name of a member that takes or returns it."""
    return typ.removesuffix("?") in UNSIGNED


def nested(typ: str) -> bool:
    """Whether a Kotlin type holds an unsigned scalar as a type argument."""
    return bool(NESTED.search(typ))


def boxed(typ: str) -> bool:
    """Whether Java sees a Kotlin type as a boxed unsigned value it can't use."""
    return (unsigned(typ) and typ.endswith("?")) or nested(typ)


def signed(typ: str) -> str:
    """The type that Java passes for a Kotlin type: `Int` for `UInt`."""
    bare = typ.removesuffix("?")
    if bare in UNSIGNED:
        return UNSIGNED[bare] + typ[len(bare) :]
    if nested(typ):
        raise ValueError(f"{typ}: a nested unsigned type needs a Java form")
    return typ


def to_kotlin(expression: str, typ: str) -> str:
    """Convert [expression] of Java's type for Kotlin type [typ] to [typ]."""
    bare = typ.removesuffix("?")
    if bare not in UNSIGNED:
        return expression
    return f"{expression}{'?' if typ.endswith('?') else ''}.to{bare}()"


def to_java(expression: str, typ: str) -> str:
    """Convert [expression] of Kotlin type [typ] to the type Java reads."""
    bare = typ.removesuffix("?")
    if bare not in UNSIGNED:
        return expression
    return f"{expression}{'?' if typ.endswith('?') else ''}.to{UNSIGNED[bare]}()"


def bare(member: str) -> str:
    return member.strip("`")


def getter(member: str) -> str:
    """The JVM name of a property's getter."""
    name = bare(member)
    if re.match(r"is[A-Z]", name):
        return name
    return "get" + name[0].upper() + name[1:]


def property_annotation(member: str, typ: str) -> str:
    """What keeps a public property's getter callable from Java, or hides it behind a twin."""
    if boxed(typ):
        return "@get:JvmSynthetic "
    if unsigned(typ):
        return f'@get:JvmName("{getter(member)}") '
    return ""


def property_twin(member: str, typ: str, indent: str = "  ") -> list[str]:
    """The getter that Java calls in place of a property it can't read."""
    if not boxed(typ):
        return []
    return [
        (
            f'{indent}@PublishedApi @JvmName("{getter(member)}") internal fun '
            f"{bare(member)}ForJava(): {signed(typ)} = {to_java(member, typ)}"
        )
    ]


def function_annotation(method: str, types: list[str]) -> str:
    """The `@JvmName` that keeps a function's name when an unsigned type would mangle it."""
    if any(boxed(typ) for typ in types):
        raise ValueError(f"{method}: a boxed unsigned type needs a Java twin")
    if any(unsigned(typ) for typ in types):
        return f'@JvmName("{bare(method)}") '
    return ""


def record(public: str, members: list[tuple[str, str, str | None]], view: bool = False):
    """The Java members of a record with [members]: (name, type, default).

    Returns the lines of the class body and those of its companion. The `of`
    factory of a record with a member that has a default takes the members
    without one, and Java sets each other one with a `withX` copy. A record
    whose constructor Java can't call, and that has no option that defaults to
    null, also gets an `of` factory that takes every member. A [view] is a
    class rather than a data class, so its copy calls the constructor and keeps
    the view's `bindingScope`.
    """
    body = [line for member, typ, _ in members for line in property_twin(member, typ)]
    factories = []
    options = (
        "Creates a value from the members without defaults, for Java, which sets "
        "the others with the `withX` methods."
    )
    if any(default is not None for _, _, default in members):
        for member, typ, default in members:
            if default is None:
                continue
            value = typ.removesuffix("?")
            update = f"{member} = {to_kotlin(member, value)}"
            if view:
                arguments = ", ".join(
                    update if other == member else f"{other} = {other}"
                    for other, _, _ in members
                )
                copy = (
                    f"{public}({arguments}).also {{ it.bindingScope = bindingScope }}"
                )
            else:
                copy = f"copy({update})"
            body.append(
                f"  @PublishedApi internal fun with{pascal(bare(member))}({member}: {signed(value)}): "
                f"{public} = {copy}"
            )
        factories.append(
            (
                [(member, typ) for member, typ, default in members if default is None],
                options,
            )
        )
    if any(unsigned(typ) for _, typ, _ in members) and not any(
        default == "null" for _, _, default in members
    ):
        factories.append(
            (
                [(member, typ) for member, typ, _ in members],
                "Creates a value, for Java, which can't call the constructor.",
            )
        )
    companion = []
    for required, summary in factories:
        parameters = ", ".join(f"{member}: {signed(typ)}" for member, typ in required)
        arguments = ", ".join(
            f"{member} = {to_kotlin(member, typ)}" for member, typ in required
        )
        companion.append(
            f"    /** {summary} */\n"
            f"    @JvmStatic @PublishedApi internal fun of({parameters}): {public} = "
            f"{public}({arguments})"
        )
    return body, companion


def class_body(lines: list[str], companion: list[str]) -> str:
    """A class body of [lines] and a public companion of [companion], or nothing."""
    if companion:
        lines = [*lines, "  public companion object {", *companion, "  }"]
    return " {\n" + "\n".join(lines) + "\n}" if lines else ""
