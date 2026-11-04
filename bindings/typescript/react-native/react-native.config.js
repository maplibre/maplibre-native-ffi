module.exports = {
  dependency: {
    platforms: {
      android: {
        packageImportPath:
          "import org.maplibre.nativeffi.reactnative.MaplibreNativeFfiPackage;",
        packageInstance: "new MaplibreNativeFfiPackage()",
      },
    },
  },
};
