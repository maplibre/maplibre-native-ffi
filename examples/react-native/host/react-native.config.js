module.exports = {
  dependency: {
    platforms: {
      android: {
        packageImportPath:
          "import org.maplibre.nativeffi.example.host.MaplibreHostPackage;",
        packageInstance: "new MaplibreHostPackage()",
      },
    },
  },
};
