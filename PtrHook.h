#ifndef PtrHook_h
#define PtrHook_h

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int PtrHook(const char *module, uintptr_t offset, void *hook, void **orig);

#ifdef __cplusplus
}
#endif

#endif
