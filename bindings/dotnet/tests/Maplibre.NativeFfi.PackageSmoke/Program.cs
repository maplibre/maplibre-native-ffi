using Maplibre.NativeFfi;
using Maplibre.NativeFfi.Runtime;
using NativeMaplibre = Maplibre.NativeFfi.Maplibre;

NativeMaplibre.LoadNativeLibrary();
using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
Console.WriteLine($"Loaded MapLibre Native C ABI {NativeMaplibre.CVersion()}.");
