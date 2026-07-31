# JavaCPP resolves the JNI library name reflectively from the generated presets
# class and the @Properties-annotated config it extends. Renaming either makes
# Loader ask for libjni<obfuscated-name>.so and the app dies on first use.
-keep class org.maplibre.nativeffi.internal.javacpp.** { *; }

# Loader.getCallerClass locates itself in a live stack trace and reads the frame
# a fixed distance above it. Inlining anything inside Loader shifts that frame,
# so it resolves the wrong class and derives the wrong library name. Native code
# also reads Pointer's fields, and JavaCPP reads its annotations reflectively.
# Scoped to the runtime package: org.bytedeco.javacpp.tools is build-time only.
-keep class org.bytedeco.javacpp.* { *; }
-keep class org.bytedeco.javacpp.annotation.** { *; }
-keepattributes *Annotation*

# JavaCPP also carries JVM-only code paths that Android does not ship: JMX
# pointer accounting, an slf4j logger, and an OSGi annotation. Android never
# reaches them, so let R8 shrink them away instead of failing the build.
-dontwarn java.lang.management.**
-dontwarn javax.management.**
-dontwarn org.osgi.annotation.**
-dontwarn org.slf4j.**
