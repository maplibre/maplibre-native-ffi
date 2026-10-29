module.exports = {
  dependency: {
    platforms: {
      android: {
        sourceDir: "android",
        packageImportPath:
          "import org.maplibre.nativeffi.MaplibreNativeFfiPackage;",
        packageInstance: "new MaplibreNativeFfiPackage()",
        cmakeListsPath: "build/generated/source/codegen/jni/CMakeLists.txt",
      },
    },
  },
};
