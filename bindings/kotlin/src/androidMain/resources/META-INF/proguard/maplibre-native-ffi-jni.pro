# The JNI shim registers these natives and looks up these upcalls by name when
# libmaplibre-native-ffi-jni.so loads, so neither the classes nor their members
# may be renamed or removed.
-keep class org.maplibre.nativeffi.internal.c.C { native <methods>; }
-keep class org.maplibre.nativeffi.internal.c.Jni { native <methods>; }
-keep class org.maplibre.nativeffi.internal.memory.NativeMemory { native <methods>; }
-keep class org.maplibre.nativeffi.internal.c.Upcalls { public static <methods>; }
-keep class org.maplibre.nativeffi.internal.c.ThreadedUpcalls { public static <methods>; }

# The native library's HTTPS verifies certificates by calling
# rustls-platform-verifier's Android component through JNI.
-keep,includedescriptorclasses class org.rustls.platformverifier.** { *; }
