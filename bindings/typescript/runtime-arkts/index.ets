// Loads this payload's compiled addon on ArkTS.
//
// ArkTS implements Node-API and resolves a native module by the library name in
// the import specifier rather than by a path, so the addon is named here where
// every other runtime names a file.
import addon from 'libmaplibre-native-ffi.so';

import metadata from './runtime.json';

export const runtime = metadata;
export { addon };
