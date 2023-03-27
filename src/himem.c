#include <stdint.h>
#include <stddef.h>
#include <x68k/iocs.h>
#include <x68k/dos.h>
#include "himem.h"

//
//  allocate high memory
//
static void* __himem_malloc(size_t size) {

    struct iocs_regs in_regs = { 0 };
    struct iocs_regs out_regs = { 0 };

    in_regs.d0 = 0xF8;      // IOCS _HIMEM
    in_regs.d1 = 1;         // HIMEM_MALLOC
    in_regs.d2 = size;

    _iocs_trap15(&in_regs, &out_regs);

    uint32_t rc = out_regs.d0;

    return (rc == 0) ? (void*)out_regs.a1 : NULL;
}

//
//  free high memory
//
static void __himem_free(void* ptr) {
    
    struct iocs_regs in_regs = { 0 };
    struct iocs_regs out_regs = { 0 };

    in_regs.d0 = 0xF8;      // IOCS _HIMEM
    in_regs.d1 = 2;         // HIMEM_FREE
    in_regs.d2 = (size_t)ptr;

    _iocs_trap15(&in_regs, &out_regs);
}

//
//  getsize high memory
//
size_t __himem_getsize() {

    struct iocs_regs in_regs = { 0 };
    struct iocs_regs out_regs = { 0 };

    in_regs.d0 = 0xF8;          // IOCS _HIMEM
    in_regs.d1 = 3;             // HIMEM_GETSIZE

    _iocs_trap15(&in_regs, &out_regs);
  
    return (size_t)out_regs.d0;
}

//
//  resize high memory
//
int __himem_resize(void* ptr, size_t size) {

    struct iocs_regs in_regs = { 0 };
    struct iocs_regs out_regs = { 0 };

    in_regs.d0 = 0xF8;          // IOCS _HIMEM
    in_regs.d1 = 4;             // HIMEM_RESIZE
    in_regs.d2 = (size_t)ptr;
    in_regs.d3 = size;

    _iocs_trap15(&in_regs, &out_regs);
  
    return out_regs.d0;
}

//
//  allocate main memory
//
static void* __mainmem_malloc(size_t size) {
  void* addr = _dos_malloc(size);
  return ((uint32_t)addr >= 0x81000000) ? NULL : addr;
}

//
//  free main memory
//
static void __mainmem_free(void* ptr) {
  if (ptr == NULL) return;
  _dos_mfree(ptr);
}

//
//  getsize main memory
//
static size_t __mainmem_getsize() {
  return (size_t)0;
}

//
//  resize main memory
//
static int32_t __mainmem_resize(void* ptr, size_t size) {
  return _dos_setblock(ptr, size);
}

//
//  allocate memory
//
void* himem_malloc(size_t size, int32_t use_high_memory) {
    return use_high_memory ? __himem_malloc(size) : __mainmem_malloc(size);
}

//
//  free memory
//
void himem_free(void* ptr, int32_t use_high_memory) {
    if (use_high_memory) {
        __himem_free(ptr);
    } else {
        __mainmem_free(ptr);
    }
}

//
//  getsize memory
//
size_t himem_getsize(int32_t use_high_memory) {
    return use_high_memory ? __himem_getsize() : __mainmem_getsize();
}

//
//  resize memory
//
int32_t himem_resize(void* ptr, size_t size, int32_t use_high_memory) {
    return use_high_memory ? __himem_resize(ptr, size) : __mainmem_resize(ptr, size);
}

//
//  check high memory availability
//
int32_t himem_isavailable() {
  int32_t v = (int32_t)_dos_intvcg(0x1f8);
  return (v < 0 || (v >= 0xfe0000 && v <= 0xffffff)) ? 0 : 1;
}