using Maplibre.NativeFfi;

MaplibreNative.LoadNativeLibrary();
using var runtime = RuntimeHandle.Create(new RuntimeOptions());
Console.WriteLine($"Loaded MapLibre Native C ABI {MaplibreNative.CVersion()}.");
