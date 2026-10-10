"""Identifier rules shared by the managed-language emitters."""

from .names import camel, pascal, type_name

# Words that no Dart declaration can use as a name.
DART_RESERVED = {
    "assert",
    "break",
    "case",
    "catch",
    "class",
    "const",
    "continue",
    "default",
    "do",
    "else",
    "enum",
    "extends",
    "false",
    "final",
    "finally",
    "for",
    "if",
    "in",
    "is",
    "new",
    "null",
    "rethrow",
    "return",
    "super",
    "switch",
    "this",
    "throw",
    "true",
    "try",
    "var",
    "void",
    "while",
    "with",
}

# Dart built-in and contextual identifiers, which some declarations cannot use.
DART_BUILTIN = {
    "Function",
    "abstract",
    "as",
    "async",
    "await",
    "base",
    "covariant",
    "deferred",
    "dynamic",
    "export",
    "extension",
    "external",
    "factory",
    "get",
    "hide",
    "implements",
    "import",
    "interface",
    "late",
    "library",
    "mixin",
    "of",
    "on",
    "operator",
    "part",
    "required",
    "sealed",
    "set",
    "show",
    "static",
    "sync",
    "typedef",
    "when",
    "yield",
}

# Reserved words of .NET and Dart. Each emitter escapes, renames, or rejects a
# public name that matches one.
KEYWORDS = {
    "dotnet": {
        "abstract",
        "as",
        "base",
        "bool",
        "break",
        "byte",
        "case",
        "catch",
        "char",
        "checked",
        "class",
        "const",
        "continue",
        "decimal",
        "default",
        "delegate",
        "do",
        "double",
        "else",
        "enum",
        "event",
        "explicit",
        "extern",
        "false",
        "finally",
        "fixed",
        "float",
        "for",
        "foreach",
        "goto",
        "if",
        "implicit",
        "in",
        "int",
        "interface",
        "internal",
        "is",
        "lock",
        "long",
        "namespace",
        "new",
        "null",
        "object",
        "operator",
        "out",
        "override",
        "params",
        "private",
        "protected",
        "public",
        "readonly",
        "ref",
        "return",
        "sbyte",
        "sealed",
        "short",
        "sizeof",
        "stackalloc",
        "static",
        "string",
        "struct",
        "switch",
        "this",
        "throw",
        "true",
        "try",
        "typeof",
        "uint",
        "ulong",
        "unchecked",
        "unsafe",
        "ushort",
        "using",
        "virtual",
        "void",
        "volatile",
        "while",
    },
    "dart": DART_RESERVED | DART_BUILTIN,
}
LOCALS = {
    "completion",
    "result",
    "value",
    "values",
    "copied",
    "index",
    "arena",
    "scope",
    "cancellationToken",
    "nativeArgs",
    "bindingMapHandle",
    "handle",
    "_handle",
}


def conflicting_functions(bound, language: str) -> set[str]:
    """Reject ambiguous public method names instead of emitting overloads.

    Two operations collide when they share a receiver and their members
    convert to the same name in the target language.
    """
    convert = pascal if language == "dotnet" else camel
    owners = {}
    for plan in bound.operations:
        receiver = plan.receiver or plan.scoped_receiver
        owner = (
            type_name(
                next(p for p in plan.function.parameters if p.name == receiver).type
            )
            if receiver
            else None
        )
        owners.setdefault((owner, convert(plan.member)), []).append(plan.name)
    return {name for names in owners.values() if len(names) > 1 for name in names}
