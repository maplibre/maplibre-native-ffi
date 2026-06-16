// MapLibre Native's default HTTP file source includes <dlfcn.h> but does not
// use any dl* APIs on Windows. Provide the header locally so Windows builds do
// not need a dlfcn-win32 package for that unused include.
