using Maplibre.NativeFfi;

MaplibreNative.LoadNativeLibrary();
using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
Console.WriteLine($"Loaded MapLibre Native C ABI {MaplibreNative.CVersion()}.");
