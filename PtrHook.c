#include "PtrHook.h"
#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <string.h>

static bool _wptr(void *addr, void *newPtr, void **orig) {
    mach_port_t port = mach_task_self();
    vm_address_t page = (vm_address_t)addr & ~(vm_page_size - 1);

    if (vm_protect(port, page, vm_page_size, false, VM_PROT_READ | VM_PROT_WRITE) != KERN_SUCCESS) {
        if (vm_protect(port, page, vm_page_size, false, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY) != KERN_SUCCESS)
            return false;
    }

    if (orig && !*orig) *orig = *(void **)addr;
    *(void **)addr = newPtr;
    vm_protect(port, page, vm_page_size, false, VM_PROT_READ);
    return true;
}

static uintptr_t _base(const char *name, const struct mach_header_64 **hdr, intptr_t *slide) {
    for (uint32_t i = 0; i < _dyld_image_count(); i++) {
        const char *img = _dyld_get_image_name(i);
        if (!img) continue;
        if (!name) {
            if (i != 0) continue;
            if (hdr) *hdr = (const struct mach_header_64 *)_dyld_get_image_header(0);
            if (slide) *slide = _dyld_get_image_vmaddr_slide(0);
            return (uintptr_t)_dyld_get_image_header(0);
        }
        if (strstr(img, name)) {
            if (hdr) *hdr = (const struct mach_header_64 *)_dyld_get_image_header(i);
            if (slide) *slide = _dyld_get_image_vmaddr_slide(i);
            return (uintptr_t)_dyld_get_image_header(i);
        }
    }
    return 0;
}

int PtrHook(const char *module, uintptr_t offset, void *hook, void **orig) {
    const struct mach_header_64 *hdr = NULL;
    intptr_t slide = 0;
    uintptr_t base = _base(module, &hdr, &slide);
    if (!base || !hdr) return 0;

    uintptr_t target = base + offset;
    int n = 0;

    struct load_command *lc = (struct load_command *)((uintptr_t)hdr + sizeof(struct mach_header_64));
    for (uint32_t i = 0; i < hdr->ncmds; i++) {
        if (lc->cmd != LC_SEGMENT_64) { lc = (struct load_command *)((uintptr_t)lc + lc->cmdsize); continue; }

        struct segment_command_64 *seg = (struct segment_command_64 *)lc;
        if (strcmp(seg->segname, "__DATA") && strcmp(seg->segname, "__DATA_CONST") && strcmp(seg->segname, "__DATA_DIRTY")) {
            lc = (struct load_command *)((uintptr_t)lc + lc->cmdsize);
            continue;
        }

        struct section_64 *sec = (struct section_64 *)((uintptr_t)seg + sizeof(struct segment_command_64));
        for (uint32_t j = 0; j < seg->nsects; j++) {
            uintptr_t start = sec[j].addr + slide;
            uintptr_t end = start + sec[j].size;
            for (uintptr_t p = start; p + 8 <= end; p += 8) {
                if (*(void **)p == (void *)target && _wptr((void *)p, hook, orig)) n++;
            }
        }
        lc = (struct load_command *)((uintptr_t)lc + lc->cmdsize);
    }
    return n;
}
