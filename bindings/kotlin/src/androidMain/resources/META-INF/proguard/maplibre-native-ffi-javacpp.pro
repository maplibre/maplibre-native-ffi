# JavaCPP derives the JNI library name reflectively from the generated presets
# class and the @Properties config it extends, so renaming either makes Loader
# ask for libjni<obfuscated-name>.so.
-keep class org.maplibre.nativeffi.internal.javacpp.** { *; }

# Loader.getCallerClass reads a frame a fixed distance above itself, so inlining
# inside Loader derives the wrong library name. Native code reads Pointer's
# fields, and JavaCPP reads its annotations reflectively. Scoped to the runtime
# package because org.bytedeco.javacpp.tools is build-time only.
-keep class org.bytedeco.javacpp.* { *; }
-keep class org.bytedeco.javacpp.annotation.** { *; }
-keepattributes *Annotation*,InnerClasses,EnclosingMethod

# JavaCPP carries JVM-only code paths Android does not ship and never reaches.
-dontwarn java.lang.management.**
-dontwarn javax.management.**
-dontwarn org.osgi.annotation.**
-dontwarn org.slf4j.**
