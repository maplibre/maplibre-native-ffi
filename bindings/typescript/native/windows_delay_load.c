#include <windows.h>

// The SDK's delayimp.h needs the Windows types above.
#include <delayimp.h>
#include <string.h>

// Node, Bun, and Deno export Node-API from their executable. Resolve the
// delayed node.exe imports against the running host, including renamed hosts.
static FARPROC WINAPI load_host(unsigned event, PDelayLoadInfo info) {
  if (
    event == dliNotePreLoadLibrary && _stricmp(info->szDll, "node.exe") == 0
  ) {
    return (FARPROC)GetModuleHandleW(NULL);
  }
  return NULL;
}

const PfnDliHook __pfnDliNotifyHook2 = load_host;

// Older LLD delay thunks let the resolver overwrite saved floating-point
// arguments. Resolve imports first; the name must match node_api.def exactly.
__attribute__((constructor)) static void resolve_node_api(void) {
  __HrLoadAllImportsForDll("NODE.EXE");
}
