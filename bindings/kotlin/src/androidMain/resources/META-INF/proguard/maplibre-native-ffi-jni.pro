# The JNI shim registers these natives and looks up these upcalls by name when
# libmaplibre-native-ffi-jni.so loads, so neither the classes nor their members
# may be renamed or removed.
-keep class org.maplibre.nativeffi.internal.c.C { native <methods>; }
-keep class org.maplibre.nativeffi.internal.c.Jni { native <methods>; }
-keep class org.maplibre.nativeffi.internal.memory.NativeMemory { native <methods>; }
-keep class org.maplibre.nativeffi.internal.c.Upcalls { public static <methods>; }
