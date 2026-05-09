---
title: Java JNI Binding Conventions
description: Language-specific implementation conventions for Java JNI bindings.
---

Resources:

- Tracking issue:
  [#46](https://github.com/maplibre/maplibre-native-ffi/issues/46)
- [JNI specification](https://docs.oracle.com/en/java/javase/25/docs/specs/jni/)
- [Android JNI tips](https://developer.android.com/training/articles/perf-jni)
- [Java FFM conventions](/maplibre-native-ffi/development/bindings-java-ffm/)
- [JavaCPP](https://github.com/bytedeco/javacpp) and
  [SWIG](https://www.swig.org/)

The Java JNI binding targets Android and JVMs where FFM is unavailable. Modern
JVMs with FFM support use the
[Java FFM conventions](/maplibre-native-ffi/development/bindings-java-ffm/).

Separate JNI access from the safe public binding. The internal C layer contains
generated native-method declarations and JNI bridge entry points that call the
public C API. Evaluate JavaCPP and SWIG for generated-assisted coverage. Treat
successful generation and compilation as the header bindability check for this
path.

Keep JNI types internal. Public APIs do not expose `JNIEnv`, `jobject`, JNI
reference handles, generated C layout classes, or raw `long` pointers.
Backend-native handles cross the public API as `NativePointer` values and
convert to internal JNI pointer values at the native-method boundary.

JNI modified UTF-8 is not the C API string encoding. The JNI bridge transcodes
through UTF-16 or Java UTF-8 byte arrays instead of passing
`GetStringUTFChars()` or `NewStringUTF()` bytes as general C API text.

JNI callback state uses global references for Java callbacks that native code
may invoke after the registering native method returns. Store those references
in owner-scoped native state:

- process-global state for logging callbacks;
- runtime-owned state for resource transforms and providers;
- map/style-owned state for custom geometry source callbacks;
- request-owned state for handled resource requests.

JNI callback code attaches native threads to the JVM before invoking Java code
and detaches threads that the binding attached. It clears or reports pending
Java exceptions before returning to C according to the callback contract.
