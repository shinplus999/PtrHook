# PtrHook

Scans data segments for function pointers and replaces them with your own. No need to know struct layout or vtable index.

## What it does

Say you wanna hook a C++ virtual method but you don't know its vtable index. Or there's a callback buried in some opaque struct. Normally you'd have to reverse the vtable layout or figure out the field offset — with this you just throw in the function offset and it scans all data sections for pointers to it.

Works for:
- C++ vtable entries
- Function pointers in structs (`ctx->onXXX`)
- ObjC method IMPs
- Dispatch tables, callback tables
- Protocol method lists

Doesn't work for:
- Direct calls (not going through a pointer)
- Inlined functions

## Usage

```c
#include "PtrHook.h"

// offset = function address in IDA/Hopper - base address
#define OFF_GameMethod  0x12345

static void (*orig_GameMethod)(void *self, int x);

static void hook_GameMethod(void *self, int x) {
    orig_GameMethod(self, x);
}

void init(void) {
    int n = PtrHook(NULL, OFF_GameMethod, hook_GameMethod, (void **)&orig_GameMethod);
    if (!n) return; // nothing found
}
```

Params:
- `module` — `NULL` for main binary, or a dylib name like `"UnityFramework"`
- `offset` — distance from base to the target function
- `hook` — your replacement
- `orig` — stores the original so you can call it later

Returns how many pointers were patched. Might be >1 if multiple vtables point to the same function.

## Finding the offset

Open the binary in Hopper/IDA, find your function, subtract the base address. ASLR slide is handled internally, don't worry about it.

You can also use RVA from a dump tool — if the dumper gives you raw file offsets, convert to VM address first. Same thing.

## How it works

1. Finds module base + slide via `_dyld`
2. Parses the Mach-O header, walks `__DATA` / `__DATA_CONST` / `__DATA_DIRTY`
3. Scans every section for 8-byte pointers that equal `base + offset`
4. `vm_protect` to make it writable, patches the pointer, locks it back

It's brute-force. Doesn't care about structs, doesn't care about indices.

## License

MIT
