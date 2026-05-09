---
title: Java JNI Binding Conventions
description: Language-specific implementation conventions for Java JNI bindings.
---

Resources:

- Tracking issue:
  [#46](https://github.com/maplibre/maplibre-native-ffi/issues/46)
- [JNI specification](https://docs.oracle.com/en/java/javase/25/docs/specs/jni/)
- [Android JNI tips](https://developer.android.com/training/articles/perf-jni)
- [Rust `jni` crate](https://docs.rs/jni/)
- [Java FFM conventions](/maplibre-native-ffi/development/bindings-java-ffm/)
- [JavaCPP](https://github.com/bytedeco/javacpp) and
  [SWIG](https://www.swig.org/)

The Java JNI binding targets Android and JVMs where FFM is unavailable. Modern
JVMs with FFM support use the
[Java FFM conventions](/maplibre-native-ffi/development/bindings-java-ffm/).

JNI is a native bridge path. Java calls declared `native` methods, and a
companion native library implements those JNI entry points and calls the public
C API.

Build the bridge library in Rust with the `jni` crate from jni-rs over the
shared internal crates defined by the
[Rust binding conventions](/maplibre-native-ffi/development/bindings-rust/). For
JNI, jni-rs plays the same role that PyO3 plays for Python: it adapts Rust code
to the host runtime's native ABI. JNI policy stays in project-owned bridge code.
JavaCPP and SWIG can inform generated-assisted coverage. Rust plus jni-rs is the
supported bridge path.

Keep JNI types internal: `JNIEnv`, `JavaVM`, `jobject`, JNI reference handles,
generated C layout classes, and raw `long` pointers stay below the public API.
Backend-native handles cross the public API as `NativePointer` values. The
bridge converts them to native pointer representations.

Load the bridge with the package's normal Java or Android library-loading
mechanism. Prefer explicit native-method registration from `JNI_OnLoad` for
generated method tables and startup validation. Native calls translate C
statuses and diagnostics into Java exceptions. Rust bridge code catches panics
before returning through JNI.

Ordinary Java calls execute where they are invoked and report native
wrong-thread errors. Use an execution context that preserves native thread
identity across related calls. Java scheduling and UI routing belong in adapters
above this layer.

The C API expects standard UTF-8 text. The bridge transcodes Java strings
through UTF-16 or Java UTF-8 byte arrays before passing them to C API text
fields.

JNI callback state uses global references for Java callbacks that native code
may invoke after the registering native method returns. Store those references
in owner-scoped native state. Callback code attaches native threads to the JVM
before invoking Java code, detaches threads that the bridge attached, and clears
or reports pending Java exceptions before returning to C according to the
callback contract.
