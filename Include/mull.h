#ifndef SLABALLOCATOR_MULL_H
#define SLABALLOCATOR_MULL_H

#include <stdlib.h>

int mullinit(size_t size, int count) __result_use_check;
void* mulloc(int SlabID) __result_use_check;
void mullfree(void * restrict ptr, int SlabID);
void freeall(int SlabID);

#endif //SLABALLOCATOR_MULL_H
